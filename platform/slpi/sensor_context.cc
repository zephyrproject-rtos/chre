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

extern "C" {

#include "qmi_client.h"
#include "sns_smgr_api_v01.h"

}  // extern "C"

#include "chre/platform/fatal_error.h"
#include "chre/platform/log.h"
#include "chre/platform/sensor_context.h"

namespace chre {

//! The timeout for QMI messages in milliseconds.
constexpr uint32_t kQmiTimeoutMs = 1000;

//! The QMI client handle.
qmi_client_type gSensorContextQmiClientHandle = nullptr;

void SensorContextQmiIndicationCallback(void *userHandle,
                                        unsigned int messageId,
                                        void *buffer, unsigned int bufferLength,
                                        void *callbackData) {
  LOGD("Received indication callback");
}

void SensorContext::init() {
  qmi_idl_service_object_type sensorServiceObject =
      SNS_SMGR_SVC_get_service_object_v01();
  if (sensorServiceObject == nullptr) {
    FATAL_ERROR("Failed to obtain the SNS SMGR service instance");
  }

  qmi_client_os_params sensorContextOsParams;
  qmi_client_error_type status = qmi_client_init_instance(sensorServiceObject,
      QMI_CLIENT_INSTANCE_ANY, &SensorContextQmiIndicationCallback, nullptr,
      &sensorContextOsParams, kQmiTimeoutMs, &gSensorContextQmiClientHandle);
  if (status != QMI_NO_ERR) {
    FATAL_ERROR("Failed to initialize the sensors QMI client");
  }
}

}  // namespace chre
