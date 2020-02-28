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

#define AVS_UNIT_ENABLE_SHORT_ASSERTS
#include <avsystem/commons/avs_unit_test.h>

#include <avsystem/commons/avs_memory.h>
#include <avsystem/commons/avs_prng.h>

#include <string.h>

static int entropy_callback(unsigned char *out_buf, size_t out_buf_len) {
    memset(out_buf, 7, out_buf_len);
    return 0;
}

AVS_UNIT_TEST(avs_crypto_prng, get_random_bytes) {
    const size_t test_buf_len = 64;
    avs_crypto_prng_ctx_t *ctx = avs_crypto_prng_new(entropy_callback);
    ASSERT_NOT_NULL(ctx);

    unsigned char *test_buf = (unsigned char *) avs_calloc(1, test_buf_len);
    ASSERT_NOT_NULL(test_buf);

    unsigned char *compare_buf = (unsigned char *) avs_calloc(1, test_buf_len);
    ASSERT_NOT_NULL(compare_buf);

    ASSERT_OK(avs_crypto_prng_bytes(ctx, test_buf, test_buf_len));

    ASSERT_NE_BYTES_SIZED(test_buf, compare_buf, test_buf_len);

    avs_crypto_prng_free(ctx);
    avs_free(test_buf);
    avs_free(compare_buf);
}
