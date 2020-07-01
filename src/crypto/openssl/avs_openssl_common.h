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
#ifndef AVS_COMMONS_CRYPTO_OPENSSL_COMMON_H
#define AVS_COMMONS_CRYPTO_OPENSSL_COMMON_H

#include <openssl/err.h>

#ifdef AVS_COMMONS_WITH_AVS_CRYPTO_VALGRIND
#    include <stdint.h>
#    include <valgrind/helgrind.h>
#    include <valgrind/memcheck.h>
#    include <valgrind/valgrind.h>
extern void *sbrk(intptr_t __delta);
#else
#    define RUNNING_ON_VALGRIND 0
#    define VALGRIND_HG_DISABLE_CHECKING(addr, len) ((void) 0)
#    define VALGRIND_MAKE_MEM_DEFINED_IF_ADDRESSABLE(addr, len) ((void) 0)
#endif

VISIBILITY_PRIVATE_HEADER_BEGIN

#ifdef AVS_COMMONS_WITH_INTERNAL_LOGS

#    define log_openssl_error()                                                \
        do {                                                                   \
            char error_buffer[256]; /* see 'man ERR_error_string' */           \
            LOG(ERROR, "%s", ERR_error_string(ERR_get_error(), error_buffer)); \
        } while (0)

#else // AVS_COMMONS_WITH_INTERNAL_LOGS

#    define log_openssl_error() ((void) 0)

#endif // AVS_COMMONS_WITH_INTERNAL_LOGS

VISIBILITY_PRIVATE_HEADER_END

#endif // AVS_COMMONS_CRYPTO_OPENSSL_COMMON_H