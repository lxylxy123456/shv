/*
 * @XMHF_LICENSE_HEADER_START@
 *
 * eXtensible, Modular Hypervisor Framework (XMHF)
 * Copyright (c) 2009-2012 Carnegie Mellon University
 * Copyright (c) 2010-2012 VDG Inc.
 * All Rights Reserved.
 *
 * Developed by: XMHF Team
 *               Carnegie Mellon University / CyLab
 *               VDG Inc.
 *               http://xmhf.org
 *
 * This file is part of the EMHF historical reference
 * codebase, and is released under the terms of the
 * GNU General Public License (GPL) version 2.
 * Please see the LICENSE file for details.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @XMHF_LICENSE_HEADER_END@
 */

#include <unity.h>
#include <string.h>
#include <assert.h>

#include "tcm.h"

#include "Mockaudited-kv.h"
#include "Mockaudit.h"

audit_ctx_t g_audit_ctx;
tcm_ctx_t tcm_ctx;
akv_ctx_t akv_ctx;

void setUp(void)
{
  tcm_init(&tcm_ctx, &g_audit_ctx, &akv_ctx);
}

void tearDown(void)
{
  tcm_release(&tcm_ctx);
}

void test_tcm_init_null_ctx_error(void)
{
  TEST_ASSERT_EQUAL(TCM_EINVAL, tcm_init(NULL, NULL, NULL));
}

void test_tcm_init_null_params_err(void)
{
  tcm_ctx_t tcm_ctx;
  TEST_ASSERT_EQUAL(TCM_EINVAL, tcm_init(&tcm_ctx, NULL, NULL));
  TEST_ASSERT_EQUAL(TCM_EINVAL, tcm_init(&tcm_ctx, &g_audit_ctx, NULL));
  TEST_ASSERT_EQUAL(TCM_EINVAL, tcm_init(&tcm_ctx, NULL, &akv_ctx));
}

void test_tcm_init_saves_ctxts(void)
{
  tcm_ctx_t tcm_ctx;
  audit_ctx_t audit_ctx;
  tcm_init(&tcm_ctx, &audit_ctx, &akv_ctx);
  TEST_ASSERT_EQUAL_PTR(&audit_ctx, tcm_ctx.audit_ctx);
  TEST_ASSERT_EQUAL_PTR(&akv_ctx, tcm_ctx.akv_ctx);
}

void test_tcm_release_null_ok(void)
{
  tcm_release(NULL);
}

void test_tcm_release_uninitd_ok(void)
{
  tcm_ctx_t tcm_ctx;
  tcm_release(&tcm_ctx);
}

void test_tcm_release_initd_ok(void)
{
  tcm_ctx_t tcm_ctx;
  tcm_init(&tcm_ctx, NULL, NULL);
  tcm_release(&tcm_ctx);
}

void test_tcm_db_add_null_ctx_error(void)
{
  TEST_ASSERT_EQUAL(TCM_EINVAL, tcm_db_add(NULL, NULL, NULL));
}

void test_tcm_db_add_null_key_error(void)
{
  TEST_ASSERT_EQUAL(TCM_EINVAL, tcm_db_add(&tcm_ctx, NULL, "foo"));
}

void test_tcm_db_add_null_val_error(void)
{
  TEST_ASSERT_EQUAL(TCM_EINVAL, tcm_db_add(&tcm_ctx, "foo", NULL));
}

void test_tcm_db_add_gets_audit_challenge(void)
{
  akv_begin_db_add_IgnoreAndReturn(0);
  audit_get_token_IgnoreAndReturn(0);

  tcm_db_add(&tcm_ctx, "key", "value");
}

void test_tcm_db_add_call_akv_reasonable_params(void)
{
  const char *test_key = "key";
  const char *test_val = "val";
  int callcount=0;

  int akv_begin_db_add_cb(akv_ctx_t*  ctx,
                          uint8_t*    epoch_nonce,
                          size_t*     epoch_nonce_len,
                          uint64_t*   epoch_offset,
                          char*       audit_string,
                          size_t*     audit_string_len,
                          const char* key,
                          const char* val,
                          int num_calls
                          )
  {
    callcount++;
    TEST_ASSERT_EQUAL_PTR(&akv_ctx, ctx);
    TEST_ASSERT_EQUAL_STRING(test_key, key);
    TEST_ASSERT_EQUAL_STRING(test_val, val);
    return 0;
  }
  
  akv_begin_db_add_StubWithCallback(&akv_begin_db_add_cb);
  audit_get_token_IgnoreAndReturn(0);

  tcm_db_add(&tcm_ctx, test_key, test_val);
  TEST_ASSERT(callcount > 0);
}

void test_tcm_db_add_detects_akv_failure(void)
{
  akv_begin_db_add_IgnoreAndReturn(1);
  TEST_ASSERT(tcm_db_add(&tcm_ctx, "key", "value"));
}

void test_tcm_db_add_calls_audit_get_token_with_reasonable_param(void)
{
  uint8_t test_epoch_nonce[] = { 0xde, 0xad, 0xbe, 0xef };
  size_t test_epoch_nonce_len = sizeof(test_epoch_nonce);
  uint64_t test_epoch_offset = 0xfeedfeedfeedfeedULL;
  const char *test_audit_string = "audit command string";
  size_t test_audit_string_len = strlen(test_audit_string)+1;

  int akv_callcount=0;
  int akv_begin_db_add_cb(akv_ctx_t*  ctx,
                          uint8_t*    epoch_nonce,
                          size_t*     epoch_nonce_len,
                          uint64_t*   epoch_offset,
                          char*       audit_string,
                          size_t*     audit_string_len,
                          const char* key,
                          const char* val,
                          int num_calls
                          ) {
    akv_callcount++;

    TEST_ASSERT_NOT_NULL(epoch_nonce_len);
    assert(*epoch_nonce_len >= test_epoch_nonce_len);
    memcpy(epoch_nonce, test_epoch_nonce, test_epoch_nonce_len);
    *epoch_nonce_len = test_epoch_nonce_len;

    *epoch_offset = test_epoch_offset;

    assert(*audit_string_len >= test_audit_string_len);
    strcpy(audit_string, test_audit_string);

    return 0;
  }
  int audit_callcount=0;
  int audit_get_token_cb(audit_ctx_t*    audit_ctx,
                         const uint8_t*  epoch_nonce,
                         size_t          epoch_nonce_len,
                         uint64_t        epoch_offset,
                         const char*     audit_string,
                         size_t          audit_string_len,
                         int             count)
  {
    audit_callcount++;
    TEST_ASSERT_EQUAL_PTR(&g_audit_ctx, audit_ctx);
    TEST_ASSERT_EQUAL(test_epoch_nonce_len, epoch_nonce_len);
    TEST_ASSERT_EQUAL_MEMORY(test_epoch_nonce, epoch_nonce, test_epoch_nonce_len);
    TEST_ASSERT_EQUAL_UINT64(test_epoch_offset, epoch_offset);
    return 0;
  }

  akv_begin_db_add_StubWithCallback(&akv_begin_db_add_cb);
  audit_get_token_StubWithCallback(&audit_get_token_cb);

  tcm_db_add(&tcm_ctx, "key", "value");
  TEST_ASSERT(akv_callcount > 0);
  TEST_ASSERT(audit_callcount > 0);
}
