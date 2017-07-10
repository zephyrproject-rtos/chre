/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <cinttypes>

#include <chre.h>
#include "chre/util/macros.h"
#include "chre/util/nanoapp/log.h"
#include "chre/util/nanoapp/sensor.h"
#include "chre/util/time.h"
#include "nano_calibration.h"

#define LOG_TAG "[ImuCal]"

#ifdef CHRE_NANOAPP_INTERNAL
namespace chre {
namespace {
#endif  // CHRE_NANOAPP_INTERNAL

namespace {

struct SensorState {
  const uint8_t type;
  uint32_t handle;
  bool isInitialized;
  bool enable;
  uint64_t interval;  // nsec
  uint64_t latency;   // nsec
  chreSensorInfo info;
};

SensorState sensors[] = {
    {
        .type = CHRE_SENSOR_TYPE_ACCELEROMETER_TEMPERATURE,
        .enable = true,
        .interval = Seconds(1).toRawNanoseconds(),
        .latency = 0,
    },
    {
        .type = CHRE_SENSOR_TYPE_UNCALIBRATED_ACCELEROMETER,
        .enable = true,
        .interval = Milliseconds(10).toRawNanoseconds(),
        .latency = Milliseconds(250).toRawNanoseconds(),
    },
    {
        .type = CHRE_SENSOR_TYPE_UNCALIBRATED_GYROSCOPE,
        .enable = true,
        .interval = Milliseconds(10).toRawNanoseconds(),
        .latency = Milliseconds(250).toRawNanoseconds(),
    },
    {
        .type = CHRE_SENSOR_TYPE_UNCALIBRATED_GEOMAGNETIC_FIELD,
        .enable = true,
        .interval = Milliseconds(20).toRawNanoseconds(),
        .latency = Milliseconds(250).toRawNanoseconds(),
    },
};

// Container for all runtime calibration algorithms.
nano_calibration::NanoSensorCal nanoCal;

}  // namespace

bool nanoappStart() {
  LOGI("App started on platform ID %" PRIx64, chreGetPlatformId());

  bool accelIsInitialized = false;
  bool magIsInitialized = false;
  for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
    SensorState& sensor = sensors[i];
    sensor.isInitialized = chreSensorFindDefault(sensor.type, &sensor.handle);

    // TODO: Handle error condition.
    LOGI("sensor %d initialized: %s with handle %" PRIu32, i,
         sensor.isInitialized ? "true" : "false", sensor.handle);

    if (sensor.isInitialized) {
      // Checks to see if the accelerometer was initialized.
      accelIsInitialized |=
          (sensor.type == CHRE_SENSOR_TYPE_UNCALIBRATED_ACCELEROMETER);

      // Checks to see if the magnetometer was initialized.
      magIsInitialized |=
          (sensor.type == CHRE_SENSOR_TYPE_UNCALIBRATED_GEOMAGNETIC_FIELD);
    } else {
      LOGE("chreSensorFindDefault failed");
    }

    // Passively subscribe to sensors.
    if (sensor.enable) {
      float odrHz = 1e9f / sensor.interval;
      float latencySec = sensor.latency / 1e9f;
      bool status = chreSensorConfigure(
          sensor.handle, CHRE_SENSOR_CONFIGURE_MODE_PASSIVE_CONTINUOUS,
          sensor.interval, sensor.latency);

      // TODO: Handle error condition.
      LOGI("Requested data: odr %f Hz, latency %f sec, %s", odrHz, latencySec,
           status ? "success" : "failure");
    }
  }

  // Checks for the minimimal conditions for a nanoCal to have an active
  // calibration algorithm running.
  //  Sensor Requirements:
  //  - GyroCal:  accelerometer, gyroscope, magnetometer [optional]
  //  - OTC-Gyro: GyroCal required sensors + temperature
  //  - AccelCal: accelerometer
  //  - MagCal:   magnetometer
  if (accelIsInitialized || magIsInitialized) {
    nanoCal.Initialize();
  } else {
    LOGE(
        "None of the required sensors to enable a runtime calibration were "
        "successfully initialized.");
  }

  return true;
}

void nanoappHandleEvent(uint32_t senderInstanceId, uint16_t eventType,
                        const void *eventData) {
  switch (eventType) {
    case CHRE_EVENT_SENSOR_UNCALIBRATED_ACCELEROMETER_DATA:
    case CHRE_EVENT_SENSOR_UNCALIBRATED_GYROSCOPE_DATA:
    case CHRE_EVENT_SENSOR_UNCALIBRATED_GEOMAGNETIC_FIELD_DATA: {
      nanoCal.HandleSensorSamples(
          eventType, static_cast<const chreSensorThreeAxisData *>(eventData));
      break;
    }

    case CHRE_EVENT_SENSOR_ACCELEROMETER_TEMPERATURE_DATA: {
      nanoCal.HandleTemperatureSamples(
          eventType, static_cast<const chreSensorFloatData *>(eventData));
      break;
    }

    case CHRE_EVENT_SENSOR_SAMPLING_CHANGE:
      break;

    default:
      LOGW("Unhandled event %d", eventType);
      break;
  }
}

void nanoappEnd() {
  // TODO: Unsubscribe to sensors
  LOGI("Stopped");
}

#ifdef CHRE_NANOAPP_INTERNAL
}  // anonymous namespace
}  // namespace chre

#include "chre/platform/static_nanoapp_init.h"
#include "chre/util/nanoapp/app_id.h"

CHRE_STATIC_NANOAPP_INIT(ImuCal, chre::kImuCalAppId, 0);
#endif  // CHRE_NANOAPP_INTERNAL
