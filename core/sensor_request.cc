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

#include <algorithm>

#include "chre/core/sensor_request.h"
#include "chre/platform/assert.h"
#include "chre/platform/fatal_error.h"

namespace chre {

const char *getSensorTypeName(SensorType sensorType) {
  switch (sensorType) {
    case SensorType::Unknown:
      return "Unknown";
    case SensorType::Accelerometer:
      return "Accelerometer";
    case SensorType::InstantMotion:
      return "Instant Motion";
    case SensorType::StationaryDetect:
      return "Stationary Detect";
    case SensorType::Gyroscope:
      return "Gyroscope";
    case SensorType::GeomagneticField:
      return "Geomagnetic Field";
    case SensorType::Pressure:
      return "Pressure";
    case SensorType::Light:
      return "Light";
    case SensorType::Proximity:
      return "Proximity";
    default:
      CHRE_ASSERT(false);
      return "";
  }
}

uint16_t getSampleEventTypeForSensorType(SensorType sensorType) {
  if (sensorType == SensorType::Unknown) {
    FATAL_ERROR("Tried to obtain the sensor sample event index for an unknown "
                "sensor type");
  }

  // The enum values of SensorType may not map to the defined values in the
  // CHRE API.
  uint8_t sensorTypeValue = getUnsignedIntFromSensorType(sensorType);
  return CHRE_EVENT_SENSOR_DATA_EVENT_BASE + sensorTypeValue;
}

SensorType getSensorTypeFromUnsignedInt(uint8_t sensorType) {
  switch (sensorType) {
    case CHRE_SENSOR_TYPE_ACCELEROMETER:
      return SensorType::Accelerometer;
    case CHRE_SENSOR_TYPE_INSTANT_MOTION_DETECT:
      return SensorType::InstantMotion;
    case CHRE_SENSOR_TYPE_STATIONARY_DETECT:
      return SensorType::StationaryDetect;
    case CHRE_SENSOR_TYPE_GYROSCOPE:
      return SensorType::Gyroscope;
    case CHRE_SENSOR_TYPE_GEOMAGNETIC_FIELD:
      return SensorType::GeomagneticField;
    case CHRE_SENSOR_TYPE_PRESSURE:
      return SensorType::Pressure;
    case CHRE_SENSOR_TYPE_LIGHT:
      return SensorType::Light;
    case CHRE_SENSOR_TYPE_PROXIMITY:
      return SensorType::Proximity;
    default:
      return SensorType::Unknown;
  }
}

uint8_t getUnsignedIntFromSensorType(SensorType sensorType) {
  switch (sensorType) {
    case SensorType::Accelerometer:
      return CHRE_SENSOR_TYPE_ACCELEROMETER;
    case SensorType::InstantMotion:
      return CHRE_SENSOR_TYPE_INSTANT_MOTION_DETECT;
    case SensorType::StationaryDetect:
      return CHRE_SENSOR_TYPE_STATIONARY_DETECT;
    case SensorType::Gyroscope:
      return CHRE_SENSOR_TYPE_GYROSCOPE;
    case SensorType::GeomagneticField:
      return CHRE_SENSOR_TYPE_GEOMAGNETIC_FIELD;
    case SensorType::Pressure:
      return CHRE_SENSOR_TYPE_PRESSURE;
    case SensorType::Light:
      return CHRE_SENSOR_TYPE_LIGHT;
    case SensorType::Proximity:
      return CHRE_SENSOR_TYPE_PROXIMITY;
    default:
      // Update implementation to prevent undefined or SensorType::Unknown from
      // being used.
      CHRE_ASSERT(false);
      return 0;
  }
}

SensorMode getSensorModeFromEnum(enum chreSensorConfigureMode enumSensorMode) {
  switch (enumSensorMode) {
    case CHRE_SENSOR_CONFIGURE_MODE_DONE:
      return SensorMode::Off;
    case CHRE_SENSOR_CONFIGURE_MODE_CONTINUOUS:
      return SensorMode::ActiveContinuous;
    case CHRE_SENSOR_CONFIGURE_MODE_ONE_SHOT:
      return SensorMode::ActiveOneShot;
    case CHRE_SENSOR_CONFIGURE_MODE_PASSIVE_CONTINUOUS:
      return SensorMode::PassiveContinuous;
    case CHRE_SENSOR_CONFIGURE_MODE_PASSIVE_ONE_SHOT:
      return SensorMode::PassiveOneShot;
    default:
      // Default to off since it is the least harmful and has no power impact.
      return SensorMode::Off;
  }
}

SensorRequest::SensorRequest()
    : SensorRequest(SensorMode::Off,
                    Nanoseconds(CHRE_SENSOR_INTERVAL_DEFAULT),
                    Nanoseconds(CHRE_SENSOR_LATENCY_DEFAULT)) {}

SensorRequest::SensorRequest(SensorMode mode,
                             Nanoseconds interval,
                             Nanoseconds latency)
    : mInterval(interval), mLatency(latency), mMode(mode) {}

SensorRequest::SensorRequest(Nanoapp *nanoapp, SensorMode mode,
                             Nanoseconds interval,
                             Nanoseconds latency)
    : mNanoapp(nanoapp), mInterval(interval), mLatency(latency), mMode(mode) {}

bool SensorRequest::isEquivalentTo(const SensorRequest& request) const {
  return (mMode == request.mMode
      && mInterval == request.mInterval
      && mLatency == request.mLatency);
}

bool SensorRequest::mergeWith(const SensorRequest& request) {
  bool attributesChanged = false;

  if (request.mInterval < mInterval) {
    mInterval = request.mInterval;
    attributesChanged = true;
  }

  if (request.mLatency < mLatency) {
    mLatency = request.mLatency;
    attributesChanged = true;
  }

  // Compute the highest priority mode. Active continuous is the highest
  // priority and passive one-shot is the lowest.
  SensorMode maximalSensorMode = SensorMode::Off;
  if (mMode == SensorMode::ActiveContinuous
      || request.mMode == SensorMode::ActiveContinuous) {
    maximalSensorMode = SensorMode::ActiveContinuous;
  } else if (mMode == SensorMode::ActiveOneShot
      || request.mMode == SensorMode::ActiveOneShot) {
    maximalSensorMode = SensorMode::ActiveOneShot;
  } else if (mMode == SensorMode::PassiveContinuous
      || request.mMode == SensorMode::PassiveContinuous) {
    maximalSensorMode = SensorMode::PassiveContinuous;
  } else if (mMode == SensorMode::PassiveOneShot
      || request.mMode == SensorMode::PassiveOneShot) {
    maximalSensorMode = SensorMode::PassiveOneShot;
  } else {
    CHRE_ASSERT(false);
  }

  if (mMode != maximalSensorMode) {
    mMode = maximalSensorMode;
    attributesChanged = true;
  }

  return attributesChanged;
}

Nanoseconds SensorRequest::getInterval() const {
  return mInterval;
}

Nanoseconds SensorRequest::getLatency() const {
  return mLatency;
}

SensorMode SensorRequest::getMode() const {
  return mMode;
}

Nanoapp *SensorRequest::getNanoapp() const {
  return mNanoapp;
}

}  // namespace chre
