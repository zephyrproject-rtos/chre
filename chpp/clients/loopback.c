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

#include "chpp/clients/loopback.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "chpp/app.h"
#include "chpp/clients.h"
#include "chpp/memory.h"
#include "chpp/platform/log.h"
#include "chpp/transport.h"

/************************************************
 *  Prototypes
 ***********************************************/

/************************************************
 *  Private Definitions
 ***********************************************/

/**
 * Structure to maintain state for the loopback client and its Request/Response
 * (RR) functionality.
 */
struct ChppLoopbackClientState {
  struct ChppClientState client;                    // Loopback client state
  struct ChppRequestResponseState runLoopbackTest;  // Loopback test state

  struct ChppLoopbackTestResult testResult;  // Last test result
  const uint8_t *loopbackData;               // Pointer to loopback data
};

// Note: This global definition of gLoopbackClientContext supports only one
// instance of the CHPP loopback client at a time.
struct ChppLoopbackClientState gLoopbackClientContext;

/************************************************
 *  Public Functions
 ***********************************************/

void chppLoopbackClientInit(struct ChppAppState *context) {
  gLoopbackClientContext.client.appContext = context;
  chppClientInit(&gLoopbackClientContext.client, CHPP_HANDLE_LOOPBACK);
  gLoopbackClientContext.testResult.error = CHPP_APP_ERROR_NONE;
}

void chppLoopbackClientDeinit(void) {
  chppClientDeinit(&gLoopbackClientContext.client);
}

bool chppDispatchLoopbackServiceResponse(struct ChppAppState *context,
                                         const uint8_t *response, size_t len) {
  UNUSED_VAR(context);
  CHPP_ASSERT(len >= CHPP_LOOPBACK_HEADER_LEN);
  CHPP_NOT_NULL(response);
  CHPP_NOT_NULL(gLoopbackClientContext.loopbackData);

  chppClientTimestampResponse(&gLoopbackClientContext.runLoopbackTest,
                              (struct ChppAppHeader *)response);

  gLoopbackClientContext.testResult.error = CHPP_APP_ERROR_NONE;
  gLoopbackClientContext.testResult.responseLen = len;
  gLoopbackClientContext.testResult.firstError = len;
  gLoopbackClientContext.testResult.byteErrors = 0;
  gLoopbackClientContext.testResult.rtt =
      gLoopbackClientContext.runLoopbackTest.responseTime -
      gLoopbackClientContext.runLoopbackTest.requestTime;

  if (gLoopbackClientContext.testResult.requestLen !=
      gLoopbackClientContext.testResult.responseLen) {
    gLoopbackClientContext.testResult.error = CHPP_APP_ERROR_INVALID_LENGTH;
    gLoopbackClientContext.testResult.firstError =
        MIN(gLoopbackClientContext.testResult.requestLen,
            gLoopbackClientContext.testResult.responseLen);
  }

  for (size_t loc = CHPP_LOOPBACK_HEADER_LEN;
       loc < MIN(gLoopbackClientContext.testResult.requestLen,
                 gLoopbackClientContext.testResult.responseLen);
       loc++) {
    if (gLoopbackClientContext.loopbackData[loc - CHPP_LOOPBACK_HEADER_LEN] !=
        response[loc]) {
      gLoopbackClientContext.testResult.error = CHPP_APP_ERROR_UNSPECIFIED;
      gLoopbackClientContext.testResult.firstError =
          MIN(gLoopbackClientContext.testResult.firstError,
              loc - CHPP_LOOPBACK_HEADER_LEN);
      gLoopbackClientContext.testResult.byteErrors++;
    }
  }

  CHPP_LOGD(
      "Loopback client processed service response. Test %s. response "
      "len=%" PRIuSIZE ", request len=%" PRIuSIZE ", error code=0x%" PRIx16
      ", first error=%" PRIuSIZE ", total errors=%" PRIuSIZE,
      (gLoopbackClientContext.testResult.error == CHPP_APP_ERROR_NONE)
          ? "succeeded"
          : "failed",
      gLoopbackClientContext.testResult.responseLen,
      gLoopbackClientContext.testResult.requestLen,
      gLoopbackClientContext.testResult.error,
      gLoopbackClientContext.testResult.firstError,
      gLoopbackClientContext.testResult.byteErrors);

  // Notify waiting (synchronous) client
  chppMutexLock(&gLoopbackClientContext.client.responseMutex);
  gLoopbackClientContext.client.responseReady = true;
  chppConditionVariableSignal(&gLoopbackClientContext.client.responseCondVar);
  chppMutexUnlock(&gLoopbackClientContext.client.responseMutex);

  return true;
}

struct ChppLoopbackTestResult chppRunLoopbackTest(struct ChppAppState *context,
                                                  const uint8_t *buf,
                                                  size_t len) {
  UNUSED_VAR(context);
  CHPP_NOT_NULL(buf);

  CHPP_LOGD("Running loopback test with payload len=%" PRIuSIZE
            ", request len=%" PRIuSIZE,
            len, len + CHPP_LOOPBACK_HEADER_LEN);

  if (gLoopbackClientContext.testResult.error == CHPP_APP_ERROR_BLOCKED) {
    CHPP_LOGE("Loopback test cannot be run while another is in progress");
    CHPP_DEBUG_ASSERT(false);

  } else {
    gLoopbackClientContext.testResult.error = CHPP_APP_ERROR_BLOCKED;
    gLoopbackClientContext.testResult.requestLen =
        len + CHPP_LOOPBACK_HEADER_LEN;
    gLoopbackClientContext.testResult.responseLen = 0;
    gLoopbackClientContext.testResult.firstError = 0;
    gLoopbackClientContext.testResult.byteErrors = 0;
    gLoopbackClientContext.testResult.rtt = 0;
    gLoopbackClientContext.runLoopbackTest.requestTime = CHPP_TIME_NONE;
    gLoopbackClientContext.runLoopbackTest.responseTime = CHPP_TIME_NONE;

    if (len == 0) {
      CHPP_LOGE("Loopback payload too short");
      gLoopbackClientContext.testResult.error = CHPP_APP_ERROR_INVALID_LENGTH;

    } else {
      uint8_t *loopbackRequest = (uint8_t *)chppAllocClientRequest(
          &gLoopbackClientContext.client,
          gLoopbackClientContext.testResult.requestLen);

      if (loopbackRequest == NULL) {
        // OOM
        gLoopbackClientContext.testResult.requestLen = 0;
        gLoopbackClientContext.testResult.error = CHPP_APP_ERROR_OOM;
        CHPP_LOG_OOM();

      } else {
        gLoopbackClientContext.loopbackData = buf;
        memcpy(&loopbackRequest[CHPP_LOOPBACK_HEADER_LEN], buf, len);

        if (!chppSendTimestampedRequestAndWait(
                &gLoopbackClientContext.client,
                &gLoopbackClientContext.runLoopbackTest, loopbackRequest,
                gLoopbackClientContext.testResult.requestLen)) {
          gLoopbackClientContext.testResult.error = CHPP_APP_ERROR_UNSPECIFIED;
        }  // else {gLoopbackClientContext.testResult is now populated}
      }
    }
  }

  return gLoopbackClientContext.testResult;
}
