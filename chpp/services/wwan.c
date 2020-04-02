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

#include "chpp/services/wwan.h"

/************************************************
 *  Prototypes
 ***********************************************/

void chppDispatchWwan(struct ChppAppState *context, uint8_t *buf, size_t len);

/************************************************
 *  Private Definitions
 ***********************************************/

static const struct ChppService wwanService = {
    .descriptor.uuid = {0x0d, 0x0e, 0x0a, 0x0d, 0x0b, 0x0e, 0x0e, 0x0f, 0x0d,
                        0x0e, 0x0a, 0x0d, 0x0b, 0x0e, 0x0e, 0x0f},

    // Human-readable name
    .descriptor.name = "WWAN",

    .descriptor.versionMajor = 1,

    .descriptor.versionMinor = 0,

    .descriptor.versionPatch = 0,

    // Dispatch function pointer
    .dispatchFunctionPtr = &chppDispatchWwan,

    // Min length is the entire header
    .minLength = sizeof(struct ChppAppHeader)};

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Dispatches an Rx Datagram from the transport layer that is determined to be
 * for the WWAN service (i.e. a WWAN request from a client).
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
void chppDispatchWwan(struct ChppAppState *context, uint8_t *buf, size_t len) {
  UNUSED_VAR(context);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppWwanServiceInit(struct ChppAppState *context) {
  chppRegisterService(context, &wwanService);
}
