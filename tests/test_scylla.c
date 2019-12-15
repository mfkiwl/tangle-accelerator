/*
 * Copyright (C) 2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#include "storage/scylla_api.h"
#include "test_define.h"

static char* host = "localhost";
static char* keyspace_name;

typedef enum { TEST_ID_1 = 1, TEST_ID_2, TEST_ID_3, TEST_ID_4, TEST_ID_5, TEST_ID_6 } TEST_ID;

void test_get_column_from_edgeTable(db_client_service_t* service) {
  char expected_result[][FLEX_TRIT_SIZE_243] = {
      {BUNDLE_1},       // address_2
      {BUNDLE_2},       // address_2
      {BUNDLE_3},       // address_2
      {BUNDLE_1},       // address_1
      {TRANSACTION_4},  // approvees T7
      {TRANSACTION_3},  // approvees T7
      {TRANSACTION_6},  // approvees T7
      {TRANSACTION_5}   // approvees T7
  };
  hash243_queue_t res_hash = NULL;
  hash243_queue_entry_t* iter243 = NULL;
  get_column_from_edgeTable(service, &res_hash, (cass_byte_t*)ADDRESS_2, "bundle");
  get_column_from_edgeTable(service, &res_hash, (cass_byte_t*)ADDRESS_1, "bundle");
  get_column_from_edgeTable(service, &res_hash, (cass_byte_t*)TRANSACTION_7, "hash");
  size_t idx = 0;
  CDL_FOREACH(res_hash, iter243) {
    TEST_ASSERT_EQUAL_MEMORY(iter243->hash, (flex_trit_t*)expected_result[idx++],
                             sizeof(flex_trit_t) * FLEX_TRIT_SIZE_243);
  }
  hash243_queue_free(&res_hash);
  TEST_ASSERT_EQUAL_INT(idx, sizeof(expected_result) / (sizeof(char) * FLEX_TRIT_SIZE_243));
}

void test_get_transactions(db_client_service_t* service) {
  char expected_response_transactions[][NUM_FLEX_TRITS_HASH] = {
      {TRANSACTION_1},  // bundle 1
      {TRANSACTION_2},  // bundle 1
      {TRANSACTION_4},  // bundle 2
      {TRANSACTION_3},  // bundle 2
      {TRANSACTION_1},  // address_1
      {TRANSACTION_2},  // address_2
      {TRANSACTION_4},  // address_2
      {TRANSACTION_6},  // address_2
      {TRANSACTION_5},  // approvees T6
      {TRANSACTION_4},  // approvees T7
      {TRANSACTION_3},  // approvees T7
      {TRANSACTION_6},  // approvees T7
      {TRANSACTION_5}   // approvees T7
  };

  hash243_queue_t transactions = NULL;
  hash243_queue_entry_t* iter243 = NULL;
  hash243_queue_t bundles = NULL;
  hash243_queue_t addresses = NULL;
  hash243_queue_t approvees = NULL;
  hash243_queue_push(&bundles, (flex_trit_t const* const)BUNDLE_1);
  hash243_queue_push(&bundles, (flex_trit_t const* const)BUNDLE_2);

  hash243_queue_push(&addresses, (flex_trit_t const* const)ADDRESS_1);
  hash243_queue_push(&addresses, (flex_trit_t const* const)ADDRESS_2);

  hash243_queue_push(&approvees, (flex_trit_t const* const)TRANSACTION_6);
  hash243_queue_push(&approvees, (flex_trit_t const* const)TRANSACTION_7);

  get_transactions(service, &transactions, bundles, addresses, approvees);

  size_t idx = 0;
  CDL_FOREACH(transactions, iter243) {
    TEST_ASSERT_EQUAL_MEMORY(iter243->hash, (flex_trit_t*)expected_response_transactions[idx++],
                             sizeof(flex_trit_t) * NUM_FLEX_TRITS_HASH);
  }
  TEST_ASSERT_EQUAL_INT(idx, sizeof(expected_response_transactions) / (sizeof(char) * NUM_FLEX_TRITS_HASH));
  hash243_queue_free(&bundles);
  hash243_queue_free(&addresses);
  hash243_queue_free(&approvees);
  hash243_queue_free(&transactions);
}

void test_scylla(void) {
  /* input_transactions' graph
   * T1-T2 in BUNDLE 1 approved T3-T4 and T5-T6
   * T3-T4 in BUNDLE 2 approved T7-T8 and T9
   * T5-T6 in BUNDLE 3 approved T7-T8 and T9
   *
   *    T1-T2
   *    | /  \
   *    |/    \
   *    T3-T4  T5-T6
   *    | /__\_/__/
   *    |/    \  /
   *    T7-T8   T9
   */
  db_client_service_t db_client_service;
  char hashes[][NUM_FLEX_TRITS_HASH] = {
      {TRANSACTION_1}, {TRANSACTION_2}, {TRANSACTION_3}, {TRANSACTION_4}, {TRANSACTION_5}, {TRANSACTION_6},
  };
  char bundles[][NUM_FLEX_TRITS_BUNDLE] = {
      {BUNDLE_1}, {BUNDLE_1}, {BUNDLE_2}, {BUNDLE_2}, {BUNDLE_3}, {BUNDLE_3},
  };
  char addresses[][NUM_FLEX_TRITS_ADDRESS] = {
      {ADDRESS_1}, {ADDRESS_2}, {ADDRESS_3}, {ADDRESS_2}, {ADDRESS_4}, {ADDRESS_2},
  };
  char messages[][NUM_FLEX_TRITS_MESSAGE] = {
      {MESSAGE_1}, {MESSAGE_2}, {MESSAGE_3}, {MESSAGE_4}, {MESSAGE_5}, {MESSAGE_6},
  };
  char timestamps[][NUM_FLEX_TRITS_TIMESTAMP] = {
      {TIMESTAMP_1}, {TIMESTAMP_1}, {TIMESTAMP_2}, {TIMESTAMP_2}, {TIMESTAMP_3}, {TIMESTAMP_3},
  };
  char trunks[][NUM_FLEX_TRITS_TRUNK] = {
      {TRANSACTION_2}, {TRANSACTION_3}, {TRANSACTION_4}, {TRANSACTION_7}, {TRANSACTION_6}, {TRANSACTION_7},
  };
  char branches[][NUM_FLEX_TRITS_BRANCH] = {
      {TRANSACTION_3}, {TRANSACTION_5}, {TRANSACTION_7}, {TRANSACTION_9}, {TRANSACTION_7}, {TRANSACTION_9},
  };

  size_t tx_num = sizeof(hashes) / (NUM_FLEX_TRITS_HASH);
  scylla_iota_transaction_t* transaction;
  db_client_service.host = strdup(host);
  TEST_ASSERT_EQUAL_INT(db_client_service_init(&db_client_service), SC_OK);
  TEST_ASSERT_EQUAL_INT(db_permanent_keyspace_init(&db_client_service, true, keyspace_name), SC_OK);

  new_scylla_iota_transaction(&transaction);

  for (size_t i = 0; i < tx_num; i++) {
    set_transaction_hash(transaction, (cass_byte_t*)hashes[i], NUM_FLEX_TRITS_HASH);
    set_transaction_bundle(transaction, (cass_byte_t*)bundles[i], NUM_FLEX_TRITS_BUNDLE);
    set_transaction_address(transaction, (cass_byte_t*)addresses[i], NUM_FLEX_TRITS_ADDRESS);
    set_transaction_message(transaction, (cass_byte_t*)messages[i], NUM_FLEX_TRITS_MESSAGE);
    set_transaction_trunk(transaction, (cass_byte_t*)trunks[i], NUM_FLEX_TRITS_TRUNK);
    set_transaction_branch(transaction, (cass_byte_t*)branches[i], NUM_FLEX_TRITS_BRANCH);
    set_transaction_timestamp(transaction, (int64_t)timestamps[i]);
    set_transaction_value(transaction, (int64_t)i);
    /* insert test input transactions to ScyllyDB */
    TEST_ASSERT_EQUAL_INT(insert_transaction_into_bundleTable(&db_client_service, transaction, 1), SC_OK);
    TEST_ASSERT_EQUAL_INT(insert_transaction_into_edgeTable(&db_client_service, transaction, 1), SC_OK);
  }

  /* test select bundles by edge in edge table */
  test_get_column_from_edgeTable(&db_client_service);
  /* test find transactions by bundles or addresses */
  test_get_transactions(&db_client_service);

  free_scylla_iota_transaction(&transaction);
  db_client_service_free(&db_client_service);
}

void test_db_get_identity_objs_by_status(db_client_service_t* db_client_service) {
  const char expected_select_hashes[][NUM_FLEX_TRITS_HASH] = {
      {TRANSACTION_2}, {TRANSACTION_2}, {TRANSACTION_1}, {TRANSACTION_1}, {TRANSACTION_3}, {TRANSACTION_3},
  };
  db_identity_array_t* db_identity_array = db_identity_array_new();
  db_get_identity_objs_by_status(db_client_service, INSERTING_TXN, db_identity_array);
  db_get_identity_objs_by_status(db_client_service, PENDING_TXN, db_identity_array);
  db_get_identity_objs_by_status(db_client_service, CONFIRMED_TXN, db_identity_array);
  db_identity_t* itr;
  int idx = 0;
  IDENTITY_TABLE_ARRAY_FOREACH(db_identity_array, itr) {
    TEST_ASSERT_EQUAL_MEMORY(db_ret_identity_hash(itr), (flex_trit_t*)expected_select_hashes[idx % 6],
                             sizeof(flex_trit_t) * NUM_FLEX_TRITS_HASH);
    idx++;
  }
  db_identity_array_free(&db_identity_array);
}

void test_db_get_identity_objs_by_id(db_client_service_t* db_client_service) {
  const char expected_select_hashes[][NUM_FLEX_TRITS_HASH] = {{TRANSACTION_2}, {TRANSACTION_3}, {TRANSACTION_1}};
  db_identity_array_t* db_identity_array = db_identity_array_new();
  db_get_identity_objs_by_id(db_client_service, TEST_ID_3, db_identity_array);
  db_get_identity_objs_by_id(db_client_service, TEST_ID_6, db_identity_array);
  db_get_identity_objs_by_id(db_client_service, TEST_ID_1, db_identity_array);
  db_identity_t* itr;
  int idx = 0;
  IDENTITY_TABLE_ARRAY_FOREACH(db_identity_array, itr) {
    TEST_ASSERT_EQUAL_MEMORY(db_ret_identity_hash(itr), (flex_trit_t*)expected_select_hashes[idx],
                             sizeof(flex_trit_t) * NUM_FLEX_TRITS_HASH);
    idx++;
  }
  db_identity_array_free(&db_identity_array);
}

void test_db_get_identity_objs_by_hash(db_client_service_t* db_client_service) {
  const int64_t expected_select_id[] = {TEST_ID_1, TEST_ID_2, TEST_ID_5, TEST_ID_6};
  db_identity_array_t* db_identity_array = db_identity_array_new();
  db_get_identity_objs_by_hash(db_client_service, (cass_byte_t*)TRANSACTION_1, db_identity_array);
  db_get_identity_objs_by_hash(db_client_service, (cass_byte_t*)TRANSACTION_3, db_identity_array);
  db_identity_t* itr;
  int idx = 0;
  IDENTITY_TABLE_ARRAY_FOREACH(db_identity_array, itr) {
    TEST_ASSERT_EQUAL_INT(db_ret_identity_id(itr), expected_select_id[idx]);
    idx++;
  }
  db_identity_array_free(&db_identity_array);
}

void test_db_identity_table(void) {
  db_client_service_t db_client_service;

  const char hashes[][NUM_FLEX_TRITS_HASH] = {
      {TRANSACTION_1}, {TRANSACTION_1}, {TRANSACTION_2}, {TRANSACTION_2}, {TRANSACTION_3}, {TRANSACTION_3},
  };
  const int64_t inserted_ids[] = {TEST_ID_1, TEST_ID_2, TEST_ID_3, TEST_ID_4, TEST_ID_5, TEST_ID_6};
  const int inserted_status[] = {PENDING_TXN, PENDING_TXN, INSERTING_TXN, INSERTING_TXN, CONFIRMED_TXN, CONFIRMED_TXN};

  db_client_service.host = strdup(host);
  TEST_ASSERT_EQUAL_INT(db_client_service_init(&db_client_service), SC_OK);
  TEST_ASSERT_EQUAL_INT(db_init_identity_keyspace(&db_client_service, true, keyspace_name), SC_OK);
  for (int i = 0; i < 6; i++) {
    db_insert_tx_into_identity(&db_client_service, inserted_ids[i], hashes[i], inserted_status[i]);
  }
  test_db_get_identity_objs_by_status(&db_client_service);
  test_db_get_identity_objs_by_id(&db_client_service);
  test_db_get_identity_objs_by_hash(&db_client_service);

  db_client_service_free(&db_client_service);
}

int main(int argc, char** argv) {
  int cmdOpt;
  int optIdx;
  const struct option longOpt[] = {
      {"host", required_argument, NULL, 'h'}, {"keyspace", required_argument, NULL, 'k'}, {NULL, 0, NULL, 0}};

  keyspace_name = "test_scylla";
  /* Parse the command line options */
  /* TODO: Support macOS since getopt_long() is GNU extension */
  while (1) {
    cmdOpt = getopt_long(argc, argv, "b:", longOpt, &optIdx);
    if (cmdOpt == -1) break;

    /* Invalid option */
    if (cmdOpt == '?') break;

    if (cmdOpt == 'h') {
      host = optarg;
    }
    if (cmdOpt == 'k') {
      keyspace_name = optarg;
    }
  }

  UNITY_BEGIN();
  if (ta_logger_init() != SC_OK) {
    return EXIT_FAILURE;
  }
  scylla_api_logger_init();
  RUN_TEST(test_db_identity_table);
  RUN_TEST(test_scylla);
  scylla_api_logger_release();
  return UNITY_END();
}
