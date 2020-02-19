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

#ifndef CHPP_PLATFORM_SYNC_H_
#define CHPP_PLATFORM_SYNC_H_

#include <pthread.h>

#include "chpp/mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ChppNotifier {
  pthread_cond_t cond;     // Condition variable
  struct ChppMutex mutex;  // Platform-specific mutex
  bool signaled;           // Whether a notification has occurred
  bool shouldExit;         // Whether the thread should exit
};

/**
 * Platform implementation of chppNotifierInit()
 */
void chppPlatformNotifierInit(struct ChppNotifier *notifier);

/**
 * Platform implementation of chppNotifierWait()
 */
bool chppPlatformNotifierWait(struct ChppNotifier *notifier);

/**
 * Platform implementation of chppNotifierEvent()
 */
void chppPlatformNotifierEvent(struct ChppNotifier *notifier);

/**
 * Platform implementation of chppNotifierExit()
 */
void chppPlatformNotifierExit(struct ChppNotifier *notifier);

static inline void chppNotifierInit(struct ChppNotifier *notifier) {
  chppPlatformNotifierInit(notifier);
}

static inline bool chppNotifierWait(struct ChppNotifier *notifier) {
  return chppPlatformNotifierWait(notifier);
}

static inline void chppNotifierEvent(struct ChppNotifier *notifier) {
  chppPlatformNotifierEvent(notifier);
}

static inline void chppNotifierExit(struct ChppNotifier *notifier) {
  chppPlatformNotifierExit(notifier);
}

#ifdef __cplusplus
}
#endif

#endif  // CHPP_PLATFORM_SYNC_H_
