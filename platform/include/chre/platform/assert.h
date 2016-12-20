/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef CHRE_PLATFORM_ASSERT_H_
#define CHRE_PLATFORM_ASSERT_H_

#include "chre/platform/log.h"

/**
 * @file
 * Includes the platform-specific header file that supplies an assertion macro.
 * The platform header must supply the following symbol as a macro or free
 * function:
 *
 *   CHRE_ASSERT(scalar expression)
 *
 * Where expression will be checked to be false (ie: compares equal to zero) and
 * terminate the program if found to be the case.
 */

#if defined(CHRE_ASSERTIONS_ENABLED)

#include "chre/target_platform/assert.h"

#ifndef CHRE_ASSERT
#error "CHRE_ASSERT must be defined"
#endif  // CHRE_ASSERT

/**
 * Provide an CHRE_ASSERT_LOG macro based on LOGE and ASSERT. This allows
 * asserting and logging in one statement.
 */
#define CHRE_ASSERT_LOG(condition, fmt, ...) do { \
  if (!condition) {                               \
    LOGE(fmt, ##__VA_ARGS__);                     \
  }                                               \
                                                  \
  CHRE_ASSERT(condition);                         \
} while (0)

#elif defined(CHRE_ASSERTIONS_DISABLED)

#define CHRE_ASSERT(condition) ((void) (condition))

#define CHRE_ASSERT_LOG(condition, fmt, ...) do { \
  CHRE_ASSERT(condition);                         \
  chreLogNull(fmt, ##__VA_ARGS__);                \
} while(0)

#else
#error "CHRE_ASSERTIONS_ENABLED or CHRE_ASSERTIONS_DISABLED must be defined"
#endif  // CHRE_ASSERTIONS_ENABLED

#endif  // CHRE_PLATFORM_ASSERT_H_
