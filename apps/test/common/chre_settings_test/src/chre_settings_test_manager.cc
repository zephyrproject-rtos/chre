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

#include "chre_settings_test_manager.h"

#include <pb_decode.h>
#include <pb_encode.h>

#include "chre/util/nanoapp/callbacks.h"
#include "chre/util/nanoapp/log.h"
#include "chre_settings_test.nanopb.h"
#include "chre_settings_test_util.h"

#define LOG_TAG "[ChreSettingsTest]"

namespace chre {

namespace settings_test {

namespace {

constexpr uint32_t kWifiScanningCookie = 0x1234;
constexpr uint32_t kGnssLocationCookie = 0x3456;
constexpr uint32_t kGnssMeasurementCookie = 0x4567;
constexpr uint32_t kWwanCellInfoCookie = 0x5678;

bool getFeature(const chre_settings_test_TestCommand &command,
                Manager::Feature *feature) {
  bool success = true;
  switch (command.feature) {
    case chre_settings_test_TestCommand_Feature_WIFI_SCANNING:
      *feature = Manager::Feature::WIFI_SCANNING;
      break;
    case chre_settings_test_TestCommand_Feature_WIFI_RTT:
      *feature = Manager::Feature::WIFI_RTT;
      break;
    case chre_settings_test_TestCommand_Feature_GNSS_LOCATION:
      *feature = Manager::Feature::GNSS_LOCATION;
      break;
    case chre_settings_test_TestCommand_Feature_GNSS_MEASUREMENT:
      *feature = Manager::Feature::GNSS_MEASUREMENT;
      break;
    case chre_settings_test_TestCommand_Feature_WWAN_CELL_INFO:
      *feature = Manager::Feature::WWAN_CELL_INFO;
      break;
    default:
      LOGE("Unknown feature %d", command.feature);
      success = false;
  }

  return success;
}

bool getFeatureState(const chre_settings_test_TestCommand &command,
                     Manager::FeatureState *state) {
  bool success = true;
  switch (command.state) {
    case chre_settings_test_TestCommand_State_ENABLED:
      *state = Manager::FeatureState::ENABLED;
      break;
    case chre_settings_test_TestCommand_State_DISABLED:
      *state = Manager::FeatureState::DISABLED;
      break;
    default:
      LOGE("Unknown feature state %d", command.state);
      success = false;
  }

  return success;
}

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

bool Manager::isFeatureSupported(Feature feature) {
  bool supported = false;

  uint32_t version = chreGetVersion();
  switch (feature) {
    case Feature::WIFI_SCANNING: {
      uint32_t capabilities = chreWifiGetCapabilities();
      supported = (version >= CHRE_API_VERSION_1_1) &&
                  ((capabilities & CHRE_WIFI_CAPABILITIES_ON_DEMAND_SCAN) != 0);
      break;
    }
    case Feature::GNSS_LOCATION: {
      uint32_t capabilities = chreGnssGetCapabilities();
      supported = (version >= CHRE_API_VERSION_1_1) &&
                  ((capabilities & CHRE_GNSS_CAPABILITIES_LOCATION) != 0);
      break;
    }
    case Feature::GNSS_MEASUREMENT: {
      uint32_t capabilities = chreGnssGetCapabilities();
      supported = (version >= CHRE_API_VERSION_1_1) &&
                  ((capabilities & CHRE_GNSS_CAPABILITIES_MEASUREMENTS) != 0);
      break;
    }
    case Feature::WWAN_CELL_INFO: {
      uint32_t capabilities = chreWwanGetCapabilities();
      supported = (version >= CHRE_API_VERSION_1_1) &&
                  ((capabilities & CHRE_WWAN_GET_CELL_INFO) != 0);
      break;
    }
    case Feature::WIFI_RTT:
    default:
      LOGE("Unknown feature %" PRIu8, feature);
  }

  return supported;
}

void Manager::handleMessageFromHost(uint32_t senderInstanceId,
                                    const chreMessageFromHostData *hostData) {
  bool success = false;
  uint32_t messageType = hostData->messageType;
  if (senderInstanceId != CHRE_INSTANCE_ID) {
    LOGE("Incorrect sender instance id: %" PRIu32, senderInstanceId);
  } else if (messageType != chre_settings_test_MessageType_TEST_COMMAND) {
    LOGE("Invalid message type %" PRIu32, messageType);
  } else {
    pb_istream_t istream = pb_istream_from_buffer(
        static_cast<const pb_byte_t *>(hostData->message),
        hostData->messageSize);
    chre_settings_test_TestCommand testCommand =
        chre_settings_test_TestCommand_init_default;

    if (!pb_decode(&istream, chre_settings_test_TestCommand_fields,
                   &testCommand)) {
      LOGE("Failed to decode start command error %s", PB_GET_ERROR(&istream));
    } else {
      Feature feature;
      FeatureState state;
      if (getFeature(testCommand, &feature) &&
          getFeatureState(testCommand, &state)) {
        handleStartTestMessage(hostData->hostEndpoint, feature, state);
        success = true;
      }
    }
  }

  if (!success) {
    sendTestResultToHost(hostData->hostEndpoint, false /* success */);
  }
}

void Manager::handleStartTestMessage(uint16_t hostEndpointId, Feature feature,
                                     FeatureState state) {
  // If the feature is not supported, treat as success and skip the test.
  if (!isFeatureSupported(feature)) {
    sendTestResult(hostEndpointId, true /* success */);
  } else if (!startTestForFeature(feature)) {
    sendTestResult(hostEndpointId, false /* success */);
  } else {
    mTestSession = TestSession(hostEndpointId, feature, state);
  }
}

void Manager::handleDataFromChre(uint16_t eventType, const void *eventData) {
  if (mTestSession.has_value()) {
    // The validation for the correct data w.r.t. the current test session
    // will be done in the methods called from here.
    switch (eventType) {
      case CHRE_EVENT_WIFI_ASYNC_RESULT: {
        handleWifiAsyncResult(static_cast<const chreAsyncResult *>(eventData));
        break;
      }
      case CHRE_EVENT_GNSS_ASYNC_RESULT: {
        handleGnssAsyncResult(static_cast<const chreAsyncResult *>(eventData));
        break;
      }
      case CHRE_EVENT_WWAN_CELL_INFO_RESULT: {
        handleWwanCellInfoResult(
            static_cast<const chreWwanCellInfoResult *>(eventData));
        break;
      }
      default:
        LOGE("Unknown event type %" PRIu16, eventType);
    }
  }
}

bool Manager::startTestForFeature(Feature feature) {
  bool success = true;
  switch (feature) {
    case Feature::WIFI_SCANNING: {
      success = chreWifiRequestScanAsyncDefault(&kWifiScanningCookie);
      break;
    }
    case Feature::GNSS_LOCATION: {
      success = chreGnssLocationSessionStartAsync(1000 /* minIntervalMs */,
                                                  0 /* minTimeToNextFixMs */,
                                                  &kGnssLocationCookie);
      break;
    }
    case Feature::GNSS_MEASUREMENT: {
      success = chreGnssMeasurementSessionStartAsync(1000 /* minIntervalMs */,
                                                     &kGnssMeasurementCookie);
      break;
    }
    case Feature::WWAN_CELL_INFO: {
      success = chreWwanGetCellInfoAsync(&kWwanCellInfoCookie);
      break;
    }
    case Feature::WIFI_RTT:
    default:
      LOGE("Unknown feature %" PRIu8, feature);
      return false;
  }

  if (!success) {
    LOGE("Failed to make request for test feature %" PRIu8, feature);
  } else {
    LOGI("Starting test for feature %" PRIu8, feature);
  }

  return success;
}

bool Manager::validateAsyncResult(const chreAsyncResult *result,
                                  const void *expectedCookie) {
  bool success = false;
  if (result->cookie != expectedCookie) {
    LOGE("Unexpected cookie on async result");
  } else {
    chreError expectedErrorCode =
        (mTestSession->featureState == FeatureState::ENABLED)
            ? CHRE_ERROR_NONE
            : CHRE_ERROR_FUNCTION_DISABLED;

    if (result->errorCode != expectedErrorCode) {
      LOGE("Unexpected async result: error code %" PRIu8 " expect %" PRIu8,
           result->errorCode, expectedErrorCode);
    } else {
      success = true;
    }
  }

  return success;
}

void Manager::handleWifiAsyncResult(const chreAsyncResult *result) {
  bool success = false;
  switch (result->requestType) {
    case CHRE_WIFI_REQUEST_TYPE_REQUEST_SCAN: {
      if (mTestSession->feature != Feature::WIFI_SCANNING) {
        LOGE("Unexpected WiFi scan async result: test feature %" PRIu8,
             mTestSession->feature);
      } else {
        success = validateAsyncResult(
            result, static_cast<const void *>(&kWifiScanningCookie));
      }
      break;
    }
    default:
      LOGE("Unexpected WiFi request type %" PRIu8, result->requestType);
  }

  sendTestResult(mTestSession->hostEndpointId, success);
}

void Manager::handleGnssAsyncResult(const chreAsyncResult *result) {
  bool success = false;
  switch (result->requestType) {
    case CHRE_GNSS_REQUEST_TYPE_LOCATION_SESSION_START: {
      if (mTestSession->feature != Feature::GNSS_LOCATION) {
        LOGE("Unexpected GNSS location async result: test feature %" PRIu8,
             mTestSession->feature);
      } else {
        success = validateAsyncResult(
            result, static_cast<const void *>(&kGnssLocationCookie));
        chreGnssLocationSessionStopAsync(&kGnssLocationCookie);
      }
      break;
    }
    case CHRE_GNSS_REQUEST_TYPE_MEASUREMENT_SESSION_START: {
      if (mTestSession->feature != Feature::GNSS_MEASUREMENT) {
        LOGE("Unexpected GNSS measurement async result: test feature %" PRIu8,
             mTestSession->feature);
      } else {
        success = validateAsyncResult(
            result, static_cast<const void *>(&kGnssMeasurementCookie));
        chreGnssMeasurementSessionStopAsync(&kGnssMeasurementCookie);
      }
      break;
    }
    default:
      LOGE("Unexpected GNSS request type %" PRIu8, result->requestType);
  }

  sendTestResult(mTestSession->hostEndpointId, success);
}

void Manager::handleWwanCellInfoResult(const chreWwanCellInfoResult *result) {
  bool success = false;
  // For WWAN, we treat "DISABLED" as success but with empty results, per
  // CHRE API requirements.
  if (mTestSession->feature != Feature::WWAN_CELL_INFO) {
    LOGE("Unexpected WWAN cell info result: test feature %" PRIu8,
         mTestSession->feature);
  } else if (result->cookie != &kWwanCellInfoCookie) {
    LOGE("Unexpected cookie on WWAN cell info result");
  } else if (result->errorCode != CHRE_ERROR_NONE) {
    LOGE("WWAN cell info result failed: error code %" PRIu8, result->errorCode);
  } else if (mTestSession->featureState == FeatureState::DISABLED &&
             result->cellInfoCount > 0) {
    LOGE("WWAN cell info result should be empty when disabled: count %" PRIu8,
         result->cellInfoCount);
  } else {
    success = true;
  }

  sendTestResult(mTestSession->hostEndpointId, success);
}

void Manager::sendTestResult(uint16_t hostEndpointId, bool success) {
  sendTestResultToHost(hostEndpointId, success);
  mTestSession.reset();
}

}  // namespace settings_test

}  // namespace chre
