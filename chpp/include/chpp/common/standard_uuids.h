/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef CHPP_STANDARD_UUIDS_H_
#define CHPP_STANDARD_UUIDS_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 *  Standard Client / Service UUID Definitions
 ***********************************************/

/**
 * WWAN
 */
#define CHPP_UUID_WWAN_STANDARD                                             \
  {                                                                         \
    0x1f, 0x56, 0x3d, 0xf2, 0x5d, 0x07, 0x49, 0x87, 0xba, 0x2b, 0xb3, 0x0e, \
        0xf2, 0x3d, 0x11, 0x28                                              \
  }

#ifdef __cplusplus
}
#endif

#endif  // CHPP_STANDARD_UUIDS_H_
