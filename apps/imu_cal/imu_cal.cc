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

// Dynamic sensor latency settings.
constexpr uint64_t kHighPerformanceLatency =
    Milliseconds(250).toRawNanoseconds();

constexpr uint64_t kStandByLatency = Seconds(1).toRawNanoseconds();

// Defines the indices for the following sensor array definition.
enum SensorIndex {
  SENSOR_INDEX_TEMP = 0,
  SENSOR_INDEX_ACCEL = 1,
  SENSOR_INDEX_GYRO = 2,
  SENSOR_INDEX_MAG = 3,
};

SensorState sensors[] = {
  [SENSOR_INDEX_TEMP] = {
    .type = CHRE_SENSOR_TYPE_ACCELEROMETER_TEMPERATURE,
    .enable = true,
    .interval = Seconds(1).toRawNanoseconds(),
    .latency = 0,
  },
  [SENSOR_INDEX_ACCEL] = {
    .type = CHRE_SENSOR_TYPE_UNCALIBRATED_ACCELEROMETER,
    .enable = true,
    .interval = Milliseconds(10).toRawNanoseconds(),
    .latency = kStandByLatency,
  },
  [SENSOR_INDEX_GYRO] = {
    .type = CHRE_SENSOR_TYPE_UNCALIBRATED_GYROSCOPE,
    .enable = true,
    .interval = Milliseconds(10).toRawNanoseconds(),
    .latency = kStandByLatency,
  },
  [SENSOR_INDEX_MAG] = {
    .type = CHRE_SENSOR_TYPE_UNCALIBRATED_GEOMAGNETIC_FIELD,
    .enable = true,
    .interval = Milliseconds(20).toRawNanoseconds(),
    .latency = kStandByLatency,
  },
};

// Container for all runtime calibration algorithms.
nano_calibration::NanoSensorCal nanoCal;

// Configures the Nanoapp's sensors with special adjustment of accel/gyro/mag
// sensor latency based on whether the gyro is enabled.
void nanoappDynamicConfigure() {
  // Determines if the gyro is active.
  bool gyro_is_on = false;
  struct chreSensorSamplingStatus status;
  if (chreGetSensorSamplingStatus(sensors[SENSOR_INDEX_GYRO].handle, &status)) {
    gyro_is_on = status.enabled;
  } else {
    LOGE("nanoappDynamicConfigure: Failed to get gyro sampling status.");
  }

  LOGD("Dynamic sensor configuration: %s.",
       (gyro_is_on) ? "high-performance" : "stand-by");

  // Configures all sensors.
  for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
    SensorState &sensor = sensors[i];
    if (!sensor.enable) {
      // Only configure enabled sensors.
      continue;
    }

    if (sensor.type == CHRE_SENSOR_TYPE_ACCELEROMETER_TEMPERATURE &&
        !gyro_is_on) {
      // Turn off temperature when gyro is not active.
      chreSensorConfigureModeOnly(sensor.handle,
                                  CHRE_SENSOR_CONFIGURE_MODE_DONE);
    } else {
      // Update the accel/gyro/mag latency according to the gyro's state.
      uint64_t updated_latency =
          (sensor.type == CHRE_SENSOR_TYPE_ACCELEROMETER_TEMPERATURE)
              ? sensor.latency
              : (gyro_is_on) ? kHighPerformanceLatency : kStandByLatency;

      bool config_status = chreSensorConfigure(
          sensor.handle, CHRE_SENSOR_CONFIGURE_MODE_PASSIVE_CONTINUOUS,
          sensor.interval, updated_latency);

      // TODO: Handle error condition.
      LOGD("Requested Config.: handle %" PRIu32 ", interval %" PRIu64
           " nanos, latency %" PRIu64 " nanos, %s",
           sensor.handle, sensor.interval, updated_latency,
           config_status ? "success" : "failure");
    }
  }
}
}  // namespace

bool nanoappStart() {
  LOGI("App started on platform ID %" PRIx64, chreGetPlatformId());

  // Initialize all sensors to populate their handles.
  for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
    SensorState &sensor = sensors[i];
    sensor.isInitialized = chreSensorFindDefault(sensor.type, &sensor.handle);

    // TODO: Handle error condition.
    if (!sensor.isInitialized) {
      LOGE("Sensor handle %" PRIu32 " failed to initialize.", sensor.handle);
    }
  }

  // Configure the Nanoapp's sensors.
  nanoappDynamicConfigure();

  // Checks to see if the accelerometer and magnetometer were initialized.
  bool accelIsInitialized = sensors[SENSOR_INDEX_ACCEL].isInitialized;
  bool magIsInitialized = sensors[SENSOR_INDEX_MAG].isInitialized;

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

    case CHRE_EVENT_SENSOR_SAMPLING_CHANGE: {
      const auto *ev =
          static_cast<const chreSensorSamplingStatusEvent *>(eventData);

      // Is this the gyro? Check the handle.
      if (sensors[SENSOR_INDEX_GYRO].isInitialized &&
          ev->sensorHandle == sensors[SENSOR_INDEX_GYRO].handle) {
        // Modify sensor latency based on whether Gyro is enabled.
        nanoappDynamicConfigure();
      }
      break;
    }

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
