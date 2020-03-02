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
#include <string.h>

#include "chpp/macros.h"
#include "chpp/memory.h"
#include "chpp/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 *  Public Definitions
 ***********************************************/

/**
 * Loopback functionality.
 */
#define CHPP_DATAGRAM_HANDLE_LOOPBACK 0x01
#define CHPP_DATAGRAM_TYPE_CLIENT_REQUEST 0x00
#define CHPP_DATAGRAM_TYPE_SERVER_RESPONSE 0x01

struct ChppAppState {
  struct ChppTransportState *transportContext;  // Pointing to transport context
};

/************************************************
 *  Public functions
 ***********************************************/

/**
 * Initializes the CHPP app layer state stored in the parameter appContext.
 * It is necessary to initialize state for each app layer instance on
 * every platform.
 *
 * @param appContext Maintains status for each app layer instance.
 * @param transportContext The transport layer status struct associated with
 * this app layer instance.
 */
void chppAppInit(struct ChppAppState *appContext,
                 struct ChppTransportState *transportContext);

/*
 * Processes an Rx Datagram from the transport layer. It is up to the app layer
 * to call chppAppProcessDoneCb() ASAP when it is done with buf (so it is
 * properly freed, etc.)
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
void chppProcessRxDatagram(struct ChppAppState *context, uint8_t *buf,
                           size_t len);

#ifdef __cplusplus
}
#endif

#endif  // CHPP_APP_H_
