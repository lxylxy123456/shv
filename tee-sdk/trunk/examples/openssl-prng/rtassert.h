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

/* Copyright 2011, NoFuss Security, Inc.
 * Author - Jim Newsome (jnewsome@no-fuss.com)
 */

#ifndef RTASSERT_H
#define RTASSERT_H

#ifndef RTA_PRINT
#include <stdio.h>
#define RTA_PRINT(fmt, args...) fprintf(stderr, fmt , ## args) /* the spaces are important */
#endif

#ifndef RTA_EXIT
#define RTA_EXIT(x) exit(x)
#endif

/* distinguish from regular assert, which are for conditions that
   should always hold unless there was a programming error. This is
   for unhandled run-time errors that could conceivably happen. (e.g.,
   out of memory, system call returns an error, etc.), and therefore
   should never be disabled, unlike regular assert which may be
   disabled in production. also add an error string.  */

#define rtassert_eq(got, expected)              \
  do {                                          \
  int _got = (got);                             \
  int _expected = (expected);                   \
  if(_got != _expected) {                       \
    RTA_PRINT("unhandled run-time error."       \
              " expected %d got %d"             \
              " at %s:%d",                      \
              _expected,                        \
              _got,                             \
              __FILE__,                         \
              __LINE__);                        \
    RTA_EXIT(1);                                \
  }                                             \
  } while(0)

#define rtassert_not(x) rtassert_eq(x, 0)

#define rtassert(x) rtassert_not(!(x), 0)

#endif
