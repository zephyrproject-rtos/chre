/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CHPP_MACROS_H_
#define CHPP_MACROS_H_

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOST_SIGNIFICANT_NIBBLE 0xf0
#define LEAST_SIGNIFICANT_NIBBLE 0x0f

#ifndef UNUSED_VAR
#define UNUSED_VAR(var) ((void)(var))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef CHPP_ASSERT
#define CHPP_ASSERT(var) assert(var)
#endif

#ifndef CHPP_NOT_NULL
#define CHPP_NOT_NULL(var) CHPP_ASSERT((var) != NULL)
#endif

#ifndef CHPP_DEBUG_ASSERT
#define CHPP_DEBUG_ASSERT(var) CHPP_ASSERT(var)
#endif

#if defined(__GNUC__) && (__STDC_VERSION__ >= 201112L)
#define CHPP_C11_OR_NEWER
#endif

#ifdef CHPP_C11_OR_NEWER
#define CHPP_STATIC_ASSERT _Static_assert

#else
#define CHPP_STATIC_ASSERT4(cond, var) typedef char var[(!!(cond)) * 2 - 1]
#define CHPP_STATIC_ASSERT3(cond, line) \
  CHPP_STATIC_ASSERT4(cond, static_assertion_at_line_##line)
#define CHPP_STATIC_ASSERT2(cond, line) CHPP_STATIC_ASSERT3(cond, line)
#define CHPP_STATIC_ASSERT(cond, msg) CHPP_STATIC_ASSERT2(cond, __LINE__)

#endif

// Time-related macros
#define CHPP_TIME_NONE 0
#define CHPP_MSEC_PER_SEC UINT64_C(1000)
#define CHPP_USEC_PER_MSEC UINT64_C(1000)
#define CHPP_NSEC_PER_USEC UINT64_C(1000)
#define CHPP_USEC_PER_SEC (CHPP_USEC_PER_MSEC * CHPP_MSEC_PER_SEC)
#define CHPP_NSEC_PER_MSEC (CHPP_NSEC_PER_USEC * CHPP_USEC_PER_MSEC)
#define CHPP_NSEC_PER_SEC \
  (CHPP_NSEC_PER_USEC * CHPP_USEC_PER_MSEC * CHPP_MSEC_PER_SEC)

#if defined(__GNUC__) || defined(__clang__)
#define check_types_match(t1, t2) ((__typeof__(t1) *)0 != (__typeof__(t2) *)0)
#else
#define check_types_match(t1, t2) 0
#endif

#define container_of(ptr, type, member)                     \
  ((type *)(void *)((char *)(ptr)-offsetof(type, member)) + \
   check_types_match(*(ptr), ((type *)0)->member))

#define sizeof_member(type, member) (sizeof(((type *)0)->member))

/**
 * Macros for defining (compiler dependent) packed structures
 */
#if defined(__GNUC__) || defined(__clang__)
// For GCC and clang
#define CHPP_PACKED_START
#define CHPP_PACKED_END
#define CHPP_PACKED_ATTR __attribute__((packed))
#elif defined(__ICCARM__) || defined(__CC_ARM)
// For IAR ARM and Keil MDK-ARM compilers
#define CHPP_PACKED_START _Pragma("pack(push, 1)")
#define CHPP_PACKED_END _Pragma("pack(pop)")
#define CHPP_PACKED_ATTR
#else
// Unknown compiler
#error Unrecognized compiler
#endif

#define CHPP_FREE_AND_NULLIFY(p) \
  do {                           \
    chppFree(p);                 \
    (p) = NULL;                  \
  } while (0)

#ifdef __cplusplus
}
#endif

#endif  // CHPP_MACROS_H_
