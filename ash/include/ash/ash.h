/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef ASH_ASH_H_
#define ASH_ASH_H_

/**
 * @file
 * Defines the interface for the Android Sensor Hub support.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialzes the ASH API.
 *
 * This API should be called in the CHRE initialization process before event
 * loops start.
 */
void ashInit();

/**
 * Deinitiazes the ASH API.
 *
 * This API should be called in the CHRE deinitialization process after event
 * loops stopped.
 */
void ashDeinit();

#ifdef __cplusplus
}
#endif

#endif  // ASH_ASH_H_
