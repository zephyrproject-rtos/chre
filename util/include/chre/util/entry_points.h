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

#ifndef CHRE_UTIL_ENTRY_POINTS_H_
#define CHRE_UTIL_ENTRY_POINTS_H_

namespace chre {

//! The type for the Nanoapp start function pointer.
typedef bool (NanoappStartFunction)();

//! The type of the Nanoapp handle event function pointer.
typedef void (NanoappHandleEventFunction)(uint32_t senderInstanceId,
                                          uint16_t eventType,
                                          const void *eventData);

//! The type of the Nanoapp stop function pointer.
typedef void (NanoappStopFunction)();

}  // namespace chre

#endif  // CHRE_UTIL_ENTRY_POINTS_H_
