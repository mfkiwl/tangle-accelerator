#include <errno.h>

#include "common/logger.h"
#include "common/ta_errors.h"
#include "connectivity/common.h"
#include "connectivity/http/http.h"
#include "pthread.h"
#include "runtime_cli.h"
#include "time.h"
#include "utils/handles/signal.h"

#define MAIN_LOGGER "main"

static ta_core_t ta_core;
static ta_http_t ta_http;
static logger_id_t logger_id;

static void ta_stop(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    ta_http_stop(&ta_http);
  }
}

static void* health_track(void* arg) {
  ta_core_t* core = (ta_core_t*)arg;
  while (core->cache.state) {
    status_t ret = ta_get_node_status(&core->iota_service);
    if (ret == SC_CORE_NODE_UNSYNC || ret == SC_CCLIENT_FAILED_RESPONSE) {
      ta_log_error("IOTA full node status error %d. Try to connect to another IOTA full node host on priority list\n",
                   ret);
      ret = ta_update_node_connection(&core->ta_conf, &core->iota_service);
      if (ret) {
        ta_log_error("Update IOTA full node host failed: %d\n", ret);
      }
    }

    // Broadcast buffered transactions
    if (ret == SC_OK) {
      ret = broadcast_buffered_txn(core);
      if (ret) {
        ta_log_error("Broadcast buffered transactions failed. %s\n", ta_error_to_string(ret));
      }
    }

    char uuid[UUID_STR_LEN] = {};
    // The usage exceeds the maximum redis capacity
    while (core->cache.capacity < cache_occupied_space()) {
      if (pthread_rwlock_trywrlock(core->cache.rwlock)) {
        ta_log_error("%s\n", ta_error_to_string(SC_CACHE_LOCK_FAILURE));
        break;
      }

      ret = cache_list_pop(core->cache.done_list_name, uuid);
      if (ret) {
        ta_log_error("%s\n", ta_error_to_string(ret));
      }
      ret = cache_del(uuid);
      if (ret) {
        ta_log_error("%s\n", ta_error_to_string(ret));
      }

      if (pthread_rwlock_unlock(core->cache.rwlock)) {
        ta_log_error("%s\n", ta_error_to_string(SC_CACHE_LOCK_FAILURE));
      }
    }

    sleep(core->ta_conf.health_track_period);
  }
  return ((void*)NULL);
}

int main(int argc, char* argv[]) {
  if (signal_handle_register(SIGINT, ta_stop) == SIG_ERR || signal_handle_register(SIGTERM, ta_stop) == SIG_ERR) {
    return EXIT_FAILURE;
  }

  // Initialize logger
  if (ta_logger_init() != SC_OK) {
    return EXIT_FAILURE;
  }

  logger_id = logger_helper_enable(MAIN_LOGGER, LOGGER_DEBUG, true);

  // Initialize configurations with default value
  if (ta_core_default_init(&ta_core) != SC_OK) {
    return EXIT_FAILURE;
  }

  // Initialize configurations with file value
  if (ta_core_file_init(&ta_core, argc, argv) != SC_OK) {
    return EXIT_FAILURE;
  }

  // Initialize configurations with CLI value
  if (ta_core_cli_init(&ta_core, argc, argv) != SC_OK) {
    return EXIT_FAILURE;
  }

  if (ta_core_set(&ta_core) != SC_OK) {
    ta_log_error("Configure failed %s.\n", MAIN_LOGGER);
    return EXIT_FAILURE;
  }

  pthread_t health_thread;
  pthread_create(&health_thread, NULL, health_track, (void*)&ta_core);

  if (is_option_enabled(&ta_core.ta_conf, CLI_RUNTIME_CLI)) {
    pthread_t cli_thread;
    pthread_create(&cli_thread, NULL, cli_routine, (void*)&ta_core);
  }

  if (ta_http_init(&ta_http, &ta_core) != SC_OK) {
    ta_log_error("HTTP initialization failed %s.\n", MAIN_LOGGER);
    return EXIT_FAILURE;
  }

  if (ta_http_start(&ta_http) != SC_OK) {
    ta_log_error("Starting TA failed %s.\n", MAIN_LOGGER);
    goto cleanup;
  }

  log_info(logger_id, "Tangle-accelerator starts running\n");
  ta_logger_switch(is_option_enabled(&ta_core.ta_conf, CLI_QUIET_MODE), true, &(ta_core.ta_conf));

  // Once tangle-accelerator finished initializing, notify regression test script with unix domain socket
  notification_trigger();

  /* pause() cause TA to sleep until it catch a signal,
   * also the return value and errno should be -1 and EINTR on success.
   */
  int sig_ret = pause();
  if (sig_ret == -1 && errno != EINTR) {
    ta_log_error("Signal caught failed %s.\n", MAIN_LOGGER);
    return EXIT_FAILURE;
  }

cleanup:
  log_info(logger_id, "Destroying TA configurations\n");
  ta_logger_switch(true, false, &(ta_core.ta_conf));
  ta_core_destroy(&ta_core);
  logger_helper_release(logger_id);
  if (logger_helper_destroy() != RC_OK) {
    ta_log_error("Destroying logger failed %s.\n", MAIN_LOGGER);
    return EXIT_FAILURE;
  }
  return 0;
}
