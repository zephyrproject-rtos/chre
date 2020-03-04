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

#ifndef CHRE_CROSS_VALIDATOR_MANAGER_H_
#define CHRE_CROSS_VALIDATOR_MANAGER_H_

#include <chre.h>
#include <pb_encode.h>

#include "chre_cross_validation.nanopb.h"

#include "chre/util/optional.h"
#include "chre/util/singleton.h"

namespace chre {

namespace cross_validator {

// TODO(b/146052784): Break up the Manager class into more fine-grained classes
// to avoid it becoming to complex.

class Manager {
 public:
  ~Manager();

  void handleEvent(uint32_t senderInstanceId, uint16_t eventType,
                   const void *eventData);

 private:
  enum class CrossValidatorType { SENSOR };

  struct CrossValidatorState {
    // Set upon received start message and read when nanoapp ends to handle
    // cleanup
    CrossValidatorType crossValidatorType;
    // Set when start message is received and default sensor is found for
    // requested sensor type and read when the sensor configuration is being
    // cleaned up. Unused in non-sensor type validations
    uint32_t sensorHandle;
    // The host endpoint which is read from the start message and used when
    // sending data back to AP.
    uint64_t timeStart;
    // The host endpoint which is read from the start message and used when
    // sending data back to AP.
    uint16_t hostEndpoint = CHRE_HOST_ENDPOINT_BROADCAST;

    CrossValidatorState(CrossValidatorType crossValidatorTypeIn,
                        uint32_t sensorHandleIn, uint64_t timeStartIn,
                        uint16_t hostEndpointIn)
        : crossValidatorType(crossValidatorTypeIn),
          sensorHandle(sensorHandleIn),
          timeStart(timeStartIn),
          hostEndpoint(hostEndpointIn) {}
  };

  // Unset if start message was not received or error while processing start
  // message
  chre::Optional<CrossValidatorState> mCrossValidatorState;

  static bool encodeThreeAxisSensorDatapointValues(pb_ostream_t *stream,
                                                   const pb_field_t * /*field*/,
                                                   void *const *arg);

  static chre_cross_validation_SensorDatapoint makeDatapoint(
      const chreSensorThreeAxisData::chreSensorThreeAxisSampleData
          &sampleDataFromChre,
      uint64_t currentTimestamp);

  static bool encodeThreeAxisSensorDatapoints(pb_ostream_t *stream,
                                              const pb_field_t * /*field*/,
                                              void *const *arg);

  bool handleStartSensorMessage(
      const chre_cross_validation_StartSensorCommand &startSensorCommand);

  bool isValidHeader(const chreSensorDataHeader &header);

  void handleStartMessage(const chreMessageFromHostData *hostData);

  void handleMessageFromHost(uint32_t senderInstanceId,
                             const chreMessageFromHostData *hostData);

  chre_cross_validation_Data makeAccelSensorData(
      const chreSensorThreeAxisData *threeAxisDataFromChre);

  void handleSensorThreeAxisData(
      const chreSensorThreeAxisData *threeAxisDataFromChre);

  void cleanup();
};

typedef chre::Singleton<Manager> ManagerSingleton;

}  // namespace cross_validator

}  // namespace chre

#endif  // CHRE_CROSS_VALIDATOR_MANAGER_H_
