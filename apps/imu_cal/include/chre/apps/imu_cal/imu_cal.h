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

#ifndef CHRE_APPS_IMU_CAL_IMU_CAL_H_
#define CHRE_APPS_IMU_CAL_IMU_CAL_H_

#include <cstdint>

namespace chre {
namespace app {

/**
 * The primary entry point of an imu cal nanoapp. This nanoapp starts and
 * requests imu temerature(s) and uncalibrated accel, gyro and mag data. It
 * then estimates the bias and correct it.
 *
 * @return This app always returns true to indicate success.
 */
bool imuCalStart();

/**
 * The handle event entry point for the imu cal program.
 *
 * @param the sender instance ID
 * @param the type of the event data
 * @param a pointer to the event data
 */
void imuCalHandleEvent(uint32_t senderInstanceId,
                       uint16_t eventType,
                       const void *eventData);

/**
 * Stops the imu cal app.
 */
void imuCalStop();

}  // namespace app
}  // namespace chre

#endif  // CHRE_APPS_IMU_CAL_IMU_CAL_H_
