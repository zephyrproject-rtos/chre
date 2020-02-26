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

#include <stdbool.h>

#include "chpp/macros.h"
#include "chpp/memory.h"
#include "chpp/notifier.h"

void chppPlatformNotifierInit(struct ChppNotifier *notifier) {
  chppMutexInit(&notifier->mutex);
  pthread_cond_init(&notifier->cond, NULL);
}

bool chppPlatformNotifierWait(struct ChppNotifier *notifier) {
  chppMutexLock(&notifier->mutex);

  while (notifier->signaled == false && notifier->shouldExit == false) {
    pthread_cond_wait(&notifier->cond, &notifier->mutex.lock);
  }
  notifier->signaled = false;

  chppMutexUnlock(&notifier->mutex);
  return !notifier->shouldExit;
}

void chppPlatformNotifierEvent(struct ChppNotifier *notifier) {
  chppMutexLock(&notifier->mutex);

  notifier->signaled = true;
  pthread_cond_signal(&notifier->cond);

  chppMutexUnlock(&notifier->mutex);
}

void chppPlatformNotifierExit(struct ChppNotifier *notifier) {
  chppMutexLock(&notifier->mutex);

  notifier->shouldExit = true;
  pthread_cond_signal(&notifier->cond);

  chppMutexUnlock(&notifier->mutex);
}
