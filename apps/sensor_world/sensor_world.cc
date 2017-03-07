/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "chre.h"

#include <cinttypes>

#include "chre/util/array.h"
#include "chre/util/time.h"

namespace chre {
namespace app {
namespace {

struct SensorState {
  const uint8_t type;
  uint32_t handle;
  bool isInitialized;
  bool enable;
  uint64_t interval;  // nsec
  uint64_t latency;  // nsec
  chreSensorInfo info;
};

SensorState sensors[] = {
  { .type = CHRE_SENSOR_TYPE_ACCELEROMETER,
    .enable = true,
    .interval = Milliseconds(80).toRawNanoseconds(),
    .latency = Seconds(4).toRawNanoseconds(),
  },
  { .type = CHRE_SENSOR_TYPE_INSTANT_MOTION_DETECT,
    .enable = false,
  },
  { .type = CHRE_SENSOR_TYPE_STATIONARY_DETECT,
    .enable = false,
  },
  { .type = CHRE_SENSOR_TYPE_GYROSCOPE,
    .enable = true,
    .interval = Milliseconds(80).toRawNanoseconds(),
    .latency = Seconds(4).toRawNanoseconds(),
  },
  { .type = CHRE_SENSOR_TYPE_GEOMAGNETIC_FIELD,
    .enable = true,
    .interval = Milliseconds(80).toRawNanoseconds(),
    .latency = Seconds(4).toRawNanoseconds(),
  },
  { .type = CHRE_SENSOR_TYPE_PRESSURE,
    .enable = true,
    .interval = Milliseconds(200).toRawNanoseconds(),
    .latency = Seconds(4).toRawNanoseconds(),
  },
  { .type = CHRE_SENSOR_TYPE_LIGHT,
    .enable = true,
    .interval = Milliseconds(200).toRawNanoseconds(),
    .latency = 0,
  },
  { .type = CHRE_SENSOR_TYPE_PROXIMITY,
    .enable = true,
    .interval = Milliseconds(200).toRawNanoseconds(),
    .latency = 0,
  },
  { .type = CHRE_SENSOR_TYPE_ACCELEROMETER_TEMPERATURE,
    .enable = true,
    .interval = Seconds(2).toRawNanoseconds(),
    .latency = 0,
  },
  { .type = CHRE_SENSOR_TYPE_GYROSCOPE_TEMPERATURE,
    .enable = true,
    .interval = Seconds(2).toRawNanoseconds(),
    .latency = 0,
  },
  { .type = CHRE_SENSOR_TYPE_UNCALIBRATED_ACCELEROMETER,
    .enable = true,
    .interval = Milliseconds(80).toRawNanoseconds(),
    .latency = Seconds(4).toRawNanoseconds(),
  },
  { .type = CHRE_SENSOR_TYPE_UNCALIBRATED_GYROSCOPE,
    .enable = true,
    .interval = Milliseconds(80).toRawNanoseconds(),
    .latency = Seconds(4).toRawNanoseconds(),
  },
  { .type = CHRE_SENSOR_TYPE_UNCALIBRATED_GEOMAGNETIC_FIELD,
    .enable = true,
    .interval = Milliseconds(80).toRawNanoseconds(),
    .latency = Seconds(4).toRawNanoseconds(),
  },
};

const char *getSensorName(uint32_t eventType) {
  switch (eventType) {
    case CHRE_EVENT_SENSOR_ACCELEROMETER_DATA:
      return "Accel";
    case CHRE_EVENT_SENSOR_UNCALIBRATED_ACCELEROMETER_DATA:
      return "Uncal Accel";
    case CHRE_EVENT_SENSOR_GYROSCOPE_DATA:
      return "Gyro";
    case CHRE_EVENT_SENSOR_UNCALIBRATED_GYROSCOPE_DATA:
      return "Uncal Gyro";
    case CHRE_EVENT_SENSOR_GEOMAGNETIC_FIELD_DATA:
      return "Mag";
    case CHRE_EVENT_SENSOR_UNCALIBRATED_GEOMAGNETIC_FIELD_DATA:
      return "Uncal Mag";
    case CHRE_EVENT_SENSOR_PRESSURE_DATA:
      return "Baro";
    case CHRE_EVENT_SENSOR_LIGHT_DATA:
      return "Light";
    case CHRE_EVENT_SENSOR_PROXIMITY_DATA:
      return "Prox";
    case CHRE_EVENT_SENSOR_ACCELEROMETER_TEMPERATURE_DATA:
      return "Accel Temp";
    case CHRE_EVENT_SENSOR_GYROSCOPE_TEMPERATURE_DATA:
      return "Gyro Temp";
    default:
      return "Unknown";
  }
}

} // namespace

bool sensorWorldStart() {
  chreLog(CHRE_LOG_INFO, "Sensor World! - App started on platform ID %" PRIx64,
          chreGetPlatformId());

  for (size_t i = 0; i < ARRAY_SIZE(sensors); i++) {
    SensorState& sensor = sensors[i];
    sensor.isInitialized = chreSensorFindDefault(sensor.type, &sensor.handle);
    chreLog(CHRE_LOG_INFO, "sensor %d initialized: %s with handle %" PRIu32,
            i, sensor.isInitialized ? "true" : "false", sensor.handle);

    if (sensor.isInitialized) {
      // Get sensor info
      chreSensorInfo& info = sensor.info;
      bool infoStatus = chreGetSensorInfo(sensor.handle, &info);
      if (infoStatus) {
        chreLog(CHRE_LOG_INFO, "SensorInfo: %s, Type=%" PRIu8 " OnChange=%d"
                " OneShot=%d minInterval=%" PRIu64 "nsec",
                info.sensorName, info.sensorType, info.isOnChange,
                info.isOneShot, info.minInterval);
      } else {
        chreLog(CHRE_LOG_ERROR, "chreGetSensorInfo failed");
      }


      // Subscribe to sensors
      if (sensor.enable) {
        float odrHz = 1e9 / sensor.interval;
        float latencySec = sensor.latency / 1e9;
        bool status = chreSensorConfigure(sensor.handle,
            CHRE_SENSOR_CONFIGURE_MODE_CONTINUOUS, sensor.interval,
            sensor.latency);
        chreLog(CHRE_LOG_INFO, "Requested data: odr %f Hz, latency %f sec, %s",
                odrHz, latencySec, status ? "success" : "failure");
      }
    }
  }

  return true;
}

void sensorWorldHandleEvent(uint32_t senderInstanceId,
                            uint16_t eventType,
                            const void *eventData) {
  switch (eventType) {
    case CHRE_EVENT_SENSOR_ACCELEROMETER_DATA:
    case CHRE_EVENT_SENSOR_UNCALIBRATED_ACCELEROMETER_DATA:
    case CHRE_EVENT_SENSOR_GYROSCOPE_DATA:
    case CHRE_EVENT_SENSOR_UNCALIBRATED_GYROSCOPE_DATA:
    case CHRE_EVENT_SENSOR_GEOMAGNETIC_FIELD_DATA:
    case CHRE_EVENT_SENSOR_UNCALIBRATED_GEOMAGNETIC_FIELD_DATA: {
      const auto *ev = static_cast<const chreSensorThreeAxisData *>(eventData);
      const auto header = ev->header;
      const auto *data = ev->readings;

      float x = 0, y = 0, z = 0;
      for (size_t i = 0; i < header.readingCount; i++) {
        x += data[i].v[0];
        y += data[i].v[1];
        z += data[i].v[2];
      }
      x /= header.readingCount;
      y /= header.readingCount;
      z /= header.readingCount;

      chreLog(CHRE_LOG_INFO, "%s, %d samples: %f %f %f",
              getSensorName(eventType), header.readingCount, x, y, z);
      break;
    }

    case CHRE_EVENT_SENSOR_PRESSURE_DATA:
    case CHRE_EVENT_SENSOR_LIGHT_DATA:
    case CHRE_EVENT_SENSOR_PROXIMITY_DATA:
    case CHRE_EVENT_SENSOR_ACCELEROMETER_TEMPERATURE_DATA:
    case CHRE_EVENT_SENSOR_GYROSCOPE_TEMPERATURE_DATA: {
      const auto *ev = static_cast<const chreSensorFloatData *>(eventData);
      const auto header = ev->header;

      float v = 0;
      for (size_t i = 0; i < header.readingCount; i++) {
        v += ev->readings[i].value;
      }
      v /= header.readingCount;

      chreLog(CHRE_LOG_INFO, "%s, %d samples: %f",
              getSensorName(eventType), header.readingCount, v);
      break;
    }

    default:
      chreLog(CHRE_LOG_ERROR, "Unhandled event %d", eventType);
      break;
  }
}

void sensorWorldStop() {
  chreLog(CHRE_LOG_INFO, "Sensor World! - Stopped");
}

}  // namespace app
}  // namespace chre
