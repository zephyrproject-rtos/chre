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

#include "chpp/services.h"
#include "chpp/app.h"
#include "chpp/services/wwan.h"

/************************************************
 *  Prototypes
 ***********************************************/

void uuidToStr(const uint8_t uuid[CHPP_SERVICE_UUID_LEN],
               char strOut[CHPP_SERVICE_UUID_STRING_LEN]);

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Convert UUID to a human-readable, null-terminated string.
 *
 * @param uuid Input UUID
 * @param strOut Output null-terminated string
 */
void uuidToStr(const uint8_t uuid[CHPP_SERVICE_UUID_LEN],
               char strOut[CHPP_SERVICE_UUID_STRING_LEN]) {
  sprintf(
      strOut,
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
      uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14],
      uuid[15]);
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppRegisterCommonServices(struct ChppAppState *context) {
#ifdef CHPP_SERVICE_ENABLED_WWAN
  chppRegisterWwanService(context);
#endif

#ifdef CHPP_SERVICE_ENABLED_WLAN
  chppRegisterWlanService(context);
#endif

#ifdef CHPP_SERVICE_ENABLED_GNSS
  chppRegisterGnssService(context);
#endif
}

uint8_t chppRegisterService(struct ChppAppState *appContext,
                            void *serviceContext,
                            const struct ChppService *newService) {
  CHPP_NOT_NULL(newService);

  if (appContext->registeredServiceCount >= CHPP_MAX_REGISTERED_SERVICES) {
    LOGE("Cannot register new service # %" PRIu8 ". Already hit maximum",
         appContext->registeredServiceCount);
    return CHPP_HANDLE_NONE;

  } else {
    appContext->registeredServices[appContext->registeredServiceCount] =
        newService;
    appContext->registeredServiceContexts[appContext->registeredServiceCount] =
        serviceContext;
    appContext->registeredServiceCount++;

    char uuidText[CHPP_SERVICE_UUID_STRING_LEN];
    uuidToStr(newService->descriptor.uuid, uuidText);
    LOGI(
        "Registered service %" PRIu8 " on handle %" PRIu8
        " with name=%s, UUID=%s, "
        "version=%" PRIu8 ".%" PRIu8 ".%" PRIu16 ", min_len=%zu ",
        appContext->registeredServiceCount,
        appContext->registeredServiceCount + CHPP_HANDLE_NEGOTIATED_RANGE_START,
        newService->descriptor.name, uuidText,
        newService->descriptor.versionMajor,
        newService->descriptor.versionMinor,
        newService->descriptor.versionPatch, newService->minLength);

    return appContext->registeredServiceCount +
           CHPP_HANDLE_NEGOTIATED_RANGE_START;
  }
}

struct ChppAppHeader *chppAllocServiceResponse(
    const struct ChppAppHeader *requestHeader, size_t len) {
  CHPP_ASSERT(len >= sizeof(struct ChppAppHeader));

  struct ChppAppHeader *result = chppMalloc(len);
  if (result) {
    *result = *requestHeader;
    result->type = CHPP_MESSAGE_TYPE_SERVER_RESPONSE;
  }
  return (void *)result;
}

void chppTimestampRequest(struct ChppServiceRRState *rRState,
                          struct ChppAppHeader *requestHeader) {
  if (rRState->responseTime == 0 && rRState->requestTime != 0) {
    LOGE(
        "Received duplicate request while prior request was outstanding from t "
        "= %" PRIu64,
        rRState->requestTime);
  }
  rRState->requestTime = chppGetCurrentTime();
  rRState->responseTime = CHPP_TIME_NONE;
  rRState->transaction = requestHeader->transaction;
}

void chppTimestampResponse(struct ChppServiceRRState *rRState) {
  uint64_t previousResponseTime = rRState->responseTime;
  rRState->responseTime = chppGetCurrentTime();

  if (rRState->requestTime == 0) {
    LOGE("Received response at t = %" PRIu64
         " with no prior outstanding request",
         rRState->responseTime);

  } else if ((previousResponseTime - rRState->requestTime) > 0) {
    rRState->responseTime = chppGetCurrentTime();
    LOGI("Received additional response at t = %" PRIu64
         " for request at t = %" PRIu64 " (RTT = %" PRIu64 ")",
         rRState->responseTime, rRState->responseTime,
         rRState->responseTime - rRState->requestTime);

  } else {
    LOGI("Received initial response at t = %" PRIu64
         " for request at t = %" PRIu64 " (RTT = %" PRIu64 ")",
         rRState->responseTime, rRState->responseTime,
         rRState->responseTime - rRState->requestTime);
  }
}

bool chppSendTimestampedResponseOrFail(struct ChppServiceState *serviceState,
                                       struct ChppServiceRRState *rRState,
                                       void *buf, size_t len) {
  chppTimestampResponse(rRState);
  return chppEnqueueTxDatagramOrFail(serviceState->appContext->transportContext,
                                     buf, len);
}