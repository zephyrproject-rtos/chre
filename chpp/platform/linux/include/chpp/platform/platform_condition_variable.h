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

#ifndef CHPP_PLATFORM_CONDITION_VARIABLE_H_
#define CHPP_PLATFORM_CONDITION_VARIABLE_H_

#include <pthread.h>
#include <stdbool.h>

#include "chpp/mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ChppConditionVariable {
  pthread_cond_t cond;  // Condition variable
};

static inline void chppConditionVariableInit(
    struct ChppConditionVariable *condition_variable) {
  pthread_cond_init(&condition_variable->cond, NULL);
}

static inline void chppConditionVariableDeinit(
    struct ChppConditionVariable *condition_variable) {
  pthread_cond_destroy(&condition_variable->cond);
}

static inline bool chppConditionVariableWait(
    struct ChppConditionVariable *condition_variable, struct ChppMutex *mutex) {
  return pthread_cond_wait(&condition_variable->cond, &mutex->lock) == 0;
}

static inline void chppConditionVariableSignal(
    struct ChppConditionVariable *condition_variable) {
  pthread_cond_signal(&condition_variable->cond);
}

#ifdef __cplusplus
}
#endif

#endif  // CHPP_PLATFORM_CONDITION_VARIABLE_H_
