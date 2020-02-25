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

/**
 * The nanoapp that will request data from CHRE apis and send that data back to
 * the host so that it can be compared against host side data. The nanoapp will
 * request different chre apis (wifi, sensor, etc) depeding on the message type
 * given in start message.
 */

#include <cinttypes>

#include <chre.h>
#include <pb_decode.h>
#include <pb_encode.h>

#include "chre/util/nanoapp/callbacks.h"
#include "chre/util/nanoapp/log.h"
#include "chre/util/optional.h"
#include "chre/util/time.h"
#include "chre_cross_validation.nanopb.h"

#define LOG_TAG "ChreCrossValidator"

/* TODO(b/148481242): Send all errors to host as well as just logging them as
 * errors.
 *
 * TODO(b/146052784): Move start and handle data methods for each cross
 * validation type (sensor[accel, gyro, ...], wifi, gps) to a manager class.
 *
 * TODO(b/146052784): Create a helper function to get string version of
 * sensorType for logging.
 */

namespace chre {

namespace {

enum class CrossValidatorType { SENSOR };

// Set upon received start message and read when nanoapp ends to handle cleanup
chre::Optional<CrossValidatorType> gCrossValidatorType;

// Set when start message is received and default sensor is found for requested
// sensor type and read when the sensor configuration is being cleaned up.
chre::Optional<uint32_t> gSensorHandle;

// Set to the current time when start message is received. When a data event is
// processed it is discarded if its timestamp is before this time.
chre::Optional<uint64_t> gTimeStart;

// The host endpoint which is read from the start message and used when sending
// data back to AP.
uint16_t gHostEndpoint = CHRE_HOST_ENDPOINT_BROADCAST;

bool encodeThreeAxisSensorDatapointValues(pb_ostream_t *stream,
                                          const pb_field_t * /*field*/,
                                          void *const *arg) {
  const auto *sensorThreeAxisDataSample = static_cast<
      const chreSensorThreeAxisData::chreSensorThreeAxisSampleData *>(*arg);

  for (size_t i = 0; i < 3; i++) {
    if (!pb_encode_tag_for_field(
            stream,
            &chre_cross_validation_SensorDatapoint_fields
                [chre_cross_validation_SensorDatapoint_values_tag - 1])) {
      return false;
    }
    if (!pb_encode_fixed32(stream, &sensorThreeAxisDataSample->values[i])) {
      return false;
    }
  }
  return true;
}

chre_cross_validation_SensorDatapoint makeDatapoint(
    const chreSensorThreeAxisData::chreSensorThreeAxisSampleData
        &sampleDataFromChre,
    uint64_t currentTimestamp) {
  return chre_cross_validation_SensorDatapoint{
      .has_timestampInNs = true,
      .timestampInNs = currentTimestamp,
      .values = {.funcs = {.encode = encodeThreeAxisSensorDatapointValues},
                 .arg = const_cast<
                     chreSensorThreeAxisData::chreSensorThreeAxisSampleData *>(
                     &sampleDataFromChre)}};
}

bool encodeThreeAxisSensorDatapoints(pb_ostream_t *stream,
                                     const pb_field_t * /*field*/,
                                     void *const *arg) {
  const auto *sensorThreeAxisData =
      static_cast<const chreSensorThreeAxisData *>(*arg);
  uint64_t currentTimestamp = sensorThreeAxisData->header.baseTimestamp +
                              chreGetEstimatedHostTimeOffset();
  for (size_t i = 0; i < sensorThreeAxisData->header.readingCount; i++) {
    const chreSensorThreeAxisData::chreSensorThreeAxisSampleData &sampleData =
        sensorThreeAxisData->readings[i];
    currentTimestamp += sampleData.timestampDelta;
    if (!pb_encode_tag_for_field(
            stream,
            &chre_cross_validation_SensorData_fields
                [chre_cross_validation_SensorData_datapoints_tag - 1])) {
      return false;
    }
    chre_cross_validation_SensorDatapoint datapoint =
        makeDatapoint(sampleData, currentTimestamp);
    if (!pb_encode_submessage(
            stream, chre_cross_validation_SensorDatapoint_fields, &datapoint)) {
      return false;
    }
  }
  return true;
}

void handleStartSensorMessage(
    const chre_cross_validation_StartSensorCommand &startSensorCommand) {
  uint8_t sensorType = startSensorCommand.sensorType;
  uint64_t interval = startSensorCommand.samplingIntervalInNs;
  uint64_t latency = startSensorCommand.samplingMaxLatencyInNs;
  uint32_t handle;
  if (!chreSensorFindDefault(sensorType, &handle)) {
    LOGE("Could not find default sensor for sensorType %" PRIu8, sensorType);
    // TODO(b/146052784): Test other sensor configure modes
  } else {
    if (!chreSensorConfigure(handle, CHRE_SENSOR_CONFIGURE_MODE_CONTINUOUS,
                             interval, latency)) {
      LOGE("Error configuring sensor with sensorType %" PRIu8
           ", interval %" PRIu64 "ns, and latency %" PRIu64 "ns",
           sensorType, interval, latency);
    } else {
      gSensorHandle = handle;
      gCrossValidatorType = CrossValidatorType::SENSOR;
      gTimeStart = chreGetTime();
      LOGD("Sensor with sensor type %" PRIu8 " configured", sensorType);
    }
  }
}

bool isValidHeader(const chreSensorDataHeader &header) {
  return header.readingCount > 0 && gTimeStart.has_value() &&
         header.baseTimestamp > gTimeStart.value();
}

void handleStartMessage(const chreMessageFromHostData *hostData) {
  if (hostData->hostEndpoint != CHRE_HOST_ENDPOINT_UNSPECIFIED) {
    gHostEndpoint = hostData->hostEndpoint;
  } else {
    gHostEndpoint = CHRE_HOST_ENDPOINT_BROADCAST;
  }
  pb_istream_t istream = pb_istream_from_buffer(
      static_cast<const pb_byte_t *>(hostData->message), hostData->messageSize);
  chre_cross_validation_StartCommand startCommand =
      chre_cross_validation_StartCommand_init_default;
  if (!pb_decode(&istream, chre_cross_validation_StartCommand_fields,
                 &startCommand)) {
    LOGE("Could not decode start command");
  } else {
    switch (startCommand.which_command) {
      case chre_cross_validation_StartCommand_startSensorCommand_tag:
        handleStartSensorMessage(startCommand.command.startSensorCommand);
        break;
      default:
        LOGE("Unknown start command type %" PRIu8, startCommand.which_command);
    }
  }
}

void handleMessageFromHost(uint32_t senderInstanceId,
                           const chreMessageFromHostData *hostData) {
  if (senderInstanceId != CHRE_INSTANCE_ID) {
    LOGE("Incorrect sender instance id: %" PRIu32, senderInstanceId);
  } else {
    switch (hostData->messageType) {
      case chre_cross_validation_MessageType_CHRE_CROSS_VALIDATION_START:
        handleStartMessage(hostData);
        break;
      default:
        LOGE("Unknown message type %" PRIu32 " for host message",
             hostData->messageType);
    }
  }
}

chre_cross_validation_Data makeAccelSensorData(
    const chreSensorThreeAxisData *threeAxisDataFromChre) {
  chre_cross_validation_SensorData newThreeAxisData = {
      .has_sensorType = true,
      .sensorType = chre_cross_validation_SensorType_ACCELEROMETER,
      .has_accuracy = true,
      .accuracy = threeAxisDataFromChre->header.accuracy,
      .datapoints = {
          .funcs = {.encode = encodeThreeAxisSensorDatapoints},
          .arg = const_cast<chreSensorThreeAxisData *>(threeAxisDataFromChre)}};
  chre_cross_validation_Data newData = {
      .which_data = chre_cross_validation_Data_sensorData_tag,
      .data =
          {
              .sensorData = newThreeAxisData,
          },
  };
  return newData;
}

void handleSensorThreeAxisData(
    const chreSensorThreeAxisData *threeAxisDataFromChre) {
  if (!isValidHeader(threeAxisDataFromChre->header)) {
    LOGE("Invalid threeAxisData being thrown away");
  } else {
    chre_cross_validation_Data newData =
        makeAccelSensorData(threeAxisDataFromChre);
    size_t encodedSize;
    if (!pb_get_encoded_size(&encodedSize, chre_cross_validation_Data_fields,
                             &newData)) {
      LOGE("Could not get encoded size of chreSensorThreeAxisData");
    } else {
      pb_byte_t *buffer = static_cast<pb_byte_t *>(chreHeapAlloc(encodedSize));
      if (buffer == nullptr) {
        LOG_OOM();
      } else {
        pb_ostream_t ostream = pb_ostream_from_buffer(buffer, encodedSize);
        if (!pb_encode(&ostream, chre_cross_validation_Data_fields, &newData)) {
          LOGE("Could not encode three axis data protobuf");
        } else if (
            !chreSendMessageToHostEndpoint(
                static_cast<void *>(buffer), encodedSize,
                chre_cross_validation_MessageType_CHRE_CROSS_VALIDATION_DATA,
                gHostEndpoint, heapFreeMessageCallback)) {
          LOGE("Could not send message to host");
        }
      }
    }
  }
}

void cleanup() {
  if (gCrossValidatorType.has_value()) {
    switch (gCrossValidatorType.value()) {
      case CrossValidatorType::SENSOR:
        if (gSensorHandle.has_value() &&
            !chreSensorConfigureModeOnly(gSensorHandle.value(),
                                         CHRE_SENSOR_CONFIGURE_MODE_DONE)) {
          LOGE(
              "Sensor cleanup failed when trying to configure sensor with "
              "handle "
              "%" PRIu32 " to done mode",
              gSensorHandle.value());
        }
        break;
      default:
        break;
    }
  }
}

}  // namespace

extern "C" void nanoappHandleEvent(uint32_t senderInstanceId,
                                   uint16_t eventType, const void *eventData) {
  switch (eventType) {
    case CHRE_EVENT_MESSAGE_FROM_HOST:
      handleMessageFromHost(
          senderInstanceId,
          static_cast<const chreMessageFromHostData *>(eventData));
      break;
    // TODO(b/146052784): Check that data received from CHRE apis is the correct
    // type for current test.
    case CHRE_EVENT_SENSOR_ACCELEROMETER_DATA:
      handleSensorThreeAxisData(
          static_cast<const chreSensorThreeAxisData *>(eventData));
      break;
    default:
      LOGE("Got unknown event type from senderInstanceId %" PRIu32
           " and with eventType %" PRIu16,
           senderInstanceId, eventType);
  }
}

extern "C" bool nanoappStart(void) {
  return true;
}

extern "C" void nanoappEnd(void) {
  cleanup();
}

}  // namespace chre
