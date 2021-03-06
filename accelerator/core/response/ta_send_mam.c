/*
 * Copyright (C) 2019 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#include "ta_send_mam.h"

ta_send_mam_res_t* send_mam_res_new() {
  ta_send_mam_res_t* res = (ta_send_mam_res_t*)malloc(sizeof(ta_send_mam_res_t));
  memset(res->chid, 0, NUM_TRYTES_ADDRESS + 1);
  memset(res->chid1, 0, NUM_TRYTES_ADDRESS + 1);
  memset(res->bundle_hash, 0, NUM_TRYTES_BUNDLE + 1);
  memset(res->msg_id, 0, NUM_TRYTES_MAM_MSG_ID + 1);
  memset(res->announcement_bundle_hash, 0, NUM_TRYTES_BUNDLE + 1);
  return res;
}

status_t send_mam_res_set_bundle_hash(ta_send_mam_res_t* res, const tryte_t* bundle_hash) {
  if (!bundle_hash || !res) {
    return SC_NULL;
  }

  memcpy(res->bundle_hash, bundle_hash, NUM_TRYTES_HASH);
  res->bundle_hash[NUM_TRYTES_HASH] = '\0';
  return SC_OK;
}

status_t send_mam_res_set_channel_id(ta_send_mam_res_t* res, const tryte_t* channel_id) {
  if (!channel_id || !res) {
    return SC_NULL;
  }

  memcpy(res->chid, channel_id, NUM_TRYTES_HASH);
  res->chid[NUM_TRYTES_HASH] = '\0';
  return SC_OK;
}

status_t send_mam_res_set_msg_id(ta_send_mam_res_t* res, const tryte_t* msg_id) {
  if (!msg_id || !res) {
    return SC_NULL;
  }

  memcpy(res->msg_id, msg_id, NUM_TRYTES_MAM_MSG_ID);
  res->msg_id[NUM_TRYTES_MAM_MSG_ID] = '\0';
  return SC_OK;
}

status_t send_mam_res_set_msg_result(ta_send_mam_res_t* res, const tryte_t* chid, const tryte_t* msg_id,
                                     bundle_transactions_t* bundle) {
  status_t ret = SC_OK;
  ret = send_mam_res_set_channel_id(res, chid);
  if (ret) {
    goto done;
  }

  ret = send_mam_res_set_msg_id(res, msg_id);
  if (ret) {
    goto done;
  }

  ret = send_mam_res_set_bundle_hash(res, transaction_bundle((iota_transaction_t*)utarray_front(bundle)));

done:
  return SC_OK;
}

status_t send_mam_res_set_announce_bundle_hash(ta_send_mam_res_t* res, const tryte_t* announcement_bundle_hash) {
  if (!announcement_bundle_hash || !res) {
    return SC_NULL;
  }

  memcpy(res->announcement_bundle_hash, announcement_bundle_hash, NUM_TRYTES_HASH);
  res->announcement_bundle_hash[NUM_TRYTES_HASH] = '\0';
  return SC_OK;
}

status_t send_mam_res_set_chid1(ta_send_mam_res_t* res, const tryte_t* chid1) {
  if (!chid1 || !res) {
    return SC_NULL;
  }

  memcpy(res->chid1, chid1, NUM_TRYTES_HASH);
  res->chid1[NUM_TRYTES_HASH] = '\0';
  return SC_OK;
}

status_t send_mam_res_set_announce(ta_send_mam_res_t* res, const tryte_t* chid1, bundle_transactions_t* bundle) {
  status_t ret = SC_OK;
  ret = send_mam_res_set_announce_bundle_hash(res, transaction_bundle((iota_transaction_t*)utarray_front(bundle)));
  if (ret) {
    goto done;
  }

  ret = send_mam_res_set_chid1(res, chid1);

done:
  return SC_OK;
}

void send_mam_res_free(ta_send_mam_res_t** res) {
  if (!res || !(*res)) {
    return;
  }

  free(*res);
  *res = NULL;
}
