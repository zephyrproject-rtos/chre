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

#ifndef CHRE_UTIL_ID_FROM_STRING_H_
#define CHRE_UTIL_ID_FROM_STRING_H_

/**
 * Formats a string into a CHRE platform version uint64_t.
 *
 * @param the string to format
 * @return the formatted platform version id
 */
constexpr uint64_t createIdFromString(const char str[5]);

#include "chre/util/id_from_string_impl.h"

#endif  // CHRE_UTIL_ID_FROM_STRING_H_
