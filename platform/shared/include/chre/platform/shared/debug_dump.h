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

#ifndef CHRE_PLATFORM_SHARED_DEBUG_DUMP_H_
#define CHRE_PLATFORM_SHARED_DEBUG_DUMP_H_

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

namespace chre {

/**
 * Platform implementation of chreDebugDumpLog.
 *
 * This function implements chreDebugDumpLog, and is called through either the
 * CHRE API or the nanoapp support library. In addition, it validates the
 * nanoapp is handling CHRE_EVENT_DEBUG_DUMP before logging any debug data.
 *
 * @see chreDebugDumpLog
 */
void platformDso_chreDebugDumpVaLog(const char *formatStr, va_list args);

/**
 * Platform implementation of platformDso_chreDebugDumpVaLog.
 *
 * This function is only called through by platformDso_chreDebugDumpVaLog. It
 * only contains debug data logging implementation, but not validation logic.
 *
 * @see chreDebugDumpLog
 * @see platformDso_chreDebugDumpVaLog
 */
void platform_chreDebugDumpVaLog(const char *formatStr, va_list args);

}  // namespace chre

#ifdef __cplusplus
}
#endif

#endif  // CHRE_PLATFORM_SHARED_DEBUG_DUMP_H_
