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

#include "chre/platform/platform_sensor.h"

#include "stringl.h"

#include "chre_api/chre/sensor.h"
#include "chre/core/event_loop_manager.h"
#include "chre/core/sensor.h"
#include "chre/platform/assert.h"
#include "chre/platform/fatal_error.h"
#include "chre/platform/log.h"
#include "chre/platform/system_time.h"
#include "chre/platform/slpi/see/see_helper.h"
#include "chre/util/singleton.h"

namespace chre {
namespace {

//! A singleton instance of SeeHelper that can be used for making synchronous
//! sensor requests. This must only be used from the CHRE thread.
typedef Singleton<SeeHelper> SeeHelperSingleton;

// TODO: Complete the list.
//! The list of SEE platform sensor data types that CHRE intends to support.
const char *kSeeDataTypes[] = {
  "accel",
  "gyro",
  "mag",
  "pressure",
};

// TODO: Complete the list.
/**
 * Obtains the sensor type given the specified data type and whether the sensor
 * is runtime-calibrated or not.
 */
SensorType getSensorTypeFromDataType(const char *dataType, bool calibrated) {
  SensorType sensorType;
  if (strcmp(dataType, "accel") == 0) {
    if (calibrated) {
      sensorType = SensorType::Accelerometer;
    } else {
      sensorType = SensorType::UncalibratedAccelerometer;
    }
  } else if (strcmp(dataType, "gyro") == 0) {
    if (calibrated) {
      sensorType = SensorType::Gyroscope;
    } else {
      sensorType = SensorType::UncalibratedGyroscope;
    }
  } else if (strcmp(dataType, "mag") == 0) {
    if (calibrated) {
      sensorType = SensorType::GeomagneticField;
    } else {
      sensorType = SensorType::UncalibratedGeomagneticField;
    }
  } else if (strcmp(dataType, "pressure") == 0) {
    sensorType = SensorType::Pressure;
  } else {
    sensorType= SensorType::Unknown;
  }
  return sensorType;
}

/**
 * The async indication callback of SeeHelper.
 */
void seeHelperIndCb(const sns_std_suid& suid, uint32_t msgId, void *cbData) {
  LOGW("IndCb: Unhandled msg id %" PRIu32, msgId);
}

/**
 * Allocates memory and specifies the memory size for an on-change sensor to
 * store its last data event.
 *
 * @param sensorType The sensorType of this sensor.
 * @param eventSize A non-null pointer to indicate the memory size allocated.
 * @return Pointer to the memory allocated.
 */
ChreSensorData *allocateLastEvent(SensorType sensorType, size_t *eventSize) {
  CHRE_ASSERT(eventSize);

  *eventSize = 0;
  ChreSensorData *event = nullptr;
  if (sensorTypeIsOnChange(sensorType)) {
    SensorSampleType sampleType = getSensorSampleTypeFromSensorType(sensorType);
    switch (sampleType) {
      case SensorSampleType::ThreeAxis:
        *eventSize = sizeof(chreSensorThreeAxisData);
        break;
      case SensorSampleType::Float:
        *eventSize = sizeof(chreSensorFloatData);
        break;
      case SensorSampleType::Byte:
        *eventSize = sizeof(chreSensorByteData);
        break;
      case SensorSampleType::Occurrence:
        *eventSize = sizeof(chreSensorOccurrenceData);
        break;
      default:
        CHRE_ASSERT_LOG(false, "Unhandled sample type");
        break;
    }

    event = static_cast<ChreSensorData *>(memoryAlloc(*eventSize));
    if (event == nullptr) {
      *eventSize = 0;
      FATAL_ERROR("Failed to allocate last event memory for SensorType %d",
                  static_cast<int>(sensorType));
    }
  }
  return event;
}

/**
 * Constructs and initializes a sensor, and adds it to the sensor list.
 *
 * @param suid The SUID of the sensor as provided by SEE.
 * @param sensorType The sensor type of the sensor.
 * @param calibrated Whether the sensor is runtime-calibrated or not.
 * @param attr A reference to SeeAttrbutes.
 * @param sensor The sensor list.
 */
void addSensor(const sns_std_suid& suid, SensorType sensorType, bool calibrated,
               const SeeAttributes& attr, DynamicVector<Sensor> *sensors) {
  // Concatenate vendor and name with a space in between.
  char sensorName[kSensorNameMaxLen];
  strlcpy(sensorName, attr.vendor, sizeof(sensorName));
  strlcat(sensorName, " ", sizeof(sensorName));
  strlcat(sensorName, attr.name, sizeof(sensorName));

  // Override one-shot sensor's minInterval to default
  uint64_t minInterval = sensorTypeIsOneShot(sensorType) ?
      CHRE_SENSOR_INTERVAL_DEFAULT : static_cast<uint64_t>(
          Seconds(1).toRawNanoseconds()
          / static_cast<uint64_t>(attr.maxSampleRate));

  // Allocates memory for on-change sensor's last event.
  size_t lastEventSize;
  ChreSensorData *lastEvent = allocateLastEvent(sensorType, &lastEventSize);

  // Constructs and initializes PlatformSensorBase.
  Sensor sensor;
  sensor.initBase(suid, sensorType, calibrated, minInterval, sensorName,
                  lastEvent, lastEventSize);

  if (!sensors->push_back(std::move(sensor))) {
    FATAL_ERROR("Failed to allocate new sensor: out of memory");
  }
}

}  // anonymous namespace

PlatformSensor::~PlatformSensor() {
  if (mLastEvent != nullptr) {
    LOGD("Releasing lastEvent: 0x%p, size %zu",
         mLastEvent, mLastEventSize);
    memoryFree(mLastEvent);
  }
}

void PlatformSensor::init() {
  SeeHelperSingleton::init();
  SeeHelperSingleton::get()->initService(&seeHelperIndCb);
}

void PlatformSensor::deinit() {
  SeeHelperSingleton::get()->release();
  SeeHelperSingleton::deinit();
}

bool PlatformSensor::getSensors(DynamicVector<Sensor> *sensors) {
  CHRE_ASSERT(sensors);

  DynamicVector<sns_std_suid> suids;
  for (size_t i = 0; i < ARRAY_SIZE(kSeeDataTypes); i++) {
    const char *dataType = kSeeDataTypes[i];

    if (!SeeHelperSingleton::get()->findSuidSync(dataType, &suids)) {
      LOGE("Failed to find sensor '%s'", dataType);
    } else if (suids.empty()) {
      LOGW("No SUID found for '%s'", dataType);
    } else {
      LOGD("Num of SUIDs found for '%s': %zu", dataType, suids.size());
      for (size_t j = 0; j < suids.size(); j++) {
        LOGD("  0x%" PRIx64 " %" PRIx64, suids[j].suid_high, suids[j].suid_low);
      }

      // If there are more than one sensors that support the data type,
      // choose the first one for CHRE.
      SeeAttributes attr;
      if (!SeeHelperSingleton::get()->getAttributesSync(suids[0], &attr)) {
        LOGE("Failed to get attributes of SUID 0x%" PRIx64 " %" PRIx64,
             suids[0].suid_high, suids[0].suid_low);
      } else {
        LOGI("Found %s: %s %s, Max ODR %f Hz",
             attr.type, attr.vendor, attr.name, attr.maxSampleRate);

        SensorType sensorType = getSensorTypeFromDataType(dataType, true);
        addSensor(suids[0], sensorType, true, attr, sensors);

        // Add an uncalibrated version if defined.
        SensorType uncalibratedType =
            getSensorTypeFromDataType(dataType, false);
        if (sensorType != uncalibratedType) {
          addSensor(suids[0], uncalibratedType, false, attr, sensors);
        }
      }
    }
  }
  return true;
}

bool PlatformSensor::applyRequest(const SensorRequest& request) {
  // TODO: Implement this.
  return false;
}

SensorType PlatformSensor::getSensorType() const {
  return mSensorType;
}

uint64_t PlatformSensor::getMinInterval() const {
  return mMinInterval;
}

const char *PlatformSensor::getSensorName() const {
  return mSensorName;
}

PlatformSensor::PlatformSensor(PlatformSensor&& other) {
  // Our move assignment operator doesn't assume that "this" is initialized, so
  // we can just use that here.
  *this = std::move(other);
}

PlatformSensor& PlatformSensor::operator=(PlatformSensor&& other) {
  // Note: if this implementation is ever changed to depend on "this" containing
  // initialized values, the move constructor implemenation must be updated.
  mSuid = other.mSuid;
  memcpy(mSensorName, other.mSensorName, kSensorNameMaxLen);
  mSensorType = other.mSensorType;
  mCalibrated = other.mCalibrated;
  mMinInterval = other.mMinInterval;

  mLastEvent = other.mLastEvent;
  other.mLastEvent = nullptr;

  mLastEventSize = other.mLastEventSize;
  other.mLastEventSize = 0;

  mLastEventValid = other.mLastEventValid;
  mSamplingStatus = other.mSamplingStatus;

  return *this;
}

ChreSensorData *PlatformSensor::getLastEvent() const {
  return mLastEventValid ? mLastEvent : nullptr;
}

bool PlatformSensor::getSamplingStatus(
    struct chreSensorSamplingStatus *status) const {
  CHRE_ASSERT(status);

  bool success = false;
  if (status != nullptr) {
    success = true;
    memcpy(status, &mSamplingStatus, sizeof(*status));
  }
  return success;
}

void PlatformSensorBase::initBase(
    const sns_std_suid& suid, SensorType sensorType, bool calibrated,
    uint64_t minInterval, const char *sensorName,
    ChreSensorData *lastEvent, size_t lastEventSize) {
  mSuid = suid;
  mSensorType = sensorType;
  mCalibrated = calibrated;
  mMinInterval = minInterval;
  memcpy(mSensorName, sensorName, kSensorNameMaxLen);
  mLastEvent = lastEvent;
  mLastEventSize = lastEventSize;

  mSamplingStatus.enabled = false;
  mSamplingStatus.interval = CHRE_SENSOR_INTERVAL_DEFAULT;
  mSamplingStatus.latency = CHRE_SENSOR_LATENCY_DEFAULT;
}

void PlatformSensorBase::setLastEvent(const ChreSensorData *event) {
  memcpy(mLastEvent, event, mLastEventSize);
  mLastEventValid = true;
}

}  // namespace chre
