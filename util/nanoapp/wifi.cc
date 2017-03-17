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

#include "chre/util/nanoapp/wifi.h"

#include <ctype.h>

namespace chre {

bool validateSsidIsAsciiNullTerminatedStr(const uint8_t *ssid,
                                          uint8_t ssidLen) {
  bool isAscii = false;
  for (uint8_t i = 0; i < ssidLen; i++) {
    if (ssid[i] == '\0') {
      // If this character is a null-terminator and and all characters before it
      // were ascii printable, this is a valid string.
      isAscii = true;
      break;
    } else if (!isascii(ssid[i])) {
      break;
    }
  }

  return isAscii;
}

}  // namespace chre
