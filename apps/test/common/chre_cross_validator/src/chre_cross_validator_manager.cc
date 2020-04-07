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

#include "chre_cross_validator_manager.h"

#include <cinttypes>

#include <chre.h>
#include <pb_decode.h>
#include <pb_encode.h>

#include "chre/util/nanoapp/callbacks.h"
#include "chre/util/nanoapp/log.h"
#include "chre/util/optional.h"
#include "chre/util/time.h"
#include "chre_cross_validation.nanopb.h"

#include "chre_cross_validator_manager.h"

#define LOG_TAG "ChreCrossValidator"

namespace chre {

namespace cross_validator {

Manager::~Manager() {
  cleanup();
}

void Manager::cleanup() {
  if (mCrossValidatorState.has_value()) {
    switch (mCrossValidatorState->crossValidatorType) {
      case CrossValidatorType::SENSOR:
        if (!chreSensorConfigureModeOnly(mCrossValidatorState->sensorHandle,
                                         CHRE_SENSOR_CONFIGURE_MODE_DONE)) {
          LOGE(
              "Sensor cleanup failed when trying to configure sensor with "
              "handle "
              "%" PRIu32 " to done mode",
              mCrossValidatorState->sensorHandle);
        }
        break;
      default:
        break;
    }
  }
}

void Manager::handleEvent(uint32_t senderInstanceId, uint16_t eventType,
                          const void *eventData) {
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
          static_cast<const chreSensorThreeAxisData *>(eventData),
          CHRE_SENSOR_TYPE_ACCELEROMETER);
      break;
    case CHRE_EVENT_SENSOR_GYROSCOPE_DATA:
      handleSensorThreeAxisData(
          static_cast<const chreSensorThreeAxisData *>(eventData),
          CHRE_SENSOR_TYPE_GYROSCOPE);
      break;
    case CHRE_EVENT_SENSOR_GEOMAGNETIC_FIELD_DATA:
      handleSensorThreeAxisData(
          static_cast<const chreSensorThreeAxisData *>(eventData),
          CHRE_SENSOR_TYPE_GEOMAGNETIC_FIELD);
      break;
    case CHRE_EVENT_SENSOR_PRESSURE_DATA:
      handleSensorFloatData(static_cast<const chreSensorFloatData *>(eventData),
                            CHRE_SENSOR_TYPE_PRESSURE);
      break;
    default:
      LOGE("Got unknown event type from senderInstanceId %" PRIu32
           " and with eventType %" PRIu16,
           senderInstanceId, eventType);
  }
}

bool Manager::encodeThreeAxisSensorDatapointValues(pb_ostream_t *stream,
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

chre_cross_validation_SensorDatapoint Manager::makeDatapoint(
    bool (*encodeFunc)(pb_ostream_t *, const pb_field_t *, void *const *),
    const void *sampleDataFromChre, uint64_t currentTimestamp) {
  return chre_cross_validation_SensorDatapoint{
      .has_timestampInNs = true,
      .timestampInNs = currentTimestamp,
      .values = {.funcs = {.encode = encodeFunc},
                 .arg = const_cast<void *>(sampleDataFromChre)}};
}

bool Manager::encodeFloatSensorDatapointValue(pb_ostream_t *stream,
                                              const pb_field_t * /*field*/,
                                              void *const *arg) {
  const auto *sensorFloatDataSample =
      static_cast<const chreSensorFloatData::chreSensorFloatSampleData *>(*arg);
  if (!pb_encode_tag_for_field(
          stream, &chre_cross_validation_SensorDatapoint_fields
                      [chre_cross_validation_SensorDatapoint_values_tag - 1])) {
    return false;
  }
  if (!pb_encode_fixed32(stream, &sensorFloatDataSample->value)) {
    return false;
  }
  return true;
}

bool Manager::encodeThreeAxisSensorDatapoints(pb_ostream_t *stream,
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
    chre_cross_validation_SensorDatapoint datapoint = makeDatapoint(
        encodeThreeAxisSensorDatapointValues, &sampleData, currentTimestamp);
    if (!pb_encode_submessage(
            stream, chre_cross_validation_SensorDatapoint_fields, &datapoint)) {
      return false;
    }
  }
  return true;
}

bool Manager::encodeFloatSensorDatapoints(pb_ostream_t *stream,
                                          const pb_field_t * /*field*/,
                                          void *const *arg) {
  const auto *sensorFloatData = static_cast<const chreSensorFloatData *>(*arg);
  uint64_t currentTimestamp =
      sensorFloatData->header.baseTimestamp + chreGetEstimatedHostTimeOffset();
  for (size_t i = 0; i < sensorFloatData->header.readingCount; i++) {
    const chreSensorFloatData::chreSensorFloatSampleData &sampleData =
        sensorFloatData->readings[i];
    currentTimestamp += sampleData.timestampDelta;
    if (!pb_encode_tag_for_field(
            stream,
            &chre_cross_validation_SensorData_fields
                [chre_cross_validation_SensorData_datapoints_tag - 1])) {
      return false;
    }
    chre_cross_validation_SensorDatapoint datapoint = makeDatapoint(
        encodeFloatSensorDatapointValue, &sampleData, currentTimestamp);
    if (!pb_encode_submessage(
            stream, chre_cross_validation_SensorDatapoint_fields, &datapoint)) {
      return false;
    }
  }
  return true;
}

bool Manager::handleStartSensorMessage(
    const chre_cross_validation_StartSensorCommand &startSensorCommand) {
  bool success = true;
  uint8_t sensorType = startSensorCommand.chreSensorType;
  uint64_t interval = startSensorCommand.samplingIntervalInNs;
  uint64_t latency = startSensorCommand.samplingMaxLatencyInNs;
  uint32_t handle;
  if (!chreSensorFindDefault(sensorType, &handle)) {
    LOGE("Could not find default sensor for sensorType %" PRIu8, sensorType);
    success = false;
    // TODO(b/146052784): Test other sensor configure modes
  } else {
    if (!chreSensorConfigure(handle, CHRE_SENSOR_CONFIGURE_MODE_CONTINUOUS,
                             interval, latency)) {
      LOGE("Error configuring sensor with sensorType %" PRIu8
           ", interval %" PRIu64 "ns, and latency %" PRIu64 "ns",
           sensorType, interval, latency);
      success = false;
    } else {
      // Copy hostEndpoint param from previous version of cross validator
      // state
      mCrossValidatorState = CrossValidatorState(
          CrossValidatorType::SENSOR, sensorType, handle, chreGetTime(),
          mCrossValidatorState->hostEndpoint);
      LOGD("Sensor with sensor type %" PRIu8 " configured", sensorType);
    }
  }
  return success;
}

bool Manager::isValidHeader(const chreSensorDataHeader &header) {
  return header.readingCount > 0 && mCrossValidatorState.has_value() &&
         header.baseTimestamp > mCrossValidatorState->timeStart;
}

void Manager::handleStartMessage(const chreMessageFromHostData *hostData) {
  bool success = true;
  if (hostData->hostEndpoint != CHRE_HOST_ENDPOINT_UNSPECIFIED) {
    // Default values for everything but hostEndpoint param
    mCrossValidatorState = CrossValidatorState(CrossValidatorType::SENSOR, 0, 0,
                                               0, hostData->hostEndpoint);
  } else {
    // Default values for everything but hostEndpoint param
    mCrossValidatorState = CrossValidatorState(CrossValidatorType::SENSOR, 0, 0,
                                               0, CHRE_HOST_ENDPOINT_BROADCAST);
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
        success =
            handleStartSensorMessage(startCommand.command.startSensorCommand);
        break;
      default:
        LOGE("Unknown start command type %" PRIu8, startCommand.which_command);
    }
  }
  // If error occurred in validation setup then resetting mCrossValidatorState
  // will alert the event handler
  if (!success) {
    mCrossValidatorState.reset();
  }
}

void Manager::handleMessageFromHost(uint32_t senderInstanceId,
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

chre_cross_validation_Data Manager::makeSensorThreeAxisData(
    const chreSensorThreeAxisData *threeAxisDataFromChre, uint8_t sensorType) {
  chre_cross_validation_SensorData newThreeAxisData = {
      .has_chreSensorType = true,
      .chreSensorType = sensorType,
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

chre_cross_validation_Data Manager::makeSensorFloatData(
    const chreSensorFloatData *floatDataFromChre, uint8_t sensorType) {
  chre_cross_validation_SensorData newfloatData = {
      .has_chreSensorType = true,
      .chreSensorType = sensorType,
      .has_accuracy = true,
      .accuracy = floatDataFromChre->header.accuracy,
      .datapoints = {
          .funcs = {.encode = encodeFloatSensorDatapoints},
          .arg = const_cast<chreSensorFloatData *>(floatDataFromChre)}};
  chre_cross_validation_Data newData = {
      .which_data = chre_cross_validation_Data_sensorData_tag,
      .data =
          {
              .sensorData = newfloatData,
          },
  };
  return newData;
}

void Manager::handleSensorThreeAxisData(
    const chreSensorThreeAxisData *threeAxisDataFromChre, uint8_t sensorType) {
  if (processSensorData(threeAxisDataFromChre->header, sensorType)) {
    chre_cross_validation_Data newData =
        makeSensorThreeAxisData(threeAxisDataFromChre, sensorType);
    encodeAndSendDataToHost(newData);
  }
}

void Manager::handleSensorFloatData(
    const chreSensorFloatData *floatDataFromChre, uint8_t sensorType) {
  if (processSensorData(floatDataFromChre->header, sensorType)) {
    chre_cross_validation_Data newData =
        makeSensorFloatData(floatDataFromChre, sensorType);
    encodeAndSendDataToHost(newData);
  }
}

void Manager::encodeAndSendDataToHost(const chre_cross_validation_Data &data) {
  size_t encodedSize;
  if (!pb_get_encoded_size(&encodedSize, chre_cross_validation_Data_fields,
                           &data)) {
    LOGE("Could not get encoded size of data proto message");
  } else {
    pb_byte_t *buffer = static_cast<pb_byte_t *>(chreHeapAlloc(encodedSize));
    if (buffer == nullptr) {
      LOG_OOM();
    } else {
      pb_ostream_t ostream = pb_ostream_from_buffer(buffer, encodedSize);
      if (!pb_encode(&ostream, chre_cross_validation_Data_fields, &data)) {
        LOGE("Could not encode data proto message");
      } else if (
          !chreSendMessageToHostEndpoint(
              static_cast<void *>(buffer), encodedSize,
              chre_cross_validation_MessageType_CHRE_CROSS_VALIDATION_DATA,
              mCrossValidatorState->hostEndpoint, heapFreeMessageCallback)) {
        LOGE("Could not send message to host");
      }
    }
  }
}

bool Manager::processSensorData(const chreSensorDataHeader &header,
                                uint8_t sensorType) {
  if (!isValidHeader(header)) {
    LOGE("Invalid data being thrown away");
  } else if (!mCrossValidatorState.has_value()) {
    LOGE("Start message not received or invalid when data received");
  } else if (!sensorTypeIsValid(sensorType)) {
    LOGE("Unexpected sensor data type %" PRIu8 ", expected %" PRIu8, sensorType,
         mCrossValidatorState->sensorType);
  } else {
    return true;
  }
  return false;
}

bool Manager::sensorTypeIsValid(uint8_t sensorType) {
  return sensorType == mCrossValidatorState->sensorType;
}

}  // namespace cross_validator

}  // namespace chre
