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

#include "chpp/link.h"

#include <stddef.h>
#include <stdint.h>

#include "chpp/macros.h"

void chppPlatformLinkInit(struct ChppPlatformLinkParameters *params) {
  UNUSED_VAR(params);
}

void chppPlatformLinkDeinit(struct ChppPlatformLinkParameters *params) {
  UNUSED_VAR(params);
}

enum ChppLinkErrorCode chppPlatformLinkSend(
    struct ChppPlatformLinkParameters *params, uint8_t *buf, size_t len) {
  // TODO
  UNUSED_VAR(params);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);
  return CHPP_LINK_ERROR_NONE_SENT;
}

void chppPlatformLinkDoWork(struct ChppPlatformLinkParameters *params,
                            uint32_t signal) {
  UNUSED_VAR(params);
  UNUSED_VAR(signal);
}

void chppPlatformLinkReset(struct ChppPlatformLinkParameters *params) {
  // TODO
  UNUSED_VAR(params);
}
