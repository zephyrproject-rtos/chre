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
 * Maximum number of services that can be registered by CHPP (not including
 * predefined services), if not defined by the build system.
 */
#ifndef CHPP_MAX_REGISTERED_SERVICES
#define CHPP_MAX_REGISTERED_SERVICES 5
#endif

/**
 * Handle Numbers in ChppAppHeader
 */
enum ChppHandleNumber {
  //! Handleless communication
  CHPP_HANDLE_NONE = 0x00,

  //! Loopback Service
  CHPP_HANDLE_LOOPBACK = 0x01,

  //! Discovery Service
  CHPP_HANDLE_DISCOVERY = 0x0F,

  //! Negotiated Services (starting from this offset)
  CHPP_HANDLE_NEGOTIATED_RANGE_START = 0x10,
};

/**
 * Message Types as used in ChppAppHeader
 */
enum ChppMessageType {
  //! Request from client. Needs response from server.
  CHPP_MESSAGE_TYPE_CLIENT_REQUEST = 0,

  //! Response from server (with the same Command and Transaction ID as the
  //! client request).
  CHPP_MESSAGE_TYPE_SERVER_RESPONSE = 1,

  //! Notification from client. Server shall not respond.
  CHPP_MESSAGE_TYPE_CLIENT_NOTIFICATION = 2,

  //! Notification from server. Client shall not respond.
  CHPP_MESSAGE_TYPE_SERVER_NOTIFICATION = 3,
};

/**
 * CHPP Application Layer header
 */
CHPP_PACKED_START
struct ChppAppHeader {
  //! Service Handle
  uint8_t handle;

  //! Message Type (request/response/notification as detailed in
  //! ChppMessageType)
  uint8_t type;

  //! Transaction ID
  uint8_t transaction;

  //! Reserved
  uint8_t reserved;

  //! Command
  uint16_t command;

} CHPP_PACKED_ATTR;
CHPP_PACKED_END

/**
 * Function type that dispatches incoming datagrams for any particular service
 */
typedef void(ChppDispatchFunction)(void *context, uint8_t *buf, size_t len);

/**
 * Length of a service UUID and its human-readable printed form in bytes
 */
#define CHPP_SERVICE_UUID_LEN 16
#define CHPP_SERVICE_UUID_STRING_LEN (16 * 2 + 4 + 1)

/**
 * Length of a version number, in bytes (major + minor + revision), per CHPP
 * spec.
 */
#define CHPP_SERVICE_VERSION_LEN (1 + 1 + 2)

/**
 * Maximum length of a human-readable service name, per CHPP spec.
 * (15 ASCII characters + null)
 */
#define CHPP_SERVICE_NAME_MAX_LEN (15 + 1)

/**
 * CHPP definition of a service descriptor as sent over the wire.
 */
CHPP_PACKED_START
struct ChppServiceDescriptor {
  //! UUID of the service.
  //! Must be generated according to RFC 4122, UUID version 4 (random).
  uint8_t uuid[CHPP_SERVICE_UUID_LEN];

  //! Human-readable name of the service for debugging.
  char name[CHPP_SERVICE_NAME_MAX_LEN];

  //! Major version of the service (breaking changes).
  uint8_t versionMajor;

  //! Minor version of the service (backwards compatible changes).
  uint8_t versionMinor;

  //! Patch version of the service (bug fixes).
  uint16_t versionPatch;
} CHPP_PACKED_ATTR;
CHPP_PACKED_END

/**
 * CHPP definition of a service as supported on a server.
 */
struct ChppService {
  //! Service Descriptor as sent over the wire.
  struct ChppServiceDescriptor descriptor;

  //! Pointer to the function that dispatches incoming client requests for the
  //! service.
  ChppDispatchFunction *requestDispatchFunctionPtr;

  //! Pointer to the function that dispatches incoming client notifications for
  //! the service.
  ChppDispatchFunction *notificationDispatchFunctionPtr;

  //! Minimum valid length of datagrams for the service.
  size_t minLength;
};

struct ChppAppState {
  struct ChppTransportState *transportContext;  // Pointing to transport context

  const struct chrePalSystemApi *systemApi;  // Pointing to the PAL system APIs

  uint8_t registeredServiceCount;  // Number of services currently registered

  const struct ChppService *registeredServices[CHPP_MAX_REGISTERED_SERVICES];

  void *registeredServiceContexts[CHPP_MAX_REGISTERED_SERVICES];
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

/**
 * Deinitializes the CHPP app layer for e.g. clean shutdown.
 *
 * @param appContext A non-null pointer to ChppAppState initialized previously
 * in chppAppInit().
 */
void chppAppDeinit(struct ChppAppState *appContext);

/*
 * Processes an Rx Datagram from the transport layer.
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
