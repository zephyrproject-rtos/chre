/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "chre_stress_test_manager.h"

#include <pb_decode.h>

#include "chre/util/macros.h"
#include "chre/util/nanoapp/callbacks.h"
#include "chre/util/nanoapp/log.h"
#include "chre_stress_test.nanopb.h"
#include "send_message.h"

#define LOG_TAG "[ChreStressTest]"

namespace chre {

namespace stress_test {

namespace {

constexpr chre::Nanoseconds kWifiScanInterval = chre::Seconds(5);

}  // anonymous namespace

void Manager::handleEvent(uint32_t senderInstanceId, uint16_t eventType,
                          const void *eventData) {
  if (eventType == CHRE_EVENT_MESSAGE_FROM_HOST) {
    handleMessageFromHost(
        senderInstanceId,
        static_cast<const chreMessageFromHostData *>(eventData));
  } else if (senderInstanceId == CHRE_INSTANCE_ID) {
    handleDataFromChre(eventType, eventData);
  } else {
    LOGW("Got unknown event type from senderInstanceId %" PRIu32
         " and with eventType %" PRIu16,
         senderInstanceId, eventType);
  }
}

void Manager::handleMessageFromHost(uint32_t senderInstanceId,
                                    const chreMessageFromHostData *hostData) {
  bool success = false;
  uint32_t messageType = hostData->messageType;
  if (senderInstanceId != CHRE_INSTANCE_ID) {
    LOGE("Incorrect sender instance id: %" PRIu32, senderInstanceId);
  } else if (messageType != chre_stress_test_MessageType_TEST_COMMAND) {
    LOGE("Invalid message type %" PRIu32, messageType);
  } else if (mHostEndpoint.has_value() &&
             hostData->hostEndpoint != mHostEndpoint.value()) {
    LOGE("Invalid host endpoint %" PRIu16 " expected %" PRIu16,
         hostData->hostEndpoint, mHostEndpoint.value());
  } else {
    pb_istream_t istream = pb_istream_from_buffer(
        static_cast<const pb_byte_t *>(hostData->message),
        hostData->messageSize);
    chre_stress_test_TestCommand testCommand =
        chre_stress_test_TestCommand_init_default;

    if (!pb_decode(&istream, chre_stress_test_TestCommand_fields,
                   &testCommand)) {
      LOGE("Failed to decode start command error %s", PB_GET_ERROR(&istream));
    } else {
      LOGI("Got message from host: feature %d start %d", testCommand.feature,
           testCommand.start);

      success = true;
      switch (testCommand.feature) {
        case chre_stress_test_TestCommand_Feature_WIFI: {
          handleWifiStartCommand(testCommand.start);
          break;
        }
        case chre_stress_test_TestCommand_Feature_GNSS_LOCATION: {
          handleGnssLocationStartCommand(testCommand.start);
          break;
        }
        default: {
          LOGE("Unknown feature %d", testCommand.feature);
          success = false;
          break;
        }
      }
    }

    mHostEndpoint = hostData->hostEndpoint;
  }

  if (!success) {
    test_shared::sendTestResultWithMsgToHost(
        hostData->hostEndpoint,
        chre_stress_test_MessageType_TEST_RESULT /* messageType */, success,
        nullptr /* errMessage */);
  }
}

void Manager::handleDataFromChre(uint16_t eventType, const void *eventData) {
  switch (eventType) {
    case CHRE_EVENT_TIMER:
      handleTimerEvent(static_cast<const uint32_t *>(eventData));
      break;

    case CHRE_EVENT_WIFI_ASYNC_RESULT:
      handleWifiAsyncResult(static_cast<const chreAsyncResult *>(eventData));
      break;

    case CHRE_EVENT_WIFI_SCAN_RESULT:
      handleWifiScanEvent(static_cast<const chreWifiScanEvent *>(eventData));
      break;

    case CHRE_EVENT_GNSS_ASYNC_RESULT:
      handleGnssAsyncResult(static_cast<const chreAsyncResult *>(eventData));
      break;

    case CHRE_EVENT_GNSS_LOCATION:
      handleGnssLocationEvent(
          static_cast<const chreGnssLocationEvent *>(eventData));
      break;

    default:
      LOGW("Unknown event type %" PRIu16, eventType);
      break;
  }
}

void Manager::handleTimerEvent(const uint32_t *handle) {
  if (*handle == mWifiScanTimerHandle) {
    if (mWifiScanAsyncRequest.has_value()) {
      if (chreGetTime() > (mWifiScanAsyncRequest->requestTimeNs +
                           CHRE_WIFI_SCAN_RESULT_TIMEOUT_NS)) {
        logAndSendFailure("Prev WiFi scan did not complete in time");
      }
    } else {
      bool success = chreWifiRequestScanAsyncDefault(&kOnDemandWifiScanCookie);
      LOGI("Requested on demand wifi success ? %d", success);
      if (success) {
        mWifiScanAsyncRequest = AsyncRequest(&kOnDemandWifiScanCookie);
      }
    }

    requestDelayedWifiScan();
  } else if (*handle == mGnssLocationTimerHandle) {
    makeGnssLocationRequest();
  } else if (*handle == mGnssAsyncTimerHandle &&
             mGnssLocationAsyncRequest.has_value()) {
    logAndSendFailure("GNSS async result timed out");
  } else {
    logAndSendFailure("Unknown timer handle");
  }
}

void Manager::handleWifiAsyncResult(const chreAsyncResult *result) {
  if (result->requestType == CHRE_WIFI_REQUEST_TYPE_REQUEST_SCAN) {
    if (result->success) {
      LOGI("On-demand scan success");
    } else {
      LOGW("On-demand scan failed: code %" PRIu8, result->errorCode);
    }

    if (!mWifiScanAsyncRequest.has_value()) {
      logAndSendFailure("Received WiFi async result with no pending request");
    } else if (result->cookie != mWifiScanAsyncRequest->cookie) {
      logAndSendFailure("On-demand scan cookie mismatch");
    }

    mWifiScanAsyncRequest.reset();
  } else {
    logAndSendFailure("Unknown WiFi async result type");
  }
}

void Manager::handleGnssAsyncResult(const chreAsyncResult *result) {
  if ((result->requestType == CHRE_GNSS_REQUEST_TYPE_LOCATION_SESSION_START) ||
      (result->requestType == CHRE_GNSS_REQUEST_TYPE_LOCATION_SESSION_STOP)) {
    if (!mGnssLocationAsyncRequest.has_value()) {
      logAndSendFailure(
          "Received location async result with no pending request");
    } else if (!result->success) {
      logAndSendFailure("Async location failure");
    } else if (result->cookie != mGnssLocationAsyncRequest->cookie) {
      logAndSendFailure("Location cookie mismatch");
    }

    cancelTimer(&mGnssAsyncTimerHandle);
    mGnssLocationAsyncRequest.reset();
  } else {
    logAndSendFailure("Unknown GNSS async result type");
  }
}

void Manager::handleGnssLocationEvent(const chreGnssLocationEvent *event) {
  LOGI("Received GNSS location event at %" PRIu64 " ns", event->timestamp);

  // TODO(b/186868033): Check results
}

void Manager::handleWifiScanEvent(const chreWifiScanEvent *event) {
  LOGI("Received Wifi scan event of type %" PRIu8 " with %" PRIu8
       " results at %" PRIu64 " ns",
       event->scanType, event->resultCount, event->referenceTime);

  // TODO(b/186868033): Check results
}

void Manager::handleWifiStartCommand(bool start) {
  mWifiTestStarted = start;
  if (start) {
    requestDelayedWifiScan();
  }
}

void Manager::handleGnssLocationStartCommand(bool start) {
  constexpr uint64_t kTimerDelayNs = Seconds(60).toRawNanoseconds();

  if (chreGnssGetCapabilities() & CHRE_GNSS_CAPABILITIES_LOCATION) {
    mGnssLocationTestStarted = start;
    makeGnssLocationRequest();

    if (start) {
      setTimer(kTimerDelayNs, false /* oneShot */, &mGnssLocationTimerHandle);
    }
  } else {
    logAndSendFailure("Platform has no location capability");
  }
}

void Manager::setTimer(uint64_t delayNs, bool oneShot, uint32_t *timerHandle) {
  *timerHandle = chreTimerSet(delayNs, timerHandle, oneShot);
  if (*timerHandle == CHRE_TIMER_INVALID) {
    logAndSendFailure("Failed to set timer");
  }
}

void Manager::cancelTimer(uint32_t *timerHandle) {
  if (*timerHandle != CHRE_TIMER_INVALID) {
    if (!chreTimerCancel(*timerHandle)) {
      logAndSendFailure("Failed to cancel timer");
    }
    *timerHandle = CHRE_TIMER_INVALID;
  }
}

void Manager::makeGnssLocationRequest() {
  // The list of location intervals to iterate; wraps around.
  static const uint32_t kMinIntervalMsList[] = {1000, 0};
  static size_t sIntervalIndex = 0;

  uint32_t minIntervalMs = kMinIntervalMsList[sIntervalIndex];
  sIntervalIndex = (sIntervalIndex + 1) % ARRAY_SIZE(kMinIntervalMsList);

  bool success = false;
  if (minIntervalMs > 0 && mGnssLocationTestStarted) {
    success = chreGnssLocationSessionStartAsync(
        minIntervalMs, 0 /* minTimeToNextFixMs */, &kGnssLocationCookie);
  } else {
    success = chreGnssLocationSessionStopAsync(&kGnssLocationCookie);
  }

  LOGI("Configure GNSS location interval %" PRIu32 " ms success ? %d",
       minIntervalMs, success);

  if (!success) {
    logAndSendFailure("Failed to make location request");
  } else {
    mGnssLocationAsyncRequest = AsyncRequest(&kGnssLocationCookie);
    setTimer(CHRE_GNSS_ASYNC_RESULT_TIMEOUT_NS, true /* oneShot */,
             &mGnssAsyncTimerHandle);
  }
}

void Manager::requestDelayedWifiScan() {
  if (mWifiTestStarted) {
    if (chreWifiGetCapabilities() & CHRE_WIFI_CAPABILITIES_ON_DEMAND_SCAN) {
      setTimer(kWifiScanInterval.toRawNanoseconds(), true /* oneShot */,
               &mWifiScanTimerHandle);
    } else {
      logAndSendFailure("Platform has no on-demand scan capability");
    }
  }
}

void Manager::logAndSendFailure(const char *errorMessage) {
  LOGE("%s", errorMessage);
  test_shared::sendTestResultWithMsgToHost(
      mHostEndpoint.value(),
      chre_stress_test_MessageType_TEST_RESULT /* messageType */,
      false /* success */, errorMessage);
}

}  // namespace stress_test

}  // namespace chre
