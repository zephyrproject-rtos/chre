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

/*
 * This module provides a containing class (NanoSensorCal) for runtime
 * calibration algorithms that affect the following sensors:
 *       - Accelerometer (offset)
 *       - Gyroscope (offset, with optional over-temperature compensation)
 *       - Magnetometer (offset)
 *
 * Sensor Units:
 *       - Accelerometer [meters/sec^2]
 *       - Gyroscope [radian/sec]
 *       - Magnetometer [micro Tesla, uT]
 *       - Temperature [Celsius].
 *
 * NOTE: Define NANO_SENSOR_CAL_DBG_ENABLED to show debug messages.
 */

#ifndef LOCATION_LBS_CONTEXTHUB_NANOAPPS_CALIBRATION_NANO_CALIBRATION_NANO_CALIBRATION_H_
#define LOCATION_LBS_CONTEXTHUB_NANOAPPS_CALIBRATION_NANO_CALIBRATION_NANO_CALIBRATION_H_

#include <ash.h>

#ifdef ACCEL_CAL_ENABLED
#include "calibration/accelerometer/accel_cal.h"
#endif  // ACCEL_CAL_ENABLED

#ifdef GYRO_CAL_ENABLED
#include "calibration/gyroscope/gyro_cal.h"
#ifdef OVERTEMPCAL_ENABLED
#include "calibration/over_temp/over_temp_cal.h"
#endif  // OVERTEMPCAL_ENABLED
#endif  // GYRO_CAL_ENABLED

#ifdef MAG_CAL_ENABLED
#include "calibration/magnetometer/mag_cal.h"
#endif  // MAG_CAL_ENABLED

#include <chre.h>

namespace nano_calibration {

// Default sensor temperature used for initialization.
constexpr float kDefaultTemperatureCelsius = 20.0f;

// Maximum interval at which to check for OTC gyroscope offset updates.
constexpr uint64_t kOtcGyroOffsetMaxUpdateIntervalNanos = 500000000;

// The mathematical constant, Pi.
constexpr float kPi = 3.141592653589793238f;

// Unit conversion from nanoseconds to microseconds.
constexpr float kNanoToMicroseconds = 1e-3f;

/*
 * Class Definition:  NanoSensorCal.
 */
class NanoSensorCal {
 public:
  // Default constructor.
  NanoSensorCal();

  // Virtual destructor.
  virtual ~NanoSensorCal() {}

  // Initializes the sensor calibration algorithms.
  void Initialize();

  // Sends new sensor samples to the calibration algorithms.
  void HandleSensorSamples(uint16_t event_type,
                           const chreSensorThreeAxisData *event_data);

  // Provides temperature updates to the calibration algorithms.
  void HandleTemperatureSamples(uint16_t event_type,
                                const chreSensorFloatData *event_data);

  // Returns the availability of new calibration data (useful for polling).
  bool is_accel_calibration_ready() { return accel_calibration_ready_; }
  bool is_gyro_calibration_ready()  { return gyro_calibration_ready_; }
  bool is_mag_calibration_ready()   { return mag_calibration_ready_; }

  // Returns true if the NanoSensorCal object has been initialized.
  bool is_initialized() { return nanosensorcal_initialized_; }

  // Calibration data accessor functions useful, in conjunction with the
  // 'is_calibration_ready' functions, to poll for changes in calibration
  // parameters. Calling these functions returns the calibration parameters and
  // resets the calibration ready flags.
  void GetAccelerometerCalibration(struct ashCalParams *accel_cal_params) const;
  void GetGyroscopeCalibration(struct ashCalParams *gyro_cal_params) const;
  void GetMagnetometerCalibration(struct ashCalParams *mag_cal_params) const ;

 private:
  // Sends new sensor samples to the AccelCal.
  void HandleSensorSamplesAccelCal(uint16_t event_type,
                                   const chreSensorThreeAxisData *event_data);

  // Sends new sensor samples to the GyroCal/OTC. GyroCal utilizes multiple
  // sensor types (i.e., accel/gyro/mag).
  void HandleSensorSamplesGyroCal(uint16_t event_type,
                                  const chreSensorThreeAxisData *event_data);

  // Sends new sensor samples to the MagCal.
  void HandleSensorSamplesMagCal(uint16_t event_type,
                                 const chreSensorThreeAxisData *event_data);

  // Updates the local calibration parameters containers.
  void UpdateAccelCalParams();
  void UpdateGyroCalParams();
  void UpdateMagCalParams();

  // Loads persistent calibration data using the ASH API.
  void LoadAshAccelCal();
  void LoadAshGyroCal();
  void LoadAshMagCal();

  // Stores persistent calibration data parameters and updates calibration
  // information using the ASH API.
  void NotifyAshAccelCal();
  void NotifyAshGyroCal();
  void NotifyAshMagCal();

#ifdef ACCEL_CAL_ENABLED
  // Accelerometer runtime calibration.
  struct AccelCal accel_cal_;
#endif  // ACCEL_CAL_ENABLED

#ifdef GYRO_CAL_ENABLED
  // Gyroscope runtime calibration.
  struct GyroCal gyro_cal_;
#ifdef OVERTEMPCAL_ENABLED
  // Gyroscope over-temperature runtime calibration.
  struct OverTempCal over_temp_gyro_cal_;

  // Helper timer used to throttle OTC offset calibration updates.
  uint64_t otc_offset_timer_nanos_ = 0;
#endif  // OVERTEMPCAL_ENABLED
#endif  // GYRO_CAL_ENABLED

#ifdef MAG_CAL_ENABLED
  // Magnetometer runtime calibration.
  struct MagCal mag_cal_;
#endif  // MAG_CAL_ENABLED

  // Flag to indicate whether the NanoSensorCal object has been initialized.
  bool nanosensorcal_initialized_ = false;

  // Flags to indicate availability of new calibration data (polling).
  mutable bool accel_calibration_ready_ = false;
  mutable bool gyro_calibration_ready_ = false;
  mutable bool mag_calibration_ready_ = false;

  // Sensor temperature.
  float temperature_celsius_ = kDefaultTemperatureCelsius;

  // Sensor calibration parameter containers.
  struct ashCalParams accel_cal_params_;
  struct ashCalParams gyro_cal_params_;
  struct ashCalParams mag_cal_params_;
};

}  // namespace nano_calibration

#endif  // LOCATION_LBS_CONTEXTHUB_NANOAPPS_CALIBRATION_NANO_CALIBRATION_NANO_CALIBRATION_H_
