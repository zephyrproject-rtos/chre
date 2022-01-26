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

#include <chre/nanoapp.h>

#include "chre/core/nanoapp.h"
#include "chre/util/unique_ptr.h"

namespace chre {

/**
 * First possible value for CHRE_EVENT_SIMULATION_TEST events. These events are
 * reserved for utility events that can be used by any simulation test.
 */
#define CHRE_EVENT_SIMULATION_TEST_FIRST_EVENT CHRE_EVENT_FIRST_USER_VALUE

/**
 * Produce an event ID in the block of IDs reserved for CHRE simulation test
 * events.
 *
 * @param offset Index into simulation test event ID block; valid range is [0,
 * 0xFFF].
 *
 * @defgroup CHRE_SIMULATION_TEST_EVENT_ID
 * @{
 */
#define CHRE_SIMULATION_TEST_EVENT_ID(offset) \
  (CHRE_EVENT_SIMULATION_TEST_FIRST_EVENT + (offset))

/**
 * First possible value for CHRE_EVENT_SPECIFIC_SIMULATION_TEST events. Each
 * simulation test can define specific events for its use case.
 */
#define CHRE_EVENT_SPECIFIC_SIMULATION_TEST_FIRST_EVENT \
  CHRE_EVENT_FIRST_USER_VALUE + 0x1000

/**
 * Produce an event ID in the block of IDs reserved for events belonging to a
 * specific CHRE simulation test.
 *
 * @param offset Index into the event ID block of a specific simulation test;
 * valid range is [0, 0xFFF].
 *
 * @defgroup CHRE_SIMULATION_TEST_EVENT_ID
 * @{
 */
#define CHRE_SPECIFIC_SIMULATION_TEST_EVENT_ID(offset) \
  (CHRE_EVENT_SPECIFIC_SIMULATION_TEST_FIRST_EVENT + (offset))

/**
 * @return the statically loaded nanoapp based on the arguments.
 *
 * @see chreNslNanoappInfo for param descriptions.
 */
UniquePtr<Nanoapp> createStaticNanoapp(
    const char *name, uint64_t appId, uint32_t appVersion, uint32_t appPerms,
    decltype(nanoappStart) *startFunc,
    decltype(nanoappHandleEvent) *handleEventFunc,
    decltype(nanoappEnd) *endFunc);

/**
 * Default CHRE nanoapp entry points that don't do anything.
 */
bool defaultNanoappStart();
void defaultNanoappHandleEvent(uint32_t senderInstanceId, uint16_t eventType,
                               const void *eventData);
void defaultNanoappEnd();

/**
 * Create static nanoapp and load it in CHRE.
 *
 * @see createStatic Nanoapp.
 */
void loadNanoapp(const char *name, uint64_t appId, uint32_t appVersion,
                 uint32_t appPerms, decltype(nanoappStart) *startFunc,
                 decltype(nanoappHandleEvent) *handleEventFunc,
                 decltype(nanoappEnd) *endFunc);

/**
 * Unload nanoapp corresponding to appId.
 *
 * @param appId App Id of nanoapp to be unloaded.
 */
void unloadNanoapp(uint64_t appId);

/**
 * A convenience deferred callback function that can be used to start an already
 * loaded nanoapp.
 *
 * @param type The callback type.
 * @param nanoapp A pointer to the nanoapp that is already loaded.
 */
void testFinishLoadingNanoappCallback(SystemCallbackType type,
                                      UniquePtr<Nanoapp> &&nanoapp);

/**
 * A convenience deferred callback function to unload a nanoapp.
 *
 * @param type The callback type.
 * @param data The data containing the appId.
 * @param extraData Extra data.
 */
void testFinishUnloadingNanoappCallback(uint16_t type, void *data,
                                        void *extraData);

}  // namespace chre

#endif  // CHRE_SIMULATION_TEST_UTIL_H_
