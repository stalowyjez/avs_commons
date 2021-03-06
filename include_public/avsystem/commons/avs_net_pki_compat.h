/*
 * Copyright 2017-2020 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file avs_net_pki_compat.h
 *
 * Aliases for compatibility with avs_commons 4.2 and earlier.
 */

#ifndef AVS_COMMONS_NET_PKI_COMPAT_H
#define AVS_COMMONS_NET_PKI_COMPAT_H

#include <avsystem/commons/avs_crypto_pki.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef avs_crypto_certificate_chain_info_t avs_net_client_cert_info_t;
typedef avs_crypto_private_key_info_t avs_net_client_key_info_t;
typedef avs_crypto_security_info_union_t avs_net_security_info_union_t;
typedef avs_crypto_certificate_chain_info_t avs_net_trusted_cert_info_t;

#define avs_net_client_cert_info_from_buffer \
    avs_crypto_certificate_chain_info_from_buffer
#define avs_net_client_cert_info_from_file \
    avs_crypto_certificate_chain_info_from_file
#define avs_net_client_key_info_from_buffer \
    avs_crypto_private_key_info_from_buffer
#define avs_net_client_key_info_from_file avs_crypto_private_key_info_from_file
#define avs_net_trusted_cert_info_from_buffer \
    avs_crypto_certificate_chain_info_from_buffer
#define avs_net_trusted_cert_info_from_file \
    avs_crypto_certificate_chain_info_from_file
#define avs_net_trusted_cert_info_from_path \
    avs_crypto_certificate_chain_info_from_path

#ifdef __cplusplus
}
#endif

#endif /* AVS_COMMONS_NET_PKI_COMPAT_H */
