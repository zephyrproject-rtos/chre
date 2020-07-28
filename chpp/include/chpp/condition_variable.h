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
 * Each platform must supply a platform-specific platform_condition_variable.h
 * to provide definitions and a platform-specific condition_variable.c to
 * provide the implementation for the definitions in this file.
 */

#ifndef CHPP_CONDITION_VARIABLE_H_
#define CHPP_CONDITION_VARIABLE_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform-specific condition variable struct defined in the platform's
 * platform_condition_variable.h that enables the funcions defined here.
 */
struct ChppConditionVariable;

/**
 * Initializes the platform-specific ChppConditionVariable.
 */
static void chppConditionVariableInit(
    struct ChppConditionVariable *condition_variable);

/**
 * Deinitializes the platform-specific ChppConditionVariable.
 */
static void chppConditionVariableDeinit(
    struct ChppConditionVariable *condition_variable);

/**
 * Waits until signaled through chppConditionVariableSignal(). Only one entity
 * may be waiting on a condition variable at one time.
 *
 * It is expected that the provided mutex is locked before the function is
 * invoked, and otherwise results in undefined behavior.
 *
 * @param condition_variable The pointer to the condition variable.
 * @param mutex The mutex to use with this condition variable.
 *
 * @return The signal value indicated in chppConditionVariableSignal().
 */
static bool chppConditionVariableWait(
    struct ChppConditionVariable *condition_variable, struct ChppMutex *mutex);

/**
 * Signals on an entity waiting on the condition variable through
 * chppConditionVariableWait().
 *
 * @param condition_variable The pointer to the condition variable.
 */
static void chppConditionVariableSignal(
    struct ChppConditionVariable *condition_variable);

#ifdef __cplusplus
}
#endif

#include "chpp/platform/platform_condition_variable.h"

#endif  // CHPP_CONDITION_VARIABLE_H_
