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

#include "chre/util/nanoapp/log.h"
#include "chre/util/time.h"
#include "chre_cross_validation.nanopb.h"

#define LOG_TAG "ChreCrossValidator"

/* TODO(b/148481242): Send all errors to host as well as just logging them as
 * errors.
 *
 * TODO(b/146052784): Move start and handle data methods for each cross
 * validation type (sensor[accel, gyro, ...], wifi, gps) to a manager class.
 *
 * TODO(b/146052784): Craete a helper function to get string version of
 * sensorType for logging.
 */

namespace chre {

namespace {

void handleStartSensorMessage(
    const chre_cross_validation_StartSensorCommand &startSensorCommand) {
  uint8_t sensorType = startSensorCommand.sensorType;
  uint64_t interval = startSensorCommand.samplingIntervalInNs;
  uint64_t latency = startSensorCommand.samplingMaxLatencyInNs;
  uint32_t handle;
  if (!chreSensorFindDefault(sensorType, &handle)) {
    LOGE("Could not find default sensor for sensorType %" PRIu8, sensorType);
    // TODO(b/146052784): Test other sensor configure modes
  } else if (!chreSensorConfigure(handle, CHRE_SENSOR_CONFIGURE_MODE_CONTINUOUS,
                                  interval, latency)) {
    LOGE("Error configuring sensor with sensorType %" PRIu8
         ", interval %" PRIu64 "ns, and latency %" PRIu64 "ns",
         sensorType, interval, latency);
  } else {
    LOGD("Sensor with sensor type %" PRIu8 " configured", sensorType);
  }
}

void handleStartMessage(const chreMessageFromHostData *hostData) {
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

}  // namespace

extern "C" void nanoappHandleEvent(uint32_t senderInstanceId,
                                   uint16_t eventType, const void *eventData) {
  switch (eventType) {
    case CHRE_EVENT_MESSAGE_FROM_HOST:
      handleMessageFromHost(
          senderInstanceId,
          static_cast<const chreMessageFromHostData *>(eventData));
      break;
    case CHRE_EVENT_SENSOR_ACCELEROMETER_DATA:
      // TODO(b/146052784): Implement
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

extern "C" void nanoappEnd(void) {}

}  // namespace chre
