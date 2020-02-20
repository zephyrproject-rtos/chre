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
 * Implementation Notes:
 * Each platform must supply a platform-specific platform_link.h to provide the
 * definitions and a platform-specific link.c to provide the implementation for
 * the definitions in this file.
 * The platform must also initialize the ChppPlatformLinkParameters for each
 * link (context.linkParams).
 */

#ifndef CHPP_APP_H_
#define CHPP_APP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 *  Public Definitions
 ***********************************************/

struct ChppAppState {
  struct ChppTransportState *transportContext;  // Pointing to transport context
};

/************************************************
 *  Public functions
 ***********************************************/

/*
 * Processes an Rx Datagram
 */
void chppProcessRxDatagram(struct ChppAppState *context, size_t len,
                           uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif  // CHPP_APP_H_
