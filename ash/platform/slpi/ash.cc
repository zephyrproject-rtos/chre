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

#include <cinttypes>

extern "C" {

#include "fixed_point.h"
#include "sns_reg_api_v02.h"
#include "sns_smgr_api_v01.h"

}  // extern "C"

#include "ash/ash.h"
#include "ash_api/ash.h"
#include "chre/platform/assert.h"
#include "chre/platform/log.h"
#include "chre/platform/memory.h"
#include "chre/platform/slpi/smgr_client.h"
#include "chre_api/chre/sensor.h"

using chre::getSensorServiceQmiClientHandle;
using chre::memoryAlloc;
using chre::memoryFree;

namespace {

//! The timeout for QMI messages in milliseconds.
constexpr uint32_t kQmiTimeoutMs = 1000;

//! The constant to convert magnetometer readings from uT in Android to Gauss
//! in SMGR.
constexpr float kGaussPerMicroTesla = 0.01f;

//! The QMI registry service client handle.
qmi_client_type gRegistryServiceQmiClientHandle = nullptr;

//! The registry IDs that have been designated to store cal params.
const uint16_t gRegArray[3][22] = {
  {  // accel
    SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP1_V02,
    SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP1_V02,
    SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP1_V02,
    SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP1_V02,
    SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP1_V02,
    SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP1_V02,
    SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP2_V02,
    SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP2_V02,
    SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP2_V02,
    SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP2_V02,
    SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP2_V02,
    SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP2_V02,
    SNS_REG_ITEM_ACC_Y_DYN_SCALE_GROUP2_V02,
    SNS_REG_ITEM_ACC_Z_DYN_SCALE_GROUP2_V02,
    SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP3_V02,
    SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP3_V02,
    SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP3_V02,
    SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP3_V02,
    SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP3_V02,
    SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP3_V02,
    SNS_REG_ITEM_ACC_Y_DYN_SCALE_GROUP3_V02,
    SNS_REG_ITEM_ACC_Z_DYN_SCALE_GROUP3_V02,
  },
  {  // gyro
    SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP4_V02,
    SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP4_V02,
    SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP4_V02,
    SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP4_V02,
    SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP4_V02,
    SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP4_V02,
    SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP5_V02,
    SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP5_V02,
    SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP5_V02,
    SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP5_V02,
    SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP5_V02,
    SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP5_V02,
    SNS_REG_ITEM_ACC_Y_DYN_SCALE_GROUP5_V02,
    SNS_REG_ITEM_ACC_Z_DYN_SCALE_GROUP5_V02,
    SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP6_V02,
    SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP6_V02,
    SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP6_V02,
    SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP6_V02,
    SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP6_V02,
    SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP6_V02,
    SNS_REG_ITEM_ACC_Y_DYN_SCALE_GROUP6_V02,
    SNS_REG_ITEM_ACC_Z_DYN_SCALE_GROUP6_V02,
  },
  {  // mag
    SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP7_V02,
    SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP7_V02,
    SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP7_V02,
    SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP7_V02,
    SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP7_V02,
    SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP7_V02,
    SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP8_V02,
    SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP8_V02,
    SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP8_V02,
    SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP8_V02,
    SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP8_V02,
    SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP8_V02,
    SNS_REG_ITEM_ACC_Y_DYN_SCALE_GROUP8_V02,
    SNS_REG_ITEM_ACC_Z_DYN_SCALE_GROUP8_V02,
    SNS_REG_ITEM_MAG_DYN_CAL_BIAS_VALID_V02,
    SNS_REG_ITEM_MAG_X_DYN_BIAS_V02,
    SNS_REG_ITEM_MAG_Y_DYN_BIAS_V02,
    SNS_REG_ITEM_MAG_Z_DYN_BIAS_V02,
    SNS_REG_ITEM_MAG_DYN_CAL_CALIBRATION_MATRIX_VALID_V02,
    SNS_REG_ITEM_MAG_DYN_COMPENSATION_MATRIX_0_0_V02,
    SNS_REG_ITEM_MAG_DYN_COMPENSATION_MATRIX_0_1_V02,
    SNS_REG_ITEM_MAG_DYN_COMPENSATION_MATRIX_0_2_V02,
  },
};

/**
 * @param sensorType One of the CHRE_SENSOR_TYPE_* constants.
 * @return The row index of RegArray that corresponds to the sensorType.
 */
size_t getRegArrayRowIndex(uint8_t sensorType) {
  switch (sensorType) {
    case CHRE_SENSOR_TYPE_ACCELEROMETER:
      return 0;
    case CHRE_SENSOR_TYPE_GYROSCOPE:
      return 1;
    case CHRE_SENSOR_TYPE_GEOMAGNETIC_FIELD:
      return 2;
    default:
      return 0;
  }
}

/**
 * @param sensorType One of the CHRE_SENSOR_TYPE_* constants.
 * @return true if runtime sensor calibration is supported on this platform.
 */
bool isCalibrationSupported(uint8_t sensorType) {
  switch (sensorType) {
    case CHRE_SENSOR_TYPE_ACCELEROMETER:
    case CHRE_SENSOR_TYPE_GYROSCOPE:
    case CHRE_SENSOR_TYPE_GEOMAGNETIC_FIELD:
      return true;
    default:
      return false;
  }
}

/**
 * @param sensorType One of the CHRE_SENSOR_TYPE_* constants.
 * @return The sensor ID of the sensor type as defined in the SMGR API.
 */
uint8_t getSensorId(uint8_t sensorType) {
  switch (sensorType) {
    case CHRE_SENSOR_TYPE_ACCELEROMETER:
      return SNS_SMGR_ID_ACCEL_V01;
    case CHRE_SENSOR_TYPE_GYROSCOPE:
      return SNS_SMGR_ID_GYRO_V01;
    case CHRE_SENSOR_TYPE_GEOMAGNETIC_FIELD:
      return SNS_SMGR_ID_MAG_V01;
    default:
      return 0;
  }
}

/**
 * Populates the calibration request mesasge.
 *
 * @param sensorType One of the CHRE_SENSOR_TYPE_* constants.
 * @param calInfo The sensor calibraion info supplied by the user.
 * @param calRequest The SMGR cal request message to be populated.
 */
void populateCalRequest(uint8_t sensorType, const ashCalInfo *calInfo,
                        sns_smgr_sensor_cal_req_msg_v01 *calRequest) {
  CHRE_ASSERT(calInfo);
  CHRE_ASSERT(calRequest);

  calRequest->usage = SNS_SMGR_CAL_DYNAMIC_V01;
  calRequest->SensorId = getSensorId(sensorType);
  calRequest->DataType = SNS_SMGR_DATA_TYPE_PRIMARY_V01;

  // Convert from micro Tesla to Gauss for magnetometer bias
  float scaling = 1.0f;
  if (sensorType == CHRE_SENSOR_TYPE_GEOMAGNETIC_FIELD) {
    scaling = kGaussPerMicroTesla;
  }

  // Convert from Android to SMGR's NED coordinate and invert the sign as SMGR
  // defines Sc = CM * (Su + Bias) in sns_rh_calibrate_cm_and_bias().
  calRequest->ZeroBias_len = 3;
  calRequest->ZeroBias[0] = FX_FLTTOFIX_Q16(-calInfo->bias[1] * scaling);
  calRequest->ZeroBias[1] = FX_FLTTOFIX_Q16(-calInfo->bias[0] * scaling);
  calRequest->ZeroBias[2] = FX_FLTTOFIX_Q16(calInfo->bias[2] * scaling);

  // ScaleFactor will be over-written by compensation matrix.
  calRequest->ScaleFactor_len = 3;
  calRequest->ScaleFactor[0] = FX_FLTTOFIX_Q16(1.0);
  calRequest->ScaleFactor[1] = FX_FLTTOFIX_Q16(1.0);
  calRequest->ScaleFactor[2] = FX_FLTTOFIX_Q16(1.0);

  // Convert from Android to SMGR's NED coordinate.
  calRequest->CompensationMatrix_valid = true;
  calRequest->CompensationMatrix_len = 9;
  calRequest->CompensationMatrix[0] = FX_FLTTOFIX_Q16(calInfo->compMatrix[4]);
  calRequest->CompensationMatrix[1] = FX_FLTTOFIX_Q16(calInfo->compMatrix[3]);
  calRequest->CompensationMatrix[2] = FX_FLTTOFIX_Q16(-calInfo->compMatrix[5]);
  calRequest->CompensationMatrix[3] = FX_FLTTOFIX_Q16(calInfo->compMatrix[1]);
  calRequest->CompensationMatrix[4] = FX_FLTTOFIX_Q16(calInfo->compMatrix[0]);
  calRequest->CompensationMatrix[5] = FX_FLTTOFIX_Q16(-calInfo->compMatrix[2]);
  calRequest->CompensationMatrix[6] = FX_FLTTOFIX_Q16(-calInfo->compMatrix[7]);
  calRequest->CompensationMatrix[7] = FX_FLTTOFIX_Q16(-calInfo->compMatrix[6]);
  calRequest->CompensationMatrix[8] = FX_FLTTOFIX_Q16(calInfo->compMatrix[8]);

  calRequest->CalibrationAccuracy_valid = true;
  calRequest->CalibrationAccuracy = calInfo->accuracy;
}

/**
 * A helper function that reads from the sensor registry.
 *
 * @param regId The registry ID to read from.
 * @param data A non-null pointer that registry content will be written to.
 * @param dataSize The number of bytes to read from this registry ID.
 * @return true if the registry content has been successfully written to data.
 */
bool regRead(uint16_t regId, void *data, size_t dataSize) {
  CHRE_ASSERT(data);

  bool success = false;
  if (data != nullptr) {
    sns_reg_single_read_req_msg_v02 request;
    sns_reg_single_read_resp_msg_v02 response;

    request.item_id = regId;

    qmi_client_error_type status = qmi_client_send_msg_sync(
        gRegistryServiceQmiClientHandle, SNS_REG_SINGLE_READ_REQ_V02,
        &request, sizeof(request), &response, sizeof(response),
        kQmiTimeoutMs);

    if (status != QMI_NO_ERR) {
      LOGE("Error reading sensor registry: status %d", status);
    } else if (response.resp.sns_result_t != SNS_RESULT_SUCCESS_V01) {
      LOGE("Reading sensor registry failed with error: %" PRIu8,
           response.resp.sns_err_t);
    } else if (response.data_len < dataSize) {
      LOGE("Registry data len is less than requested.");
    } else {
      success = true;
      memcpy(data, &response.data, dataSize);
    }
  }
  return success;
}

/**
 * A helper function that writes to the sensor registry.
 *
 * @param regId The registry ID to write to.
 * @param data A non-null pointer to data that will be written to the registry.
 * @param dataSize The number of bytes to write to this registry ID.
 * @return true if data has been successfully written to the registry.
 */
bool regWrite(uint16_t regId, const void *data, size_t dataSize) {
  CHRE_ASSERT(data);

  bool success = false;
  if (data != nullptr) {
    sns_reg_single_write_req_msg_v02 request;
    sns_reg_single_write_resp_msg_v02 response;

    request.item_id = regId;
    request.data_len = dataSize;
    memcpy(&request.data, data, dataSize);

    qmi_client_error_type status = qmi_client_send_msg_sync(
        gRegistryServiceQmiClientHandle, SNS_REG_SINGLE_WRITE_REQ_V02,
        &request, sizeof(request), &response, sizeof(response),
        kQmiTimeoutMs);

    if (status != QMI_NO_ERR) {
      LOGE("Error writing sensor registry: status %d", status);
    } else if (response.resp.sns_result_t != SNS_RESULT_SUCCESS_V01) {
      LOGE("Writing sensor registry failed with error: %" PRIu8,
           response.resp.sns_err_t);
    } else {
      success = true;
    }
  }
  return success;
}

}  // namespace

void ashInit() {
  qmi_idl_service_object_type regServiceObject =
      SNS_REG2_SVC_get_service_object_v02();
  if (regServiceObject == nullptr) {
    FATAL_ERROR("Failed to obtain the SNS REG2 service instance");
  }

  qmi_client_os_params sensorContextOsParams;
  qmi_client_error_type status = qmi_client_init_instance(regServiceObject,
      QMI_CLIENT_INSTANCE_ANY, nullptr /* callback */,
      nullptr, &sensorContextOsParams, kQmiTimeoutMs,
      &gRegistryServiceQmiClientHandle);
  if (status != QMI_NO_ERR) {
    FATAL_ERROR("Failed to initialize the registry service QMI client: %d",
                status);
  }
}

void ashDeinit() {
  qmi_client_release(&gRegistryServiceQmiClientHandle);
  gRegistryServiceQmiClientHandle = nullptr;
}

bool ashSetCalibration(uint8_t sensorType, const struct ashCalInfo *calInfo) {
  bool success = false;
  if (!isCalibrationSupported(sensorType)) {
    LOGE("Attempting to set calibration of sensor %" PRIu8, sensorType);
  } else {
    // Allocate request and response for sensor calibraton.
    auto *calRequest = memoryAlloc<sns_smgr_sensor_cal_req_msg_v01>();
    auto *calResponse = memoryAlloc<sns_smgr_sensor_cal_resp_msg_v01>();

    if (calRequest == nullptr || calResponse == nullptr) {
      LOGE("Failed to allocated sensor cal memory");
    } else {
      populateCalRequest(sensorType, calInfo, calRequest);

      qmi_client_error_type status = qmi_client_send_msg_sync(
          getSensorServiceQmiClientHandle(), SNS_SMGR_CAL_REQ_V01,
          calRequest, sizeof(*calRequest), calResponse, sizeof(*calResponse),
          kQmiTimeoutMs);

      if (status != QMI_NO_ERR) {
        LOGE("Error setting sensor calibration: status %d", status);
      } else if (calResponse->Resp.sns_result_t != SNS_RESULT_SUCCESS_V01) {
        LOGE("Setting sensor calibration failed with error: %" PRIu8,
             calResponse->Resp.sns_err_t);
      } else {
        success = true;
      }
    }

    memoryFree(calRequest);
    memoryFree(calResponse);
  }
  return success;
}

bool ashSaveCalibrationParams(uint8_t sensorType,
                              const struct ashCalParams *calParams) {
  CHRE_ASSERT(calParams);

  bool success = false;
  if (!isCalibrationSupported(sensorType)) {
    LOGE("Attempting to save cal params of sensor %" PRIu8, sensorType);
  } else if (calParams != nullptr) {
    success = true;
    size_t idx = getRegArrayRowIndex(sensorType);
    // TODO: use group write to improve efficiency.
    // offset
    success &= regWrite(gRegArray[idx][0], &calParams->offsetSource, 1);
    success &= regWrite(gRegArray[idx][1], &calParams->offset[0], 4);
    success &= regWrite(gRegArray[idx][2], &calParams->offset[1], 4);
    success &= regWrite(gRegArray[idx][3], &calParams->offset[2], 4);

    // offsetTempCelsius
    success &= regWrite(gRegArray[idx][4],
                        &calParams->offsetTempCelsiusSource, 1);
    // retain factory cal format.
    int32_t value32 = FX_FLTTOFIX_Q16(calParams->offsetTempCelsius);
    success &= regWrite(gRegArray[idx][5], &value32, 4);

    // tempSensitivity
    success &= regWrite(gRegArray[idx][6],
                        &calParams->tempSensitivitySource, 1);
    // retain factory cal format
    value32 = FX_FLTTOFIX_Q16(calParams->tempSensitivity[1]);
    success &= regWrite(gRegArray[idx][7], &value32, 4);
    value32 = FX_FLTTOFIX_Q16(calParams->tempSensitivity[0]);
    success &= regWrite(gRegArray[idx][8], &value32, 4);
    value32 = FX_FLTTOFIX_Q16(-calParams->tempSensitivity[2]);
    success &= regWrite(gRegArray[idx][9], &value32, 4);

    // tempIntercept
    success &= regWrite(gRegArray[idx][10], &calParams->tempInterceptSource, 1);
    // retain factory cal format
    value32 = FX_FLTTOFIX_Q16(calParams->tempIntercept[1]);
    success &= regWrite(gRegArray[idx][11], &value32, 4);
    value32 = FX_FLTTOFIX_Q16(calParams->tempIntercept[0]);
    success &= regWrite(gRegArray[idx][12], &value32, 4);
    value32 = FX_FLTTOFIX_Q16(-calParams->tempIntercept[2]);
    success &= regWrite(gRegArray[idx][13], &value32, 4);

    // scaleFactor
    success &= regWrite(gRegArray[idx][14], &calParams->scaleFactorSource, 1);
    success &= regWrite(gRegArray[idx][15], &calParams->scaleFactor[0], 4);
    success &= regWrite(gRegArray[idx][16], &calParams->scaleFactor[1], 4);
    success &= regWrite(gRegArray[idx][17], &calParams->scaleFactor[2], 4);

    // crossAxis
    success &= regWrite(gRegArray[idx][18], &calParams->crossAxisSource, 1);
    success &= regWrite(gRegArray[idx][19], &calParams->crossAxis[0], 4);
    success &= regWrite(gRegArray[idx][20], &calParams->crossAxis[1], 4);
    success &= regWrite(gRegArray[idx][21], &calParams->crossAxis[2], 4);
  }
  return success;
}

bool ashLoadCalibrationParams(uint8_t sensorType,
                              struct ashCalParams *calParams) {
  CHRE_ASSERT(calParams);

  bool success = false;
  if (!isCalibrationSupported(sensorType)) {
    LOGE("Attempting to write cal params of sensor %" PRIu8, sensorType);
  } else if (calParams != nullptr) {
    success = true;
    size_t idx = getRegArrayRowIndex(sensorType);
    // TODO: use group read to improve efficiency.
    // offset
    success &= regRead(gRegArray[idx][0], &calParams->offsetSource, 1);
    success &= regRead(gRegArray[idx][1], &calParams->offset[0], 4);
    success &= regRead(gRegArray[idx][2], &calParams->offset[1], 4);
    success &= regRead(gRegArray[idx][3], &calParams->offset[2], 4);

    // offsetTempCelsius
    success &= regRead(gRegArray[idx][4],
                       &calParams->offsetTempCelsiusSource, 1);
    // factory cal format.
    int32_t value32;
    success &= regRead(gRegArray[idx][5], &value32, 4);
    calParams->offsetTempCelsius = FX_FIXTOFLT_Q16(value32);

    // tempSensitivity
    success &= regRead(gRegArray[idx][6], &calParams->tempSensitivitySource, 1);
    // factory cal format.
    success &= regRead(gRegArray[idx][7], &value32, 4);
    calParams->tempSensitivity[1] = FX_FIXTOFLT_Q16(value32);
    success &= regRead(gRegArray[idx][8], &value32, 4);
    calParams->tempSensitivity[0] = FX_FIXTOFLT_Q16(value32);
    success &= regRead(gRegArray[idx][9], &value32, 4);
    calParams->tempSensitivity[2] = -FX_FIXTOFLT_Q16(value32);

    // tempIntercept
    success &= regRead(gRegArray[idx][10], &calParams->tempInterceptSource, 1);
    // factory cal format.
    success &= regRead(gRegArray[idx][11], &value32, 4);
    calParams->tempIntercept[1] = FX_FIXTOFLT_Q16(value32);
    success &= regRead(gRegArray[idx][12], &value32, 4);
    calParams->tempIntercept[0] = FX_FIXTOFLT_Q16(value32);
    success &= regRead(gRegArray[idx][13], &value32, 4);
    calParams->tempIntercept[2] = -FX_FIXTOFLT_Q16(value32);

    // scaleFactor
    success &= regRead(gRegArray[idx][14], &calParams->scaleFactorSource, 1);
    success &= regRead(gRegArray[idx][15], &calParams->scaleFactor[0], 4);
    success &= regRead(gRegArray[idx][16], &calParams->scaleFactor[1], 4);
    success &= regRead(gRegArray[idx][17], &calParams->scaleFactor[2], 4);

    // crossAxis
    success &= regRead(gRegArray[idx][18], &calParams->crossAxisSource, 1);
    success &= regRead(gRegArray[idx][19], &calParams->crossAxis[0], 4);
    success &= regRead(gRegArray[idx][20], &calParams->crossAxis[1], 4);
    success &= regRead(gRegArray[idx][21], &calParams->crossAxis[2], 4);
  }
  return success;
}
