/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "chre/platform/slpi/debug_dump.h"

#include "chre/platform/log.h"
#include "chre/util/macros.h"

// Some devices don't have ash/debug.h implemented for them so allow swapping
// out that implementation with an empty one until an implementation can be
// provided for all devices.
#ifdef CHRE_ENABLE_ASH_DEBUG_DUMP
#include "ash/debug.h"
#else  // CHRE_ENABLE_ASH_DEBUG_DUMP
#include <cstring>
#endif  // CHRE_ENABLE_ASH_DEBUG_DUMP

namespace chre {
namespace {
#ifndef CHRE_ENABLE_ASH_DEBUG_DUMP
//! State information for the debug dump callback provided by CHRE.
struct {
  //! Provided in registerDebugDumpCallback() and used to request a debug
  //! dump from CHRE
  debugDumpCbFunc *callback;

  //! Arbitrary pointer to pass to the callback
  void *cookie;
} gDumpCallback;

//! State information for an in-progress debug dump
struct {
  //! Provided in triggerDebugDump() and used to report the output of the
  //! CHRE debug dump
  debugDumpReadyCbFunc *callback;

  //! Arbitrary pointer to pass to the callback
  void *cookie;

  //! Indicates whether the debug dump has completed
  bool done = true;
} gDebugDumpState;
#endif  // CHRE_ENABLE_ASH_DEBUG_DUMP
}  // namespace

#ifdef CHRE_ENABLE_ASH_DEBUG_DUMP
size_t debugDumpStrMaxSize = ASH_DEBUG_DUMP_STR_MAX_SIZE;
#else   // CHRE_ENABLE_ASH_DEBUG_DUMP
size_t debugDumpStrMaxSize = CHRE_MESSAGE_TO_HOST_MAX_SIZE;
#endif  // CHRE_ENABLE_ASH_DEBUG_DUMP

bool registerDebugDumpCallback(const char *name, debugDumpCbFunc *callback,
                               void *cookie) {
#ifdef CHRE_ENABLE_ASH_DEBUG_DUMP
  return ashRegisterDebugDumpCallback(name, callback, cookie);
#else   // CHRE_ENABLE_ASH_DEBUG_DUMP
  UNUSED_VAR(name);
  gDumpCallback.callback = callback;
  gDumpCallback.cookie = cookie;
  return true;
#endif  // CHRE_ENABLE_ASH_DEBUG_DUMP
}

void unregisterDebugDumpCallback(debugDumpCbFunc *callback) {
#ifdef CHRE_ENABLE_ASH_DEBUG_DUMP
  ashUnregisterDebugDumpCallback(callback);
#else   // CHRE_ENABLE_ASH_DEBUG_DUMP
  UNUSED_VAR(callback);
  gDumpCallback.callback = nullptr;
  gDumpCallback.cookie = nullptr;
#endif  // CHRE_ENABLE_ASH_DEBUG_DUMP
}

bool commitDebugDump(uint32_t handle, const char *debugStr, bool done) {
#ifdef CHRE_ENABLE_ASH_DEBUG_DUMP
  return ashCommitDebugDump(handle, debugStr, done);
#else   // CHRE_ENABLE_ASH_DEBUG_DUMP
  bool success = false;
  if (handle != 0) {
    LOGE("CHRE debug dump only supports a single debug dump callback");
  } else if (gDebugDumpState.done) {
    LOGE("CHRE debug dump already finished");
  } else if (gDebugDumpState.callback != nullptr) {
    gDebugDumpState.callback(gDebugDumpState.cookie, debugStr, strlen(debugStr),
                             done);
    gDebugDumpState.done = done;
    success = true;
  }
  return success;
#endif  // CHRE_ENABLE_ASH_DEBUG_DUMP
}

bool triggerDebugDump(debugDumpReadyCbFunc *readyCb, void *cookie) {
#ifdef CHRE_ENABLE_ASH_DEBUG_DUMP
  return ashTriggerDebugDump(readyCb, cookie);
#else   // CHRE_ENABLE_ASH_DEBUG_DUMP
  if (gDumpCallback.callback != nullptr) {
    gDebugDumpState.callback = readyCb;
    gDebugDumpState.cookie = cookie;
    gDebugDumpState.done = false;
    gDumpCallback.callback(gDumpCallback.cookie, 0 /* handle */);
  }
  return true;
#endif  // CHRE_ENABLE_ASH_DEBUG_DUMP
}

}  // namespace chre
