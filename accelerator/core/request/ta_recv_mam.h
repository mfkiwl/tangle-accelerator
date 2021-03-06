/*
 * Copyright (C) 2019-2020 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#ifndef REQUEST_TA_RECV_MAM_H_
#define REQUEST_TA_RECV_MAM_H_

#include <stdlib.h>
#include <string.h>
#include "common/macros.h"
#include "common/ta_errors.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file accelerator/core/request/ta_recv_mam.h
 */

/** struct of ta_recv_mam_req_t */
typedef struct recv_mam_req_s {
  /** Required. Data identifier. This struct pointer points to the implementation of data ID struct. */
  void* data_id;
  /** Required. Decryption key. This struct pointer points to the implementation of payload decryption key struct. */
  void* key;
  /** Required. Specified protocol. The protocol used in the current data. */
  mam_protocol_t protocol;
} ta_recv_mam_req_t;

/**
 * Allocate memory of ta_recv_mam_req_t
 *
 * @return
 * - struct of ta_recv_mam_req_t on success
 * - NULL on error
 */
ta_recv_mam_req_t* recv_mam_req_new();

/**
 * Free memory of ta_recv_mam_req_t
 *
 * @param[out] req Data type of ta_recv_mam_req_t
 */
void recv_mam_req_free(ta_recv_mam_req_t** req);

// TODO Implement interface of MAMv1.1

typedef struct recv_mam_data_id_mam_v1_s {
  /** bundle hash in trytes */
  tryte_t* bundle_hash;
  /** channel id in trytes */
  tryte_t* chid;
  /** message id in trytes */
  tryte_t* msg_id;
} recv_mam_data_id_mam_v1_t;

typedef struct recv_mam_key_mam_v1_s {
  /** Optional. The pre-shared key to decrypt the message. Each psk is in length of 81 trytes. Default: NULL. */
  UT_array* psk_array;
  /** Optional. The NTRU Secret key to decrypt the message. Each psk is in length of 1024 trytes. Default: NULL. */
  UT_array* ntru_array;
} recv_mam_key_mam_v1_t;

/**
 * @brief Set the data ID for MAMv1
 *
 * @param[in] req Response data in type of ta_recv_mam_req_t object
 * @param[in] bundle_hash Bundle hash of the message
 * @param[in] chid Channel ID of the messages
 * @param[in] msg_id Message ID of the message
 *
 * @return
 * - struct of ta_recv_mam_req_t on success
 * - NULL on error
 */
status_t recv_mam_set_mam_v1_data_id(ta_recv_mam_req_t* req, char* bundle_hash, char* chid, char* msg_id);

/**
 * @brief Set the key for MAMv1
 *
 * @param[in] req Response data in type of ta_recv_mam_req_t object
 * @param[in] psk Pre-Shared Key to decrypt message
 * @param[in] ntru NTRU public key to decrypt message
 *
 * @return
 * - struct of ta_recv_mam_req_t on success
 * - NULL on error
 */
status_t recv_mam_set_mam_v1_key(ta_recv_mam_req_t* req, tryte_t* psk, tryte_t* ntru);

/**
 * @brief Initialize 'ta_recv_mam_req_t' object as MAMv1 object
 *
 * @param[out] req 'ta_recv_mam_req_t' object to be initialized
 *
 * @return
 * - struct of ta_recv_mam_req_t on success
 * - NULL on error
 */
status_t recv_mam_req_v1_init(ta_recv_mam_req_t* req);

#ifdef __cplusplus
}
#endif

#endif  // REQUEST_TA_RECV_MAM_H_
