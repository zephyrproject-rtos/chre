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

#ifndef CHRE_ASH_H_
#define CHRE_ASH_H_

/**
 * @file
 * Defines the interface for the Android Sensor Hub support.
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The values returned by this sensor cannot be trusted, calibration is needed
 * or the environment doesn't allow readings.
 */
#define ASH_CAL_ACCURACY_UNRELIABLE UINT8_C(0)

/**
 * This sensor is reporting data with low accuracy, calibration with the
 * environment is needed.
 */
#define ASH_CAL_ACCURACY_LOW UINT8_C(1)

/**
 * This sensor is reporting data with an average level of accuracy, calibration
 * with the environment may improve the readings.
 */
#define ASH_CAL_ACCURACY_MEDIUM UINT8_C(2)

/**
 * This sensor is reporting data with maximum accuracy.
 */
#define ASH_CAL_ACCURACY_HIGH UINT8_C(3)

/**
 * Calibration info for a sensor which reports on a maximum of three axes.
 *
 * Let Su be the uncalibrated sensor data and Sc the calibrated one,
 * Sc = compMatrix * (Su - bias)
 *
 */
struct ashCalInfo {
  /**
   * The zero-bias vector in the x, y, z order. If the sensor reports on N
   * axes with N < 3, only the first N elements are considered valid.
   */
  float bias[3];

  /**
   * The compensation matrix in the row major order. If the sensor reports on N
   * axes with N < 3, only the first N elements of each row are considered
   * valid.
   */
  float compMatrix[9];

  /**
   * One of the ASH_CAL_ACCURACY_* constants. This corresponds to the
   * definition in the Android SensorManager. See
   * https://developer.android.com/reference/android/hardware/SensorEvent.html#accuracy
   * for more details.
   * Note that this accuracy field is simply a suggestion to the platform and
   * the platform can ignore or over-write it.
   */
  uint8_t accuracy;
};


//! This is used to indicate that the cal params are invalid.
#define ASH_CAL_PARAMS_SOURCE_NONE    UINT8_C(0)

//! This is used to indicate that the cal params were set by factory
//! calibration.
#define ASH_CAL_PARAMS_SOURCE_FACTORY UINT8_C(1)

//! This is used to indicate that the cal params were set by runtime
//! calibration.
#define ASH_CAL_PARAMS_SOURCE_RUNTIME UINT8_C(2)

/**
 * A struct for calibration parameters to be saved to and loaded from a
 * persistent area. The source of each section is indicated by the
 * corresponding *Source field, which is one of the ASH_CAL_PARAMS_SOURCE_*
 * constants.
 */
struct ashCalParams {
  //! The offset of the sensor in the x, y and z axis at temperature
  //! offsetTempCelsius.
  float offset[3];

  //! The temperature at which last offset was updated.
  float offsetTempCelsius;

  //! The temperature sensitivity of offset.
  float tempSensitivity[3];

  //! The estimated offset at zero degree Celsius.
  float tempIntercept[3];

  //! The scale factor of the x, y and z axis.
  float scaleFactor[3];

  //! The cross-axis factor in the [yx, zx, zy] order.
  float crossAxis[3];

  //! The source of offset[3]
  uint8_t offsetSource;

  //! The source of offsetTempCelsius
  uint8_t offsetTempCelsiusSource;

  //! The source of tempSensitivity[3]
  uint8_t tempSensitivitySource;

  //! The source of tempIntercept[3]
  uint8_t tempInterceptSource;

  //! The source of scaleFactor[3]
  uint8_t scaleFactorSource;

  //! The source of crossAxis[3]
  uint8_t crossAxisSource;
};

/**
 * Updates the runtime calibration info of a given sensor type for the platform
 * to compensate for. The calibration will be applied on top of the sensor's
 * factory calibration if present.
 *
 * @param sensorType One of the CHRE_SENSOR_TYPE_* constants.
 * @param calInfo A non-null pointer to ashCalInfo to update the sensor's
          calibration.
 * @return true if the calibration info has been successfully updated.
 */
bool ashSetCalibration(uint8_t sensorType, const struct ashCalInfo *calInfo);

/**
 * Saves the calibration parameters to a persistent area.
 *
 * @param sensorType One of the CHRE_SENSOR_TYPE_* constants.
 * @param calParams A non-null pointer to ashCalParams that is to be saved.
 * @return true if the calibration parameters have been successfully saved.
 */
bool ashSaveCalibrationParams(uint8_t sensorType,
                              const struct ashCalParams *calParams);

/**
 * Loads the calibration parameters from the persistent area.
 *
 * @param sensorType One of the CHRE_SENSOR_TYPE_* constants.
 * @param calParams A non-null pointer to ashCalParams that the calibration
          parameters will be written to.
 * @return true if the calibration parameters have been successfully loaded.
 */
bool ashLoadCalibrationParams(uint8_t sensorType,
                              struct ashCalParams *calParams);

#ifdef __cplusplus
}
#endif

#endif  // CHRE_ASH_H_
