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

/*
 * Implementation Notes
 * Each platform must supply a platform-specific platform_notifier.h to provide
 * definitions and a platform-specific notifier.c to provide the implementation
 * for the definitions in this file.
 */

#ifndef CHPP_SYNC_H_
#define CHPP_SYNC_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform-specific condition variable struct that enables the
 * platform-specific funcions defined here.
 */
struct ChppNotifier;

/**
 * Initializes the platform-specific ChppNotifier.
 * Will be called before notifier is used anywhere else.
 *
 * @param notifier Points to the ChppNotifier being initialized.
 */
static void chppNotifierInit(struct ChppNotifier *notifier);

/**
 * Waits on a platform-specific notifier until it is signaled through
 * chppNotifierEvent() or through chppNotifierExit().
 *
 * @param notifier Points to the ChppNotifier being used.
 *
 * @return True indicates that it has been signaled through chppNotifierEvent().
 * False indicates that it has been signaled through chppNotifierEvent(),
 * indicating an intent to exit.
 */
static bool chppNotifierWait(struct ChppNotifier *notifier);

/**
 * Signals chppNotifierWait() with a return value of True.
 *
 * @param notifier Points to the ChppNotifier being used.
 */
static void chppNotifierEvent(struct ChppNotifier *notifier);

/**
 * Signals chppNotifierWait() with a return value of False, i.e. indicating an
 * intent to exit.
 *
 * @param notifier Points to the ChppNotifier being used.
 */
static void chppNotifierExit(struct ChppNotifier *notifier);

#ifdef __cplusplus
}
#endif

#include "chpp/platform/platform_notifier.h"

#endif  // CHPP_SYNC_H_
