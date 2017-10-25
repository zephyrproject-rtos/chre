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

extern "C" {

#include "qmi_client.h"
#include "sns_client_api_v01.h"

}  // extern "C"

#include "chre_api/chre/sensor.h"
#include "chre/core/event_loop_manager.h"
#include "chre/core/sensor.h"
#include "chre/platform/assert.h"
#include "chre/platform/fatal_error.h"
#include "chre/platform/log.h"
#include "chre/platform/system_time.h"

namespace chre {
namespace {

//! Timeout for QMI client initialization, in milliseconds. Allow more time here
//! due to external dependencies that may block initialization of SEE.
constexpr uint32_t kQmiInitTimeoutMs = 5000;

//! The QMI sensor service client handle.
qmi_client_type gPlatformSensorServiceQmiClientHandle = nullptr;

/**
 * This callback is invoked by the QMI framework when an asynchronous message is
 * delivered. Unhandled messages are logged. The signature is defined by the QMI
 * library.
 *
 * @param userHandle The userHandle is used by the QMI library.
 * @param messageId The type of the message to decode.
 * @param buffer The buffer to decode.
 * @param bufferLength The length of the buffer to decode.
 * @param callbackData Data that is provided as a context to this callback. This
 *                     is not used in this context.
 */
void platformSensorServiceQmiIndicationCallback(void *userHandle,
                                                unsigned int messageId,
                                                void *buffer,
                                                unsigned int bufferLength,
                                                void *callbackData) {
  // TODO: Implement this.
  switch (messageId) {
    default:
      LOGW("Received unhandled sensor service message: 0x%x", messageId);
      break;
  };
}

}  // anonymous namespace

PlatformSensor::~PlatformSensor() {
  if (lastEvent != nullptr) {
    LOGD("Releasing lastEvent: 0x%p, size %zu",
         lastEvent, lastEventSize);
    memoryFree(lastEvent);
  }
}

void PlatformSensor::init() {
  qmi_idl_service_object_type snsSvcObj =
      SNS_CLIENT_SVC_get_service_object_v01();
  if (snsSvcObj == nullptr) {
    FATAL_ERROR("Failed to obtain the SNS service instance");
  }

  // TODO(P2-aa0089): Replace QMI with an interface that doesn't introduce big
  // image wakeups.
  qmi_client_os_params sensorContextOsParams;
  qmi_client_error_type status = qmi_client_init_instance(snsSvcObj,
      QMI_CLIENT_INSTANCE_ANY, &platformSensorServiceQmiIndicationCallback,
      nullptr, &sensorContextOsParams, kQmiInitTimeoutMs,
      &gPlatformSensorServiceQmiClientHandle);
  if (status != QMI_NO_ERR) {
    FATAL_ERROR("Failed to initialize the sensor service QMI client: %d",
                status);
  }
}

void PlatformSensor::deinit() {
  qmi_client_error_type err = qmi_client_release(
      gPlatformSensorServiceQmiClientHandle);
  if (err != QMI_NO_ERR) {
    LOGE("Failed to release SensorService QMI client: %d", err);
  }
  gPlatformSensorServiceQmiClientHandle = nullptr;
}

bool PlatformSensor::getSensors(DynamicVector<Sensor> *sensors) {
  CHRE_ASSERT(sensors);

  // TODO: Implement this.
  return false;
}

bool PlatformSensor::applyRequest(const SensorRequest& request) {
  // TODO: Implement this.
  return false;
}

SensorType PlatformSensor::getSensorType() const {
  // TODO: Implement this.
  return SensorType::Unknown;
}

uint64_t PlatformSensor::getMinInterval() const {
  return minInterval;
}

const char *PlatformSensor::getSensorName() const {
  return sensorName;
}

PlatformSensor::PlatformSensor(PlatformSensor&& other) {
  // Our move assignment operator doesn't assume that "this" is initialized, so
  // we can just use that here.
  *this = std::move(other);
}

PlatformSensor& PlatformSensor::operator=(PlatformSensor&& other) {
  // TODO: Implement this.
  // Note: if this implementation is ever changed to depend on "this" containing
  // initialized values, the move constructor implemenation must be updated.
  memcpy(sensorName, other.sensorName, 1);
  minInterval = other.minInterval;

  lastEvent = other.lastEvent;
  other.lastEvent = nullptr;

  lastEventSize = other.lastEventSize;
  other.lastEventSize = 0;

  lastEventValid = other.lastEventValid;
  isSensorOff = other.isSensorOff;
  samplingStatus = other.samplingStatus;

  return *this;
}

ChreSensorData *PlatformSensor::getLastEvent() const {
  return (this->lastEventValid) ? this->lastEvent : nullptr;
}

bool PlatformSensor::getSamplingStatus(
    struct chreSensorSamplingStatus *status) const {
  CHRE_ASSERT(status);

  bool success = false;
  if (status != nullptr) {
    success = true;
    memcpy(status, &samplingStatus, sizeof(*status));
  }
  return success;
}

void PlatformSensorBase::setLastEvent(const ChreSensorData *event) {
  memcpy(this->lastEvent, event, this->lastEventSize);
  this->lastEventValid = true;
}

}  // namespace chre
