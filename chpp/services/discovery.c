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

#include "chpp/services/discovery.h"

/************************************************
 *  Prototypes
 ***********************************************/

void chppDiscoveryDiscoverAll(struct ChppAppState *context,
                              uint8_t transaction);

/************************************************
 *  Private Definitions
 ***********************************************/

/**
 * Commands used by the Discovery Service
 */
enum ChppDiscoveryCommands {
  // Discover all services.
  CHPP_DISCOVERY_COMMAND_DISCOVER_ALL = 0x0001,
};

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Processes the Discover All Services command (0x0001)
 *
 * @param context Maintains status for each app layer instance.
 * @param transaction Transaction ID (to be included in the response).
 */
void chppDiscoveryDiscoverAll(struct ChppAppState *context,
                              uint8_t transaction) {
  // TODO

  UNUSED_VAR(context);
  UNUSED_VAR(transaction);
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppDispatchDiscovery(struct ChppAppState *context, uint8_t *buf,
                           size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  uint8_t transaction = rxHeader->transaction;
  uint16_t command = rxHeader->command;

  switch (rxHeader->type) {
    case CHPP_MESSAGE_TYPE_CLIENT_REQUEST:
      // Discovery request from client.

      if (command == CHPP_DISCOVERY_COMMAND_DISCOVER_ALL) {
        // Send back a list of services supported by this platform.
        chppDiscoveryDiscoverAll(context, transaction);

      } else {
        LOGE("Received unknown discovery command: %#x, transaction = %d",
             command, transaction);
      }
      break;

    case CHPP_MESSAGE_TYPE_SERVER_RESPONSE:
      // Received discovery response from server.

      // TODO: Register client for discovered services
      UNUSED_VAR(len);

      break;

    default:
      LOGE(
          "Received unknown discovery message type: %#x, command = %#x, "
          "transaction = %d",
          rxHeader->type, command, transaction);
  }
}
