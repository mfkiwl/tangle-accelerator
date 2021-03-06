/*
 * Copyright (C) 2019-2020 BiiLabs Co., Ltd. and Contributors
 * All Rights Reserved.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the MIT license. A copy of the license can be found in the file
 * "LICENSE" at the root of this distribution.
 */

#ifndef CONN_HTTP_H
#define CONN_HTTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "common/ta_errors.h"
#include "mbedtls/certs.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"

typedef struct {
  bool https;                         /**< Flag for check whether SSL connection is enabled or not */
  mbedtls_net_context *net_ctx;       /**< mbedtls_net context                       */
  mbedtls_entropy_context *entropy;   /**< mbedtls_entropy context                    */
  mbedtls_ctr_drbg_context *ctr_drbg; /**< mbedlts_ctr_drbg context                   */
  mbedtls_ssl_context *ssl_ctx;       /**< mbedtls_ssl context                        */
  mbedtls_ssl_config *ssl_config;     /**< mbedtls_ssl_config context                 */
  mbedtls_x509_crt *cacert;           /**< mbedtls_x509 container                     */
} connect_info_t;

/**
 * @brief Initialize logger of conn_http
 */
void conn_http_logger_init();

/**
 * @brief Release logger of conn_http
 *
 * @return
 * - zero on success
 * - EXIT_FAILURE on error
 */
int conn_http_logger_release();

/**
 * @brief Open HTTP(S) connection
 *
 * @param[in, out] info Context for HTTP(S) connection
 * @param[in] seed_nonce Personalization data, that is device-specific identifiers. Can be NULL.
 * @param[in] host HTTP(S) host to connect
 * @param[in] port HTTP(S) port to connect
 *
 * @return
 * - #SC_UTILS_HTTPS_INIT_ERROR failed on HTTP(S) init error
 * - #SC_UTILS_HTTPS_CONN_ERROR failed on HTTP(S) connect error
 * - #SC_UTILS_HTTPS_X509_ERROR failed on HTTP(S) certificate setting error
 * - #SC_UTILS_HTTPS_SSL_ERROR failed on HTTP ssl setting error
 * - #SC_OK on success
 * @see #status_t
 */
status_t http_open(connect_info_t *const info, char const *const seed_nonce, char const *const host,
                   char const *const port);

/**
 * @brief Send request to HTTP(S) connection
 *
 * @param[in] info Context for HTTP(S) connection
 * @param[in] req Buffer holding the data
 *
 * @return
 * - #SC_OK on success
 * - non-zero on error
 * @see #status_t
 */
status_t http_send_request(connect_info_t *const info, const char *req);

/**
 * @brief Read response from HTTP(S) server
 *
 * @param[in] info Context for HTTP(S) connection
 * @param[out] res Buffer that will hold the data
 * @param[out] res_len Length of res
 *
 * @return
 * - #SC_OK on success
 * - non-zero on error
 * @see #status_t
 */
status_t http_read_response(connect_info_t *const info, char *res, size_t res_len);

/**
 * @brief Close HTTP(S) connection
 *
 * @param[in] info Context for HTTP(S) connection
 *
 * @return
 * - #SC_OK on success
 * - non-zero on error
 * @see #status_t
 */
status_t http_close(connect_info_t *const info);

/**
 * @brief Set POST request message
 *
 * @param[in] path API path for POST request to HTTP(S) server, i.e "transaction/". It
 * must be in string.
 * @param[in] host HTTP(S) host to connect
 * @param[in] port HTTP(S) port to connect
 * @param[in] req_body Pointer of POST request body
 * @param[out] out POST request message
 *
 * @return
 * - #SC_OK on success
 * - #SC_OOM failed on out of memory error
 * - non-zero on error
 * @see #status_t
 */
status_t set_post_request(char const *const path, char const *const host, const uint32_t port,
                          char const *const req_body, char **out);
/**
 * @brief Set GET request message
 *
 * @param[in] path API path for GET request to HTTP(S) server, i.e "transaction/". It
 * must be in string.
 * @param[in] host HTTP(S) host to connect
 * @param[in] port HTTP(S) port to connect
 * @param[out] out GET request message
 *
 * @return
 * - #SC_OK on success
 * - #SC_OOM failed on out of memory error
 * - non-zero on error
 * @see #status_t
 */
status_t set_get_request(char const *const path, char const *const host, const uint32_t port, char **out);

#ifdef __cplusplus
}
#endif

#endif  // CONN_HTTP_H
