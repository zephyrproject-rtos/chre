/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef CHRE_CORE_HOST_NOTIFICATIONS_H_
#define CHRE_CORE_HOST_NOTIFICATIONS_H_

#include <cinttypes>

#include "chre/util/system/debug_dump.h"

namespace chre {

/**
 * Updates host endpoint [dis]connection to CHRE.
 *
 * @param hostEndpointId The host endpoint ID.
 */
void postHostEndpointConnected(uint16_t hostEndpointId);
void postHostEndpointDisconnected(uint16_t hostEndpointId);

}  // namespace chre

#endif  // CHRE_CORE_HOST_NOTIFICATIONS_H_
