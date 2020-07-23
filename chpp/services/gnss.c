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

#include "chpp/services/gnss.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include "chpp/common/gnss.h"
#include "chpp/common/standard_uuids.h"
#include "chpp/macros.h"
#include "chpp/platform/log.h"
#include "chpp/services.h"
#include "chpp/services/gnss_types.h"
#include "chre/pal/gnss.h"

/************************************************
 *  Prototypes
 ***********************************************/

static bool chppDispatchGnssRequest(void *serviceContext, uint8_t *buf,
                                    size_t len);

/************************************************
 *  Private Definitions
 ***********************************************/

/**
 * Configuration parameters for this service
 */
static const struct ChppService kGnssServiceConfig = {
    .descriptor.uuid = CHPP_UUID_GNSS_STANDARD,

    // Human-readable name
    .descriptor.name = "GNSS",

    // Version
    .descriptor.version.major = 1,
    .descriptor.version.minor = 0,
    .descriptor.version.patch = 0,

    // Client request dispatch function pointer
    .requestDispatchFunctionPtr = &chppDispatchGnssRequest,

    // Client notification dispatch function pointer
    .notificationDispatchFunctionPtr = NULL,  // Not supported

    // Min length is the entire header
    .minLength = sizeof(struct ChppAppHeader),
};

/**
 * Structure to maintain state for the GNSS service and its Request/Response
 * (RR) functionality.
 */
struct ChppGnssServiceState {
  struct ChppServiceState service;   // GNSS service state
  const struct chrePalGnssApi *api;  // GNSS PAL API

  // Based on chre/pal/gnss.h and chrePalGnssApi
  struct ChppRequestResponseState open;             // Service init state
  struct ChppRequestResponseState close;            // Service deinit state
  struct ChppRequestResponseState getCapabilities;  // Get Capabilities state
  struct ChppRequestResponseState
      controlLocationSession;  // Control Location measurement state
  struct ChppRequestResponseState
      controlMeasurementSession;  // Control Raw GNSS measurement state
  struct ChppRequestResponseState
      configurePassiveLocationListener;  // Configure Passive location receiving
                                         // state
};

CHPP_PACKED_START

//! Parameters for controlLocationSession
// TODO: Replace with auto-generated parser function when available
struct ChppGnssControlLocationSessionParameters {
  bool enable;
  uint32_t minIntervalMs;
  uint32_t minTimeToNextFixMs;
} CHPP_PACKED_ATTR;

//! Parameters for controlMeasurementSession
// TODO: Replace with auto-generated parser function when available
struct ChppGnssControlMeasurementSessionParameters {
  bool enable;
  uint32_t minIntervalMs;
} CHPP_PACKED_ATTR;

CHPP_PACKED_END

// Note: The CHRE PAL API only allows for one definition - see comment in WWAN
// service for details.
// Note: There is no notion of a cookie in the CHRE GNSS API so we need to use
// the global service state (gGnssServiceContext) directly in all callbacks.
struct ChppGnssServiceState gGnssServiceContext;

/************************************************
 *  Prototypes
 ***********************************************/

static bool chppDispatchGnssRequest(void *serviceContext, uint8_t *buf,
                                    size_t len);

static void chppGnssServiceOpen(struct ChppGnssServiceState *gnssServiceContext,
                                struct ChppAppHeader *requestHeader);
static void chppGnssServiceClose(
    struct ChppGnssServiceState *gnssServiceContext,
    struct ChppAppHeader *requestHeader);

static void chppGnssServiceGetCapabilities(
    struct ChppGnssServiceState *gnssServiceContext,
    struct ChppAppHeader *requestHeader);
static void chppGnssServiceControlLocationSession(
    struct ChppGnssServiceState *gnssServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len);
static void chppGnssServiceControlMeasurementSession(
    struct ChppGnssServiceState *gnssServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len);
static void chppGnssServiceConfigurePassiveLocationListener(
    struct ChppGnssServiceState *gnssServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len);

static void chppGnssServiceRequestStateResyncCallback(void);
static void chppGnssServiceLocationStatusChangeCallback(bool enabled,
                                                        uint8_t errorCode);
static void chppGnssServiceLocationEventCallback(
    struct chreGnssLocationEvent *event);
static void chppGnssServiceMeasurementStatusChangeCallback(bool enabled,
                                                           uint8_t errorCode);
static void chppGnssServiceMeasurementEventCallback(
    struct chreGnssDataEvent *event);

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Dispatches a client request from the transport layer that is determined to be
 * for the GNSS service.
 *
 * This function is called from the app layer using its function pointer given
 * during service registration.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 *
 * @return False indicates error (unknown command).
 */
static bool chppDispatchGnssRequest(void *serviceContext, uint8_t *buf,
                                    size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  buf += sizeof(struct ChppAppHeader);
  len -= sizeof(struct ChppAppHeader);

  struct ChppGnssServiceState *gnssServiceContext =
      (struct ChppGnssServiceState *)serviceContext;
  bool success = true;

  switch (rxHeader->command) {
    case CHPP_GNSS_OPEN: {
      chppServiceTimestampRequest(&gnssServiceContext->open, rxHeader);
      chppGnssServiceOpen(gnssServiceContext, rxHeader);
      break;
    }

    case CHPP_GNSS_CLOSE: {
      chppServiceTimestampRequest(&gnssServiceContext->close, rxHeader);
      chppGnssServiceClose(gnssServiceContext, rxHeader);
      break;
    }

    case CHPP_GNSS_GET_CAPABILITIES: {
      chppServiceTimestampRequest(&gnssServiceContext->getCapabilities,
                                  rxHeader);
      chppGnssServiceGetCapabilities(gnssServiceContext, rxHeader);
      break;
    }

    case CHPP_GNSS_CONTROL_LOCATION_SESSION: {
      chppServiceTimestampRequest(&gnssServiceContext->controlLocationSession,
                                  rxHeader);
      chppGnssServiceControlLocationSession(gnssServiceContext, rxHeader, buf,
                                            len);
      break;
    }

    case CHPP_GNSS_CONTROL_MEASUREMENT_SESSION: {
      chppServiceTimestampRequest(
          &gnssServiceContext->controlMeasurementSession, rxHeader);
      chppGnssServiceControlMeasurementSession(gnssServiceContext, rxHeader,
                                               buf, len);
      break;
    }

    case CHPP_GNSS_CONFIGURE_PASSIVE_LOCATION_LISTENER: {
      chppServiceTimestampRequest(
          &gnssServiceContext->configurePassiveLocationListener, rxHeader);
      chppGnssServiceConfigurePassiveLocationListener(gnssServiceContext,
                                                      rxHeader, buf, len);
      break;
    }

    default: {
      success = false;
      break;
    }
  }

  return success;
}

/**
 * Initializes the GNSS service upon an open request from the client and
 * responds to the client with the result.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 */
static void chppGnssServiceOpen(struct ChppGnssServiceState *gnssServiceContext,
                                struct ChppAppHeader *requestHeader) {
  // TODO: Check for OOM here and elsewhere
  struct ChppAppHeader *response =
      chppAllocServiceResponseFixed(requestHeader, struct ChppAppHeader);

  static const struct chrePalGnssCallbacks palCallbacks = {
      .requestStateResync = chppGnssServiceRequestStateResyncCallback,
      .locationStatusChangeCallback =
          chppGnssServiceLocationStatusChangeCallback,
      .locationEventCallback = chppGnssServiceLocationEventCallback,
      .measurementStatusChangeCallback =
          chppGnssServiceMeasurementStatusChangeCallback,
      .measurementEventCallback = chppGnssServiceMeasurementEventCallback,
  };

  if (!gnssServiceContext->api->open(
          gnssServiceContext->service.appContext->systemApi, &palCallbacks)) {
    CHPP_LOGE("GNSS PAL API initialization failed");
    CHPP_DEBUG_ASSERT(false);
    response->error = CHPP_APP_ERROR_UNSPECIFIED;
  } else {
    response->error = CHPP_APP_ERROR_NONE;
  }

  chppSendTimestampedResponseOrFail(&gnssServiceContext->service,
                                    &gnssServiceContext->open, response,
                                    sizeof(*response));
}

/**
 * Deinitializes the GNSS service.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 */
static void chppGnssServiceClose(
    struct ChppGnssServiceState *gnssServiceContext,
    struct ChppAppHeader *requestHeader) {
  struct ChppAppHeader *response =
      chppAllocServiceResponseFixed(requestHeader, struct ChppAppHeader);

  gnssServiceContext->api->close();

  response->error = CHPP_APP_ERROR_NONE;
  chppSendTimestampedResponseOrFail(&gnssServiceContext->service,
                                    &gnssServiceContext->close, response,
                                    sizeof(*response));
}

/**
 * Retrieves a set of flags indicating the GNSS features supported by the
 * current implementation.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 */
static void chppGnssServiceGetCapabilities(
    struct ChppGnssServiceState *gnssServiceContext,
    struct ChppAppHeader *requestHeader) {
  struct ChppGnssGetCapabilitiesResponse *response =
      chppAllocServiceResponseFixed(requestHeader,
                                    struct ChppGnssGetCapabilitiesResponse);

  response->capabilities = gnssServiceContext->api->getCapabilities();
  response->header.error = CHPP_APP_ERROR_NONE;

  CHPP_LOGD("chppGnssServiceGetCapabilities returning %" PRIx32 ", %zu bytes",
            response->capabilities, sizeof(*response));

  chppSendTimestampedResponseOrFail(&gnssServiceContext->service,
                                    &gnssServiceContext->getCapabilities,
                                    response, sizeof(*response));
}

/**
 * Start/stop/modify the GNSS location session.
 *
 * This function returns an error code synchronously. A subsequent call to
 * chppGnssServiceLocationEventCallback() will be used to communicate the
 * location fixes.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppGnssServiceControlLocationSession(
    struct ChppGnssServiceState *gnssServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len) {
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  if (len < sizeof(struct ChppGnssControlLocationSessionParameters)) {
    error = CHPP_APP_ERROR_INVALID_ARG;

  } else {
    struct ChppGnssControlLocationSessionParameters *parameters =
        (struct ChppGnssControlLocationSessionParameters *)buf;

    if (!gnssServiceContext->api->controlLocationSession(
            parameters->enable, parameters->minIntervalMs,
            parameters->minTimeToNextFixMs)) {
      error = CHPP_APP_ERROR_UNSPECIFIED;
    }
  }

  // TODO: Consolidate to avoid duplication
  if (error != CHPP_APP_ERROR_NONE) {
    // Error occurred, send a synchronous error response
    struct ChppAppHeader *response =
        chppAllocServiceResponseFixed(requestHeader, struct ChppAppHeader);
    response->error = error;
    chppSendTimestampedResponseOrFail(
        &gnssServiceContext->service,
        &gnssServiceContext->controlLocationSession, response,
        sizeof(*response));
  }
}

/**
 * Start/stop/modify the raw GNSS measurement session.
 *
 * This function returns an error code synchronously. A subsequent call to
 * chppGnssServiceMeasurementEventCallback() will be used to communicate the
 * location fixes.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppGnssServiceControlMeasurementSession(
    struct ChppGnssServiceState *gnssServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len) {
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  if (len < sizeof(struct ChppGnssControlMeasurementSessionParameters)) {
    error = CHPP_APP_ERROR_INVALID_ARG;

  } else {
    struct ChppGnssControlMeasurementSessionParameters *parameters =
        (struct ChppGnssControlMeasurementSessionParameters *)buf;

    if (!gnssServiceContext->api->controlMeasurementSession(
            parameters->enable, parameters->minIntervalMs)) {
      error = CHPP_APP_ERROR_UNSPECIFIED;
    }
  }

  // TODO: Consolidate to avoid duplication
  if (error != CHPP_APP_ERROR_NONE) {
    // Error occurred, send a synchronous error response
    struct ChppAppHeader *response =
        chppAllocServiceResponseFixed(requestHeader, struct ChppAppHeader);
    response->error = error;
    chppSendTimestampedResponseOrFail(
        &gnssServiceContext->service,
        &gnssServiceContext->controlMeasurementSession, response,
        sizeof(*response));
  }
}

/**
 * Configures whether to opportunistically deliver any location fixes produced
 * for other clients of the GNSS engine.
 *
 * This function returns an error code synchronously. A subsequent call to
 * chppGnssServiceLocationEventCallback() will be used to communicate the
 * location fixes.
 *
 * @param serviceContext Maintains status for each service instance.
 * @param requestHeader App layer header of the request.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppGnssServiceConfigurePassiveLocationListener(
    struct ChppGnssServiceState *gnssServiceContext,
    struct ChppAppHeader *requestHeader, uint8_t *buf, size_t len) {
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  if (len < sizeof(bool)) {
    error = CHPP_APP_ERROR_INVALID_ARG;
  } else {
    bool *enable = (bool *)buf;
    if (!gnssServiceContext->api->configurePassiveLocationListener(*enable)) {
      error = CHPP_APP_ERROR_UNSPECIFIED;
    }
  }

  // TODO: Consolidate to avoid duplication
  if (error != CHPP_APP_ERROR_NONE) {
    // Error occurred, send a synchronous error response
    struct ChppAppHeader *response =
        chppAllocServiceResponseFixed(requestHeader, struct ChppAppHeader);
    response->error = error;
    chppSendTimestampedResponseOrFail(
        &gnssServiceContext->service,
        &gnssServiceContext->configurePassiveLocationListener, response,
        sizeof(*response));
  }
}

/**
 * GNSS PAL callback to request that the core CHRE system re-send requests for
 * any active sessions and its current passive location listener setting.
 */
static void chppGnssServiceRequestStateResyncCallback(void) {
  struct ChppAppHeader *notification =
      chppAllocServiceNotificationFixed(struct ChppAppHeader);

  if (notification == NULL) {
    CHPP_LOG_OOM();
    CHPP_ASSERT(false);

  } else {
    notification->handle = gGnssServiceContext.service.handle;
    notification->command = CHPP_GNSS_REQUEST_STATE_RESYNC_NOTIFICATION;

    chppEnqueueTxDatagramOrFail(
        gGnssServiceContext.service.appContext->transportContext, notification,
        sizeof(struct ChppAppHeader));
  }
}

/**
 * GNSS PAL callback to inform the CHRE of the result of changes to the location
 * session status.
 */
static void chppGnssServiceLocationStatusChangeCallback(bool enabled,
                                                        uint8_t errorCode) {
  // Recreate request header
  struct ChppAppHeader requestHeader = {
      .handle = gGnssServiceContext.service.handle,
      .transaction = gGnssServiceContext.controlLocationSession.transaction,
      .command = CHPP_GNSS_CONTROL_LOCATION_SESSION,
  };

  struct ChppGnssControlLocationSessionResponse *response =
      chppAllocServiceResponseFixed(
          &requestHeader, struct ChppGnssControlLocationSessionResponse);

  if (response == NULL) {
    CHPP_LOG_OOM();
    CHPP_ASSERT(false);

  } else {
    response->enabled = enabled;
    response->errorCode = errorCode;

    chppSendTimestampedResponseOrFail(
        &gGnssServiceContext.service,
        &gGnssServiceContext.controlLocationSession, response,
        sizeof(struct ChppGnssControlLocationSessionResponse));
  }
}

/**
 * GNSS PAL callback to pass GNSS location fixes to the core CHRE system.
 */
static void chppGnssServiceLocationEventCallback(
    struct chreGnssLocationEvent *event) {
  // Craft response per parser script
  struct ChppGnssLocationEventWithHeader *notification;
  size_t notificationLen;
  if (!chppGnssLocationEventFromChre(event, &notification, &notificationLen)) {
    CHPP_LOGE("chppGnssLocationEventFromChre failed (OOM?)");
    // TODO: consider sending an error response if this fails

  } else {
    notification->header.handle = gGnssServiceContext.service.handle;
    notification->header.type = CHPP_MESSAGE_TYPE_SERVICE_NOTIFICATION;
    notification->header.transaction =
        0;  // Because we don't know this is in response to a Location Session
            // or Passive Location Listener
    notification->header.error = CHPP_APP_ERROR_NONE;
    notification->header.command = CHPP_GNSS_LOCATION_RESULT_NOTIFICATION;

    chppEnqueueTxDatagramOrFail(
        gGnssServiceContext.service.appContext->transportContext, notification,
        notificationLen);
  }

  gGnssServiceContext.api->releaseLocationEvent(event);
}

/**
 * GNSS PAL callback to inform the CHRE of the result of changes to the raw GNSS
 * measurement session status.
 */
static void chppGnssServiceMeasurementStatusChangeCallback(bool enabled,
                                                           uint8_t errorCode) {
  // Recreate request header
  struct ChppAppHeader requestHeader = {
      .handle = gGnssServiceContext.service.handle,
      .transaction = gGnssServiceContext.controlMeasurementSession.transaction,
      .command = CHPP_GNSS_CONTROL_MEASUREMENT_SESSION,
  };

  struct ChppGnssControlMeasurementSessionResponse *response =
      chppAllocServiceResponseFixed(
          &requestHeader, struct ChppGnssControlMeasurementSessionResponse);

  if (response == NULL) {
    CHPP_LOG_OOM();
    CHPP_ASSERT(false);

  } else {
    response->enabled = enabled;
    response->errorCode = errorCode;

    chppSendTimestampedResponseOrFail(
        &gGnssServiceContext.service,
        &gGnssServiceContext.controlMeasurementSession, response,
        sizeof(struct ChppGnssControlMeasurementSessionResponse));
  }
}

/**
 * GNSS PAL callback to pass raw GNSS measurement data to the core CHRE system.
 */
static void chppGnssServiceMeasurementEventCallback(
    struct chreGnssDataEvent *event) {
  // Craft response per parser script
  struct ChppGnssDataEventWithHeader *notification;
  size_t notificationLen;
  if (!chppGnssDataEventFromChre(event, &notification, &notificationLen)) {
    CHPP_LOGE(
        "chppGnssMeasurementEventFromChre failed (OOM?). Transaction ID = "
        "%" PRIu8,
        gGnssServiceContext.controlMeasurementSession.transaction);
    // TODO: consider sending an error response if this fails

  } else {
    notification->header.handle = gGnssServiceContext.service.handle;
    notification->header.type = CHPP_MESSAGE_TYPE_SERVICE_NOTIFICATION;
    notification->header.transaction =
        gGnssServiceContext.controlMeasurementSession.transaction;
    notification->header.error = CHPP_APP_ERROR_NONE;
    notification->header.command = CHPP_GNSS_MEASUREMENT_RESULT_NOTIFICATION;

    chppEnqueueTxDatagramOrFail(
        gGnssServiceContext.service.appContext->transportContext, notification,
        notificationLen);
  }

  gGnssServiceContext.api->releaseMeasurementDataEvent(event);
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppRegisterGnssService(struct ChppAppState *appContext) {
  gGnssServiceContext.api = chrePalGnssGetApi(CHPP_PAL_GNSS_API_VERSION);

  if (gGnssServiceContext.api == NULL) {
    CHPP_LOGE(
        "GNSS PAL API version not compatible with CHPP. Cannot register GNSS "
        "service");
    CHPP_DEBUG_ASSERT(false);

  } else {
    gGnssServiceContext.service.appContext = appContext;
    gGnssServiceContext.service.handle = chppRegisterService(
        appContext, (void *)&gGnssServiceContext, &kGnssServiceConfig);
    CHPP_DEBUG_ASSERT(gGnssServiceContext.service.handle);
  }
}

void chppDeregisterGnssService(struct ChppAppState *appContext) {
  // TODO

  UNUSED_VAR(appContext);
}