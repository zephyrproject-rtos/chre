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

#include "chpp/clients/discovery.h"

/************************************************
 *  Prototypes
 ***********************************************/

void chppDiscoveryProcessDiscoverAll(struct ChppAppState *context,
                                     const uint8_t *buf, size_t len);

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Processes the Discover All Services (0x0001) response.
 *
 * @param context Maintains status for each app layer instance.
 * @param buf Input (request) datagram. Cannot be null.
 * @param len Length of input data in bytes.
 */
void chppDiscoveryProcessDiscoverAll(struct ChppAppState *context,
                                     const uint8_t *buf, size_t len) {
  // TODO

  UNUSED_VAR(context);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);
}

/************************************************
 *  Public Functions
 ***********************************************/

bool chppDispatchDiscoveryServiceResponse(struct ChppAppState *context,
                                          const uint8_t *buf, size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  bool success = true;

  switch (rxHeader->command) {
    case CHPP_DISCOVERY_COMMAND_DISCOVER_ALL: {
      chppDiscoveryProcessDiscoverAll(context, buf, len);
      break;
    }
    default: {
      success = false;
      break;
    }
  }
  return success;
}
