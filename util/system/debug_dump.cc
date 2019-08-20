/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "chre/util/system/debug_dump.h"

#include <cstdio>

#include "chre/platform/log.h"

namespace chre {

void DebugDumpWrapper::print(const char *formatStr, ...) {
  va_list argList;
  size_t sizeOfStr;
  va_start(argList, formatStr);
  if (!sizeOfFormattedString(formatStr, argList, &sizeOfStr)) {
    va_end(argList);
    LOGE("Error getting string size while debug dump printing");
  } else {
    va_end(argList);
    if (sizeOfStr >= kBuffSize) {
      LOGE(
          "String was too large to fit in a single buffer for debug dump"
          " print");
    } else if ((mCurrBuff == nullptr || sizeOfStr >= kBuffSize - mBuffPos) &&
               !allocNewBuffer()) {
      LOGE("Error allocating buffer in debug dump print");
    } else {
      // String fits into current buffer or created a new buffer successfully
      va_start(argList, formatStr);
      if (!insertString(formatStr, argList)) {
        LOGE("Error inserting string into buffer in debug dump");
      }
      va_end(argList);
    }
  }
}

bool DebugDumpWrapper::allocNewBuffer() {
  mCurrBuff = static_cast<char *>(memoryAlloc(kBuffSize));
  if (mCurrBuff == nullptr) {
    LOG_OOM();
  } else {
    mBuffers.emplace_back(mCurrBuff);
    mBuffPos = 0;
    mCurrBuff[0] = '\0';
  }
  return mCurrBuff != nullptr;
}

bool DebugDumpWrapper::insertString(const char *formatStr, va_list argList) {
  CHRE_ASSERT(mCurrBuff != nullptr);
  int strLen =
      vsnprintf(&mCurrBuff[mBuffPos], kBuffSize - mBuffPos, formatStr, argList);
  if (strLen >= 0) {
    mBuffPos += static_cast<size_t>(strLen);
  }
  return strLen >= 0;
}

bool DebugDumpWrapper::sizeOfFormattedString(const char *formatStr,
                                             va_list argList,
                                             size_t *sizeOfStr) {
  bool success = true;
  int strLen = vsnprintf(nullptr, 0, formatStr, argList);
  if (strLen < 0) {
    success = false;
  } else {
    *sizeOfStr = static_cast<size_t>(strLen);
  }
  return success;
}

}  // namespace chre
