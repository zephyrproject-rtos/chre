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

//! The constant to convert magnetometer readings from Gauss in SMGR to uT in
//! Android.
constexpr float kMicroTeslaPerGauss = 100.0f;

//! Group size of sensor registry SNS_REG_SCM_GROUP_ACCEL_DYN_CAL_PARAMS_V02,
//! hard-coded in sns_reg_group_info[] of sns_reg_data.c
constexpr uint16_t kGroupSizeRegAccelDynCalParams = 234;

//! The QMI registry service client handle.
qmi_client_type gRegistryServiceQmiClientHandle = nullptr;

// TODO: create a script to auto-generat this array.
//! The offset of registry IDs that have been designated to store cal params.
const uint16_t gRegArray[3][22] = {
  {  // accel
    26,   // SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP1_V02,
    28,   // SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP1_V02,
    32,   // SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP1_V02,
    36,   // SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP1_V02,
    27,   // SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP1_V02,
    40,   // SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP1_V02,
    52,   // SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP2_V02,
    54,   // SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP2_V02,
    58,   // SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP2_V02,
    62,   // SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP2_V02,
    53,   // SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP2_V02,
    66,   // SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP2_V02,
    70,   // SNS_REG_ITEM_ACC_Y_DYN_SCALE_GROUP2_V02,
    74,   // SNS_REG_ITEM_ACC_Z_DYN_SCALE_GROUP2_V02,
    78,   // SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP3_V02,
    80,   // SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP3_V02,
    84,   // SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP3_V02,
    88,   // SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP3_V02,
    79,   // SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP3_V02,
    92,   // SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP3_V02,
    96,   // SNS_REG_ITEM_ACC_Y_DYN_SCALE_GROUP3_V02,
    100,  // SNS_REG_ITEM_ACC_Z_DYN_SCALE_GROUP3_V02,
  },
  {  // gyro
    104,  // SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP4_V02,
    106,  // SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP4_V02,
    110,  // SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP4_V02,
    114,  // SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP4_V02,
    105,  // SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP4_V02,
    118,  // SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP4_V02,
    130,  // SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP5_V02,
    132,  // SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP5_V02,
    136,  // SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP5_V02,
    140,  // SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP5_V02,
    131,  // SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP5_V02,
    144,  // SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP5_V02,
    148,  // SNS_REG_ITEM_ACC_Y_DYN_SCALE_GROUP5_V02,
    152,  // SNS_REG_ITEM_ACC_Z_DYN_SCALE_GROUP5_V02,
    156,  // SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP6_V02,
    158,  // SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP6_V02,
    162,  // SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP6_V02,
    166,  // SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP6_V02,
    157,  // SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP6_V02,
    170,  // SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP6_V02,
    174,  // SNS_REG_ITEM_ACC_Y_DYN_SCALE_GROUP6_V02,
    178,  // SNS_REG_ITEM_ACC_Z_DYN_SCALE_GROUP6_V02,
  },
  {  // mag
    182,  // SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP7_V02,
    184,  // SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP7_V02,
    188,  // SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP7_V02,
    192,  // SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP7_V02,
    183,  // SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP7_V02,
    196,  // SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP7_V02,
    208,  // SNS_REG_ITEM_ACC_DYN_CAL_VALID_FLAG_GROUP8_V02,
    210,  // SNS_REG_ITEM_ACC_X_DYN_BIAS_GROUP8_V02,
    214,  // SNS_REG_ITEM_ACC_Y_DYN_BIAS_GROUP8_V02,
    218,  // SNS_REG_ITEM_ACC_Z_DYN_BIAS_GROUP8_V02,
    209,  // SNS_REG_ITEM_ACC_DYN_CAL_TEMP_MIN_GROUP8_V02,
    222,  // SNS_REG_ITEM_ACC_X_DYN_SCALE_GROUP8_V02,
    226,  // SNS_REG_ITEM_ACC_Y_DYN_SCALE_GROUP8_V02,
    230,  // SNS_REG_ITEM_ACC_Z_DYN_SCALE_GROUP8_V02,
    24,   // SNS_REG_ITEM_ACC_DYN_CAL_HEADER_V02,
    0,    // SNS_REG_ITEM_ACC_X_DYN_BIAS_V02,
    4,    // SNS_REG_ITEM_ACC_Y_DYN_BIAS_V02,
    8,    // SNS_REG_ITEM_ACC_Z_DYN_BIAS_V02,
    25,   // SNS_REG_ITEM_ACC_DYN_CAL_TEMP_BIN_SIZE_V02,
    12,   // SNS_REG_ITEM_ACC_X_DYN_SCALE_V02,
    16,   // SNS_REG_ITEM_ACC_Y_DYN_SCALE_V02,
    20,   // SNS_REG_ITEM_ACC_Z_DYN_SCALE_V02 ,
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
 * A helper function that reads the sensor registry group.
 *
 * @param response A non-null pointer that registry group content will be
 *        written to.
 * @return true if the registry content has been successfully written to data.
 */
bool regRead(sns_reg_group_read_resp_msg_v02 *response) {
  CHRE_ASSERT(response);

  bool success = false;
  if (response != nullptr) {
    sns_reg_group_read_req_msg_v02 request;
    request.group_id = SNS_REG_SCM_GROUP_ACCEL_DYN_CAL_PARAMS_V02;

    qmi_client_error_type status = qmi_client_send_msg_sync(
        gRegistryServiceQmiClientHandle, SNS_REG_GROUP_READ_REQ_V02,
        &request, sizeof(request), response, sizeof(*response),
        kQmiTimeoutMs);

    if (status != QMI_NO_ERR) {
      LOGE("Error reading sensor registry: status %d", status);
    } else if (response->resp.sns_result_t != SNS_RESULT_SUCCESS_V01) {
      LOGE("Reading sensor registry failed with error: %" PRIu8,
           response->resp.sns_err_t);
    } else if (response->group_id
               != SNS_REG_SCM_GROUP_ACCEL_DYN_CAL_PARAMS_V02) {
      LOGE("Incorrect response group id %" PRIu16, response->group_id);
    } else {
      success = true;
    }
  }
  return success;
}

/**
 * A helper function that writes to the sensor registry group.
 *
 * @param request A non-null pointer to data that will be written to the
          registry group.
 * @return true if data has been successfully written to the registry.
 */
bool regWrite(sns_reg_group_write_req_msg_v02 *request) {
  CHRE_ASSERT(request);

  bool success = false;
  if (request != nullptr) {
    sns_reg_group_write_resp_msg_v02 response;
    request->group_id = SNS_REG_SCM_GROUP_ACCEL_DYN_CAL_PARAMS_V02;
    // Must set to actual group size, or the qmi request would fail.
    request->data_len = kGroupSizeRegAccelDynCalParams;

    qmi_client_error_type status = qmi_client_send_msg_sync(
        gRegistryServiceQmiClientHandle, SNS_REG_GROUP_WRITE_REQ_V02,
        request, sizeof(*request), &response, sizeof(response),
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

// A helper function that converts a floating-point value to Q16 format and
// writes it to the specified index and offset of a two-dimensional reg array.
void regOffsetWrite(float value, uint8_t *reg, size_t index, uint16_t offset) {
  int32_t value32 = FX_FLTTOFIX_Q16(value);
  memcpy(reg + gRegArray[index][offset], &value32, sizeof(value32));
}

// A helper function that reads the specified index and offset of a
// two-dimensional reg array and converts it to a floating-point value.
float regOffsetRead(uint8_t *reg, size_t index, uint16_t offset) {
  int32_t value32;
  memcpy(&value32, reg + gRegArray[index][offset], sizeof(value32));
  return FX_FIXTOFLT_Q16(value32);
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
    size_t idx = getRegArrayRowIndex(sensorType);
    float scaling = 1.0f;
    if (sensorType == CHRE_SENSOR_TYPE_GEOMAGNETIC_FIELD) {
      scaling = kGaussPerMicroTesla;
    }

    // Read the registry group, modify the items of interest and write it back
    auto *dataRead = memoryAlloc<sns_reg_group_read_resp_msg_v02>();
    auto *data = memoryAlloc<sns_reg_group_write_req_msg_v02>();
    if (dataRead != nullptr && data != nullptr) {
      success = regRead(dataRead);
      if (success) {
        uint8_t *reg = dataRead->data;

        // TODO: refactory so we don't have to manually insert offset.
        // offset
        reg[gRegArray[idx][0]] = calParams->offsetSource;
        regOffsetWrite(calParams->offset[1] * scaling, reg, idx, 1);
        regOffsetWrite(calParams->offset[0] * scaling, reg, idx, 2);
        regOffsetWrite(-calParams->offset[2] * scaling, reg, idx, 3);

        // offsetTempCelsius
        reg[gRegArray[idx][4]] = calParams->offsetTempCelsiusSource;
        regOffsetWrite(calParams->offsetTempCelsius, reg, idx, 5);

        // tempSensitivity
        reg[gRegArray[idx][6]] = calParams->tempSensitivitySource;
        regOffsetWrite(calParams->tempSensitivity[1] * scaling, reg, idx, 7);
        regOffsetWrite(calParams->tempSensitivity[0] * scaling, reg, idx, 8);
        regOffsetWrite(-calParams->tempSensitivity[2] * scaling, reg, idx, 9);

        // tempIntercept
        reg[gRegArray[idx][10]] = calParams->tempInterceptSource;
        regOffsetWrite(calParams->tempIntercept[1] * scaling, reg, idx, 11);
        regOffsetWrite(calParams->tempIntercept[0] * scaling, reg, idx, 12);
        regOffsetWrite(-calParams->tempIntercept[2] * scaling, reg, idx, 13);

        // scaleFactor
        reg[gRegArray[idx][14]] = calParams->scaleFactorSource;
        regOffsetWrite(calParams->scaleFactor[1], reg, idx, 15);
        regOffsetWrite(calParams->scaleFactor[0], reg, idx, 16);
        regOffsetWrite(-calParams->scaleFactor[2], reg, idx, 17);

        // crossAxis
        reg[gRegArray[idx][18]] = calParams->crossAxisSource;
        regOffsetWrite(calParams->crossAxis[1], reg, idx, 19);
        regOffsetWrite(calParams->crossAxis[0], reg, idx, 20);
        regOffsetWrite(-calParams->crossAxis[2], reg, idx, 21);

        // Only copy the actual group size
        memcpy(data->data, reg, kGroupSizeRegAccelDynCalParams);
        success &= regWrite(data);
      }
    }
    memoryFree(dataRead);
    memoryFree(data);
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
    size_t idx = getRegArrayRowIndex(sensorType);
    float scaling = 1.0f;
    if (sensorType == CHRE_SENSOR_TYPE_GEOMAGNETIC_FIELD) {
      scaling = kMicroTeslaPerGauss;
    }

    auto *data = memoryAlloc<sns_reg_group_read_resp_msg_v02>();
    if (data != nullptr) {
      success = regRead(data);
      if (success) {
        uint8_t *reg = data->data;

        // offset
        calParams->offsetSource = reg[gRegArray[idx][0]];
        calParams->offset[1] = regOffsetRead(reg, idx, 1) * scaling;
        calParams->offset[0] = regOffsetRead(reg, idx, 2) * scaling;
        calParams->offset[2] = -regOffsetRead(reg, idx, 3) * scaling;

        // offsetTempCelsius
        calParams->offsetTempCelsiusSource = reg[gRegArray[idx][4]];
        calParams->offsetTempCelsius = regOffsetRead(reg, idx, 5);

        // tempSensitivity
        calParams->tempSensitivitySource = reg[gRegArray[idx][6]];
        calParams->tempSensitivity[1] = regOffsetRead(reg, idx, 7) * scaling;
        calParams->tempSensitivity[0] = regOffsetRead(reg, idx, 8) * scaling;
        calParams->tempSensitivity[2] = -regOffsetRead(reg, idx, 9) * scaling;

        // tempIntercept
        calParams->tempInterceptSource = reg[gRegArray[idx][10]];
        calParams->tempIntercept[1] = regOffsetRead(reg, idx, 11) * scaling;
        calParams->tempIntercept[0] = regOffsetRead(reg, idx, 12) * scaling;
        calParams->tempIntercept[2] = -regOffsetRead(reg, idx, 13) * scaling;

        // scaleFactor
        calParams->scaleFactorSource = reg[gRegArray[idx][14]];
        calParams->scaleFactor[1] = regOffsetRead(reg, idx, 15);
        calParams->scaleFactor[0] = regOffsetRead(reg, idx, 16);
        calParams->scaleFactor[2] = -regOffsetRead(reg, idx, 17);

        // crossAxis
        calParams->crossAxisSource = reg[gRegArray[idx][18]];
        calParams->crossAxis[1] = regOffsetRead(reg, idx, 19);
        calParams->crossAxis[0] = regOffsetRead(reg, idx, 20);
        calParams->crossAxis[2] = -regOffsetRead(reg, idx, 21);
      }
    }
    memoryFree(data);
  }
  return success;
}
