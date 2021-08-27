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

#ifndef CHRE_SIMULATION_TEST_UTIL_H_
#define CHRE_SIMULATION_TEST_UTIL_H_

#include "chre/core/nanoapp.h"
#include "chre/util/entry_points.h"
#include "chre/util/unique_ptr.h"

namespace chre {

/**
 * @return the statically loaded nanoapp based on the arguments.
 */
UniquePtr<Nanoapp> createStaticNanoapp(
    const char *name, uint64_t appId, uint32_t appVersion, uint32_t appPerms,
    chreNanoappStartFunction *nanoappStart,
    chreNanoappHandleEventFunction *nanoappHandleEvent,
    chreNanoappEndFunction *nanoappEnd);

/**
 * Default CHRE nanoapp entry points that don't do anything.
 */
bool defaultNanoappStart();
void defaultNanoappHandleEvent(uint32_t senderInstanceId, uint16_t eventType,
                               const void *eventData);
void defaultNanoappEnd();

/**
 * A convenience deferred callback function that can be used to start an already
 * loaded nanoapp.
 *
 * @param type The callback type.
 * @param nanoapp A pointer to the nanoapp that is already loaded.
 */
void testFinishLoadingNanoappCallback(SystemCallbackType type,
                                      UniquePtr<Nanoapp> &&nanoapp);

}  // namespace chre

#endif  // CHRE_SIMULATION_TEST_UTIL_H_
