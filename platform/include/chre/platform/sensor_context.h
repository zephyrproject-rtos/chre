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

#ifndef CHRE_PLATFORM_SENSOR_CONTEXT_H_
#define CHRE_PLATFORM_SENSOR_CONTEXT_H_

#include "chre/util/non_copyable.h"

namespace chre {

/**
 * Provides a mechanism to interact with sensors provided by the platform. This
 * includes requesting sensor data and querying available sensors.
 */
class SensorContext {
 public:
  /**
   * Initializes the platform sensors subsystem. This must be called as part of
   * the initialization of the runtime.
   */
  static void init();
};

}  // namespace chre

#endif  // CHRE_PLATFORM_SENSOR_CONTEXT_H_
