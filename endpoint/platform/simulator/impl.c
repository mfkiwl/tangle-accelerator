/*
 * Copyright (C) 2019-2020 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common/ta_errors.h"
#include "endpoint/hal/device.h"
#include "legato.h"

#include "le_hashmap.h"
#include "le_log.h"

#define UART_BUFFER_SIZE 1024

/* Setting data to produce predictable results for simulator */
// device id
static const char *device_id = "470010171566423";
// private key
static const uint8_t private_key[32] = {82,  142, 184, 64,  74, 105, 126, 65,  154, 116, 14,  193, 208, 41,  8,  115,
                                        158, 252, 228, 160, 79, 5,   167, 185, 13,  159, 135, 113, 49,  209, 58, 68};
// Buffer for uart
static char UART_BUFFER[UART_BUFFER_SIZE];
// Hashmap to simulate secure storage
static le_hashmap_Ref_t simulator_sec_table;

extern struct device_type simulator_device_type;

static status_t simulator_init(void) {
  status_t err = register_device(&simulator_device_type);
  if (err != SC_OK) LE_ERROR("register simulator device error:%d", err);
  return err;
}

static void simulator_release(void) {
  status_t ret = unregister_device(&simulator_device_type);
  LE_INFO("unregister simulator return: %d", ret);
}

static status_t simulator_get_key(uint8_t *key) {
  memcpy(key, private_key, 16);
  LE_INFO("Get device key success");
  return SC_OK;
}

static status_t simulator_get_device_id(char *id) {
  memcpy(id, device_id, 16);
  LE_INFO("Get device id success");
  return SC_OK;
}

static status_t uart_init(const char *device) {
  LE_INFO("UART init device %s success", device);
  return SC_OK;
}

static void uart_write(const int fd, const char *cmd) {
  /* simple output */
  size_t cmd_len = strlen(cmd);
  if (cmd_len > UART_BUFFER_SIZE) {
    LE_ERROR("command too long");
    return;
  }
  snprintf(UART_BUFFER, cmd_len, "%s", cmd);
}

static char *uart_read(const int fd) {
  char *response = UART_BUFFER;
  return response;
}

static void uart_clean(const int fd) { LE_INFO("UART clean success"); }

static status_t sec_init(void) {
  LE_INFO("Initialize secure storage");
  simulator_sec_table =
      le_hashmap_Create("Simulator secure storage", 32, le_hashmap_HashString, le_hashmap_EqualsString);
  return SC_OK;
}

static status_t sec_write(const char *name, const uint8_t *buf, size_t buf_size) {
  uint8_t *data = malloc(buf_size);
  if (data == NULL) {
    LE_ERROR("Cannot fetch enough memory");
    return SC_OOM;
  }
  uint8_t *ptr;
  memcpy(data, buf, buf_size);

  LE_INFO("Write %s into secure storage", name);

  // the hashmap will return old value if it is replaced
  ptr = le_hashmap_Put(simulator_sec_table, name, data);
  if (ptr != NULL) {
    free(ptr);
  }
  return SC_OK;
}

static status_t sec_read(const char *name, uint8_t *buf, size_t *buf_size) {
  LE_INFO("Read %s from secure storage", name);
  uint8_t *data = le_hashmap_Get(simulator_sec_table, name);

  if (data == NULL) {
    *buf_size = 0;
    return SC_ENDPOINT_SEC_ITEM_NOT_FOUND;
  }

  memcpy(buf, data, *buf_size);
  return SC_OK;
}

static status_t sec_delete(const char *name) {
  LE_INFO("Delete %s in secure storage", name);
  uint8_t *data = le_hashmap_Remove(simulator_sec_table, name);

  if (data == NULL) {
    return SC_ENDPOINT_SEC_ITEM_NOT_FOUND;
  }

  free(data);
  return SC_OK;
}

static const struct device_operations simulator_ops = {
    .init = simulator_init,
    .fini = simulator_release,
    .get_key = simulator_get_key,
    .get_device_id = simulator_get_device_id,
};

static const struct uart_operations simulator_uart = {
    .init = uart_init,
    .write = uart_write,
    .read = uart_read,
    .clean = uart_clean,
};

static const struct secure_store_operations simulator_sec_ops = {
    .init = sec_init,
    .write = sec_write,
    .read = sec_read,
    .delete = sec_delete,
};

struct device_type simulator_device_type = {
    .name = "simulator",
    .op = &simulator_ops,
    .uart = &simulator_uart,
    .sec_ops = &simulator_sec_ops,
};

DECLARE_DEVICE(simulator);
