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

#include "chre/platform/slpi/see/see_helper.h"

#include "pb_decode.h"
#include "pb_encode.h"
#include "sns_cal.pb.h"
#include "sns_client.pb.h"
#include "sns_client_api_v01.h"
#include "sns_proximity.pb.h"
#include "sns_rc.h"
#include "sns_remote_proc_state.pb.h"
#include "sns_std.pb.h"
#include "sns_std_sensor.pb.h"
#include "stringl.h"
#include "timer.h"

#include <algorithm>
#include <cfloat>
#include <cinttypes>
#include <cmath>

#include "chre/platform/assert.h"
#include "chre/platform/log.h"
#include "chre/platform/slpi/system_time_util.h"
#include "chre/util/lock_guard.h"
#include "chre/util/macros.h"

#ifdef CHREX_SENSOR_SUPPORT
#include "chre/extensions/platform/vendor_sensor_types.h"
#endif  // CHREX_SENSOR_SUPPORT

#define LOG_NANOPB_ERROR(stream) \
    LOGE("Nanopb error: %s:%d", PB_GET_ERROR(stream), __LINE__)

namespace chre {
namespace {

//! Operating mode indicating sensor is disabled.
const char *kOpModeOff = "OFF";

//! The SUID of the look up sensor.
const sns_std_suid kSuidLookup = sns_suid_sensor_init_default;

//! A struct to facilitate pb encode/decode
struct SeeBufArg {
  const void *buf;
  size_t bufLen;
};

//! A struct to facilitate pb decode of sync calls.
struct SeeSyncArg {
  sns_std_suid syncSuid;
  void *syncData;
  const char *syncDataType;
  bool syncIndFound;
};

//! SeeFloatArg can be used to decode a vectorized 3x3 array.
constexpr size_t kSeeFloatArgValLen = 9;

//! A struct to facilitate decoding a float array.
struct SeeFloatArg {
  size_t index;
  float val[kSeeFloatArgValLen];
};

//! A struct to facilitate pb decode of sensor data event.
struct SeeDataArg {
  uint64_t prevTimeNs;
  uint64_t timeNs;
  size_t sampleIndex;
  size_t totalSamples;
  UniquePtr<uint8_t> event;
  UniquePtr<SeeHelperCallbackInterface::SamplingStatusData> status;
  SeeCalData *cal;
  SensorType sensorType;
  bool isHostWakeSuspendEvent;
  bool isHostAwake;
};

//! A struct to facilitate pb decode
struct SeeInfoArg {
  qmi_client_type qmiHandle;
  sns_std_suid suid;
  uint32_t msgId;
  SeeSyncArg *sync;
  SeeDataArg *data;
  bool decodeMsgIdOnly;
  Optional<sns_std_suid> *remoteProcSuid;
  SeeCalInfo *calInfo;
};

//! A struct to facilitate decoding sensor attributes.
struct SeeAttrArg {
  union {
    char strVal[kSeeAttrStrValLen];
    bool boolVal;
    struct {
      float fltMin;
      float fltMax;
    };
    int64_t int64;
  };
  bool initialized;
};

size_t getCalIndexFromSensorType(SensorType sensorType) {
  SeeCalSensor index;
  switch (sensorType) {
    case SensorType::Accelerometer:
      index = SeeCalSensor::AccelCal;
      break;
    case SensorType::Gyroscope:
      index = SeeCalSensor::GyroCal;
      break;
    case SensorType::GeomagneticField:
      index = SeeCalSensor::MagCal;
      break;
    default:
      index = SeeCalSensor::NumCalSensors;
  }
  return static_cast<size_t>(index);
}

size_t getCalIndexFromDataType(const char *dataType) {
  SensorType sensorType = SensorType::Unknown;
  if (strcmp(dataType, "accel_cal") == 0) {
    sensorType = SensorType::Accelerometer;
  } else if (strcmp(dataType, "gyro_cal") == 0) {
    sensorType = SensorType::Gyroscope;
  } else if (strcmp(dataType, "mag_cal") == 0) {
    sensorType = SensorType::GeomagneticField;
  }
  return getCalIndexFromSensorType(sensorType);
}

size_t getCalIndexFromSuid(const sns_std_suid& suid,
                           const SeeCalInfo *calInfo) {
  size_t i = 0;
  for (; i < kNumSeeCalSensors; i++) {
    if (calInfo[i].suid.has_value()
        && suidsMatch(suid, calInfo[i].suid.value())) {
      break;
    }
  }
  return i;
}

/**
 * Copy an encoded pb message to a wrapper proto's field.
 */
bool copyPayload(pb_ostream_t *stream, const pb_field_t *field,
                 void *const *arg) {
  bool success = false;

  auto *data = static_cast<const SeeBufArg *>(*arg);
  if (!pb_encode_tag_for_field(stream, field)) {
    LOG_NANOPB_ERROR(stream);
  } else if (!pb_encode_string(
      stream, static_cast<const pb_byte_t *>(data->buf), data->bufLen)) {
    LOG_NANOPB_ERROR(stream);
  } else {
    success = true;
  }
  return success;
}

/**
 * Encodes sns_std_attr_req pb message.
 *
 * @param msg A non-null pointer to the pb message unique pointer whose object
 *            will be assigned here.
 * @param msgLen A non-null pointer to the size of the encoded pb message.
 *
 * @return true if the pb message and length were obtained.
 */
bool encodeSnsStdAttrReq(UniquePtr<pb_byte_t> *msg, size_t *msgLen) {
  CHRE_ASSERT(msg);
  CHRE_ASSERT(msgLen);

  // Initialize the pb message
  sns_std_attr_req req = {};

  bool success = pb_get_encoded_size(msgLen, sns_std_attr_req_fields, &req);
  if (!success) {
    LOGE("pb_get_encoded_size failed for sns_str_attr_req");
  } else {
    UniquePtr<pb_byte_t> buf(static_cast<pb_byte_t *>(memoryAlloc(*msgLen)));
    *msg = std::move(buf);

    // The encoded size can be 0 as there's only one optional field.
    if (msg->isNull() && *msgLen > 0) {
      LOG_OOM();
    } else {
      pb_ostream_t stream = pb_ostream_from_buffer(msg->get(), *msgLen);

      success = pb_encode(&stream, sns_std_attr_req_fields, &req);
      if (!success) {
        LOG_NANOPB_ERROR(&stream);
      }
    }
  }
  return success;
}

/**
 * Encodes sns_suid_req pb message.
 *
 * @param dataType Sensor data type, "accel" for example.
 * @param msg A non-null pointer to the pb message unique pointer whose object
 *            will be assigned here.
 * @param msgLen A non-null pointer to the size of the encoded pb message.
 *
 * @return true if the pb message and length were obtained.
 */
bool encodeSnsSuidReq(const char *dataType,
                      UniquePtr<pb_byte_t> *msg, size_t *msgLen) {
  CHRE_ASSERT(msg);
  CHRE_ASSERT(msgLen);
  bool success = false;

  // Initialize the pb message
  SeeBufArg data = {
    .buf = dataType,
    .bufLen = strlen(dataType),
  };
  sns_suid_req req = {
    .data_type.funcs.encode = copyPayload,
    .data_type.arg = &data,
  };

  if (!pb_get_encoded_size(msgLen, sns_suid_req_fields, &req)) {
    LOGE("pb_get_encoded_size failed for sns_suid_req: %s", dataType);
  } else if (*msgLen == 0) {
    LOGE("Invalid pb encoded size for sns_suid_req");
  } else {
    UniquePtr<pb_byte_t> buf(static_cast<pb_byte_t *>(memoryAlloc(*msgLen)));
    *msg = std::move(buf);
    if (msg->isNull()) {
      LOG_OOM();
    } else {
      pb_ostream_t stream = pb_ostream_from_buffer(msg->get(), *msgLen);

      success = pb_encode(&stream, sns_suid_req_fields, &req);
      if (!success) {
        LOG_NANOPB_ERROR(&stream);
      }
    }
  }
  return success;
}

/**
 * Encodes sns_std_sensor_config pb message.
 *
 * @param request The request to be encoded.
 * @param msg A non-null pointer to the pb message unique pointer whose object
 *            will be assigned here.
 * @param msgLen A non-null pointer to the size of the encoded pb message.
 *
 * @return true if the pb message and length were obtained.
 */
bool encodeSnsStdSensorConfig(const SeeSensorRequest& request,
                              UniquePtr<pb_byte_t> *msg, size_t *msgLen) {
  CHRE_ASSERT(msg);
  CHRE_ASSERT(msgLen);
  bool success = false;

  // Initialize the pb message
  sns_std_sensor_config req = {
    .sample_rate = request.samplingRateHz,
  };

  if (!pb_get_encoded_size(msgLen, sns_std_sensor_config_fields, &req)) {
    LOGE("pb_get_encoded_size failed for sns_std_sensor_config");
  } else if (*msgLen == 0) {
    LOGE("Invalid pb encoded size for sns_std_sensor_config");
  } else {
    UniquePtr<pb_byte_t> buf(static_cast<pb_byte_t *>(memoryAlloc(*msgLen)));
    *msg = std::move(buf);
    if (msg->isNull()) {
      LOG_OOM();
    } else {
      pb_ostream_t stream = pb_ostream_from_buffer(msg->get(), *msgLen);

      success = pb_encode(&stream, sns_std_sensor_config_fields, &req);
      if (!success) {
        LOG_NANOPB_ERROR(&stream);
      }
    }
  }
  return success;
}

bool encodeSnsRemoteProcSensorConfig(pb_byte_t *msgBuffer, size_t msgBufferSize,
                                     size_t *msgLen,
                                     sns_std_client_processor processorType) {
  CHRE_ASSERT(msgBuffer);
  CHRE_ASSERT(msgLen);

  sns_remote_proc_state_config request = {
    .proc_type = processorType,
  };

  pb_ostream_t stream = pb_ostream_from_buffer(msgBuffer, msgBufferSize);
  bool success = pb_encode(
      &stream, sns_remote_proc_state_config_fields, &request);
  if (!success) {
    LOG_NANOPB_ERROR(&stream);
  } else {
    *msgLen = stream.bytes_written;
  }

  return success;
}

/**
 * Sends a sync request to QMI.
 */
bool sendQmiReq(qmi_client_type qmiHandle, const sns_client_req_msg_v01& reqMsg,
                Nanoseconds timeoutResp) {
  bool success = false;

  sns_client_resp_msg_v01 resp;
  qmi_client_error_type status = qmi_client_send_msg_sync(
      qmiHandle, SNS_CLIENT_REQ_V01,
      const_cast<sns_client_req_msg_v01 *>(&reqMsg), sizeof(reqMsg),
      &resp, sizeof(resp), Milliseconds(timeoutResp).getMilliseconds());

  if (status != QMI_NO_ERR) {
    LOGE("Error sending QMI message %d", status);
  } else if (resp.resp.result != QMI_RESULT_SUCCESS_V01) {
    LOGE("QMI failed with error %d", resp.resp.error);
  } else if (!resp.result_valid || resp.result != SNS_RC_SUCCESS) {
    LOGE("Client request failed with result %" PRIu8,
         static_cast<uint8_t>(resp.result));
  } else {
    success = true;
  }
  return success;
}

/**
 * Sends a sns_client_req message with provided payload.
 */
bool sendSnsClientReq(qmi_client_type qmiHandle, sns_std_suid suid,
                      uint32_t msgId, void *payload, size_t payloadLen,
                      bool batchValid, uint32_t batchPeriodUs, bool passive,
                      Nanoseconds timeoutResp) {
  CHRE_ASSERT(payload || payloadLen == 0);
  bool success = false;

  // Initialize pb message to be encoded
  SeeBufArg data = {
    .buf = payload,
    .bufLen = payloadLen,
  };
  sns_client_request_msg req = {
    .suid = suid,
    .msg_id = msgId,
    .susp_config.client_proc_type = SNS_STD_CLIENT_PROCESSOR_SSC,
    .susp_config.delivery_type = SNS_CLIENT_DELIVERY_WAKEUP,
    .request.has_batching = batchValid,
    .request.batching.batch_period = batchPeriodUs,
    .request.payload.funcs.encode = copyPayload,
    .request.payload.arg = &data,
    .request.has_is_passive = true,
    .request.is_passive = passive,
  };

  auto qmiReq = MakeUniqueZeroFill<sns_client_req_msg_v01>();
  if (qmiReq.isNull()) {
    LOG_OOM();
  } else {
    // Create pb stream
    pb_ostream_t stream = pb_ostream_from_buffer(
        qmiReq->payload, SNS_CLIENT_REQ_LEN_MAX_V01);

    // Encode pb message
    if (!pb_encode(&stream, sns_client_request_msg_fields, &req)) {
      LOG_NANOPB_ERROR(&stream);
    } else {
      qmiReq->payload_len = stream.bytes_written;
      success = sendQmiReq(qmiHandle, *qmiReq, timeoutResp);
    }
  }
  return success;
}

/**
 * Helps decode a pb string field and passes the string to the calling function.
 */
bool decodeStringField(pb_istream_t *stream, const pb_field_t *field,
                       void **arg) {
  auto *data = static_cast<SeeBufArg *>(*arg);
  data->bufLen = stream->bytes_left;
  data->buf = stream->state;

  return pb_read(stream, nullptr /* buf */, stream->bytes_left);
}

/**
 * Decodes each SUID.
 */
bool decodeSnsSuidEventSuid(pb_istream_t *stream, const pb_field_t *field,
                            void **arg) {
  sns_std_suid suid = {};
  bool success = pb_decode(stream, sns_std_suid_fields, &suid);
  if (!success) {
    LOG_NANOPB_ERROR(stream);
  } else {
    auto *suids = static_cast<DynamicVector<sns_std_suid> *>(*arg);
    suids->push_back(suid);
  }
  return success;
}

bool decodeSnsSuidEvent(pb_istream_t *stream, const pb_field_t *field,
                        void **arg) {
  auto *info = static_cast<SeeInfoArg *>(*arg);
  if (!suidsMatch(info->suid, kSuidLookup)) {
    LOGE("SNS_SUID_MSGID_SNS_SUID_EVENT with incorrect SUID: 0x%" PRIx64
         " %" PRIx64, info->suid.suid_high, info->suid.suid_low);
  }

  SeeBufArg data;
  DynamicVector<sns_std_suid> suids;
  sns_suid_event event = {
    .data_type.funcs.decode = decodeStringField,
    .data_type.arg = &data,
    .suid.funcs.decode = decodeSnsSuidEventSuid,
    .suid.arg = &suids,
  };

  bool success = pb_decode(stream, sns_suid_event_fields, &event);
  if (!success) {
    LOG_NANOPB_ERROR(stream);
  } else {
    // If syncData == nullptr, this indication is received outside of a sync
    // call. If the decoded data type doesn't match the one we are waiting
    // for, this indication is from a previous call (may be findSuidSync)
    // and happens to arrive between another sync req/ind pair.
    // Note that req/ind misalignment can still happen if findSuidSync is
    // called again with the same data type.
    // Note that there's no need to compare the SUIDs as no other calls
    // but findSuidSync populate mWaitingDataType and can lead to a data
    // type match.
    if (info->sync->syncData == nullptr
        || strncmp(info->sync->syncDataType,
                   static_cast<const char *>(data.buf),
                   std::min(data.bufLen, kSeeAttrStrValLen)) != 0) {
      LOGW("Received late SNS_SUID_MSGID_SNS_SUID_EVENT indication");
    } else {
      info->sync->syncIndFound = true;
      auto *outputSuids = static_cast<DynamicVector<sns_std_suid> *>(
          info->sync->syncData);
      for (const auto& suid : suids) {
        outputSuids->push_back(suid);
      }
    }
  }
  return success;
}

/**
 * Decode messages defined in sns_suid.proto
 */
bool decodeSnsSuidProtoEvent(pb_istream_t *stream, const pb_field_t *field,
                             void **arg) {
  bool success = false;

  auto *info = static_cast<SeeInfoArg *>(*arg);
  switch (info->msgId) {
    case SNS_SUID_MSGID_SNS_SUID_EVENT:
      success = decodeSnsSuidEvent(stream, field, arg);
      break;

    default:
      LOGW("Unhandled sns_suid.proto msg ID: %" PRIu32, info->msgId);
      break;
  }
  return success;
}

/**
 * Defined in sns_std_sensor.pb.h
 */
const char *getAttrNameFromAttrId(int32_t id) {
  switch (id) {
    case SNS_STD_SENSOR_ATTRID_NAME:
      return "NAME";
    case SNS_STD_SENSOR_ATTRID_VENDOR:
      return "VENDOR";
    case SNS_STD_SENSOR_ATTRID_TYPE:
      return "TYPE";
    case SNS_STD_SENSOR_ATTRID_AVAILABLE:
      return "AVAILABLE";
    case SNS_STD_SENSOR_ATTRID_VERSION:
      return "VERSION";
    case SNS_STD_SENSOR_ATTRID_API:
      return "API";
    case SNS_STD_SENSOR_ATTRID_RATES:
      return "RATES";
    case SNS_STD_SENSOR_ATTRID_RESOLUTIONS:
      return "RESOLUTIONS";
    case SNS_STD_SENSOR_ATTRID_FIFO_SIZE:
      return "FIFO_SIZE";
    case SNS_STD_SENSOR_ATTRID_ACTIVE_CURRENT:
      return "ACTIVE_CURRENT";
    case SNS_STD_SENSOR_ATTRID_SLEEP_CURRENT:
      return "SLEEP_CURRENT";
    case SNS_STD_SENSOR_ATTRID_RANGES:
      return "RANGES";
    case SNS_STD_SENSOR_ATTRID_OP_MODES:
      return "OP_MODES";
    case SNS_STD_SENSOR_ATTRID_DRI:
      return "DRI";
    case SNS_STD_SENSOR_ATTRID_STREAM_SYNC:
      return "STREAM_SYNC";
    case SNS_STD_SENSOR_ATTRID_EVENT_SIZE:
      return "EVENT_SIZE";
    case SNS_STD_SENSOR_ATTRID_STREAM_TYPE:
      return "STREAM_TYPE";
    case SNS_STD_SENSOR_ATTRID_DYNAMIC:
      return "DYNAMIC";
    case SNS_STD_SENSOR_ATTRID_HW_ID:
      return "HW_ID";
    case SNS_STD_SENSOR_ATTRID_RIGID_BODY:
      return "RIGID_BODY";
    case SNS_STD_SENSOR_ATTRID_PLACEMENT:
      return "PLACEMENT";
    case SNS_STD_SENSOR_ATTRID_PHYSICAL_SENSOR:
      return "PHYSICAL_SENSOR";
    case SNS_STD_SENSOR_ATTRID_PHYSICAL_SENSOR_TESTS:
      return "PHYSICAL_SENSOR_TESTS";
    case SNS_STD_SENSOR_ATTRID_SELECTED_RESOLUTION:
      return "SELECTED_RESOLUTION";
    case SNS_STD_SENSOR_ATTRID_SELECTED_RANGE:
      return "SELECTED_RANGE";
    case SNS_STD_SENSOR_ATTRID_ADDITIONAL_LOW_LATENCY_RATES:
      return "LOW_LATENCY_RATES";
    case SNS_STD_SENSOR_ATTRID_PASSIVE_REQUEST:
      return "PASSIVE_REQUEST";
    default:
      return "UNKNOWN ATTRIBUTE";
  }
}

/**
 * Decodes each attribute field and passes the value to the calling function.
 * For repeated fields of float or integers, only store the maximum and
 * minimum values for the calling function.
 */
bool decodeSnsStdAttrValue(pb_istream_t *stream, const pb_field_t *field,
                           void **arg) {
  bool success = false;

  struct DecodeData {
    SeeBufArg strData;
    SeeAttrArg subtypeAttrArg;
    sns_std_attr_value_data value;
  };
  auto data = MakeUniqueZeroFill<DecodeData>();

  if (data.isNull()) {
    LOG_OOM();
  } else {
    data->value.str.funcs.decode = decodeStringField;
    data->value.str.arg = &data->strData;
    data->value.subtype.values.funcs.decode = decodeSnsStdAttrValue;
    data->value.subtype.values.arg = &data->subtypeAttrArg;

    success = pb_decode(stream, sns_std_attr_value_data_fields, &data->value);
    if (!success) {
      LOG_NANOPB_ERROR(stream);
    } else {
      auto *attrVal = static_cast<SeeAttrArg *>(*arg);
      if (data->value.has_flt) {
        // If this is a float (repeated) field, initialize the union as floats
        // to store the maximum and minmum values of the repeated fields.
        if (!attrVal->initialized) {
          attrVal->initialized = true;
          attrVal->fltMin = FLT_MAX;
          attrVal->fltMax = FLT_MIN;
        }
        if (data->value.flt < attrVal->fltMin) {
          attrVal->fltMin = data->value.flt;
        }
        if (data->value.flt > attrVal->fltMax) {
          attrVal->fltMax = data->value.flt;
        }
      } else if (data->value.has_sint) {
        attrVal->int64 = data->value.sint;
      } else if (data->value.has_boolean) {
        attrVal->boolVal = data->value.boolean;
      } else if (data->strData.buf != nullptr) {
        strlcpy(attrVal->strVal, static_cast<const char *>(data->strData.buf),
                sizeof(attrVal->strVal));
      } else if (!data->value.has_subtype) {
        LOGW("Unknown attr type");
      }
    }
  }
  return success;
}

bool decodeSnsStrAttr(pb_istream_t *stream, const pb_field_t *field,
                      void **arg) {
  bool success = false;

  struct Decodedata {
    SeeAttrArg attrArg;
    sns_std_attr attr;
  };
  auto data = MakeUniqueZeroFill<Decodedata>();

  if (data.isNull()) {
    LOG_OOM();
  } else {
    data->attr.value.values.funcs.decode = decodeSnsStdAttrValue;
    data->attr.value.values.arg = &data->attrArg;

    success = pb_decode(stream, sns_std_attr_fields, &data->attr);
    if (!success) {
      LOG_NANOPB_ERROR(stream);
    } else {
      auto *attrData = static_cast<SeeAttributes *>(*arg);
      switch (data->attr.attr_id) {
        case SNS_STD_SENSOR_ATTRID_NAME:
          strlcpy(attrData->name, data->attrArg.strVal, sizeof(attrData->name));
          break;
        case SNS_STD_SENSOR_ATTRID_VENDOR:
          strlcpy(attrData->vendor, data->attrArg.strVal,
                  sizeof(attrData->vendor));
          break;
        case SNS_STD_SENSOR_ATTRID_TYPE:
          strlcpy(attrData->type, data->attrArg.strVal, sizeof(attrData->type));
          break;
        case SNS_STD_SENSOR_ATTRID_AVAILABLE:
          if (!data->attrArg.boolVal) {
            LOGW("%s: %d", getAttrNameFromAttrId(data->attr.attr_id),
                 data->attrArg.boolVal);
          }
          break;
        case SNS_STD_SENSOR_ATTRID_RATES:
          attrData->maxSampleRate = data->attrArg.fltMax;
          break;
        case SNS_STD_SENSOR_ATTRID_STREAM_TYPE:
          attrData->streamType = data->attrArg.int64;
          break;
        case SNS_STD_SENSOR_ATTRID_HW_ID:
          attrData->hwId = data->attrArg.int64;
          break;
        case SNS_STD_SENSOR_ATTRID_PASSIVE_REQUEST:
          attrData->passiveRequest = data->attrArg.boolVal;
          break;
        default:
          break;
      }
    }
  }
  return success;
}

bool decodeSnsStdAttrEvent(pb_istream_t *stream, const pb_field_t *field,
                           void **arg) {
  bool success = false;

  struct DecodeData {
    SeeAttributes attr;
    sns_std_attr_event event;
  };
  auto data = MakeUniqueZeroFill<DecodeData>();

  if (data.isNull()) {
    LOG_OOM();
  } else {
    data->event.attributes.funcs.decode = decodeSnsStrAttr;
    data->event.attributes.arg = &data->attr;

    success = pb_decode(stream, sns_std_attr_event_fields, &data->event);
    if (!success) {
      LOG_NANOPB_ERROR(stream);
    } else {
      auto *info = static_cast<SeeInfoArg *>(*arg);

      // If syncData == nullptr, this indication is received outside of a sync
      // call. If the decoded SUID doesn't match the one we are waiting for,
      // this indication is from a previous getAttributes call and happens to
      // arrive between a later findAttributesSync req/ind pair.
      // Note that req/ind misalignment can still happen if getAttributesSync is
      // called again with the same SUID.
      if (info->sync->syncData == nullptr
          || !suidsMatch(info->suid, info->sync->syncSuid)) {
        LOGW("Received late SNS_STD_MSGID_SNS_STD_ATTR_EVENT indication");
      } else {
        info->sync->syncIndFound = true;
        memcpy(info->sync->syncData, &data->attr, sizeof(data->attr));
      }
    }
  }
  return success;
}

/**
 * Decode messages defined in sns_std.proto
 */
bool decodeSnsStdProtoEvent(pb_istream_t *stream, const pb_field_t *field,
                            void **arg) {
  bool success = false;

  auto *info = static_cast<SeeInfoArg *>(*arg);
  switch (info->msgId) {
    case SNS_STD_MSGID_SNS_STD_ATTR_EVENT:
      success = decodeSnsStdAttrEvent(stream, field, arg);
      break;

    case SNS_STD_MSGID_SNS_STD_FLUSH_EVENT:
      // An empty message.
      success = true;
      break;

    case SNS_STD_MSGID_SNS_STD_ERROR_EVENT: {
      sns_std_error_event event = {};
      success = pb_decode(stream, sns_std_error_event_fields, &event);
      if (!success) {
        LOG_NANOPB_ERROR(stream);
      } else {
        LOGW("SNS_STD_MSGID_SNS_STD_ERROR_EVENT: %d", event.error);
      }
      break;
    }

    default:
      LOGW("Unhandled sns_std.proto msg ID %" PRIu32, info->msgId);
  }
  return success;
}

// TODO: Support compensation matrix and scaling factor calibration
void applyThreeAxisCalibration(
    chreSensorThreeAxisData::chreSensorThreeAxisSampleData *sample,
    const float *val, const SeeCalData *cal) {
  float bias[3] = {};
  if (cal != nullptr && cal->hasBias) {
    memcpy(bias, cal->bias, sizeof(bias));
  }
  sample->x = val[0] - bias[0];
  sample->y = val[1] - bias[1];
  sample->z = val[2] - bias[2];
}

void populateEventSample(SeeDataArg *data, const float *val) {
  size_t index = data->sampleIndex;
  if (!data->event.isNull() && index < data->totalSamples) {
    SensorSampleType sampleType = getSensorSampleTypeFromSensorType(
        data->sensorType);

    uint32_t *timestampDelta = nullptr;
    switch (sampleType) {
      case SensorSampleType::ThreeAxis: {
        auto *event = reinterpret_cast<chreSensorThreeAxisData *>(
            data->event.get());
        applyThreeAxisCalibration(&event->readings[index], val, data->cal);
        timestampDelta = &event->readings[index].timestampDelta;
        break;
      }

      case SensorSampleType::Float: {
        auto *event = reinterpret_cast<chreSensorFloatData *>(
            data->event.get());
        event->readings[index].value = val[0];
        timestampDelta = &event->readings[index].timestampDelta;
        break;
      }

      case SensorSampleType::Byte: {
        auto *event = reinterpret_cast<chreSensorByteData *>(data->event.get());
        event->readings[index].value = 0;
        event->readings[index].isNear = (val[0] > 0.5f);
        timestampDelta = &event->readings[index].timestampDelta;
        break;
      }

      case SensorSampleType::Occurrence: {
        auto *event = reinterpret_cast<chreSensorOccurrenceData *>(
            data->event.get());
        timestampDelta = &event->readings[index].timestampDelta;
        break;
      }

#ifdef CHREX_SENSOR_SUPPORT
      case SensorSampleType::Vendor0: {
        auto *event = reinterpret_cast<chrexSensorVendor0Data *>(
            data->event.get());
        memcpy(event->readings[index].values, val,
               sizeof(event->readings[index].values));
        timestampDelta = &event->readings[index].timestampDelta;
        break;
      }

      case SensorSampleType::Vendor1: {
        auto *event = reinterpret_cast<chrexSensorVendor1Data *>(
            data->event.get());
        memcpy(event->readings[index].values, val,
               sizeof(event->readings[index].values));
        timestampDelta = &event->readings[index].timestampDelta;
        break;
      }

      case SensorSampleType::Vendor2: {
        auto *event = reinterpret_cast<chrexSensorVendor2Data *>(
            data->event.get());
        event->readings[index].value = *val;
        timestampDelta = &event->readings[index].timestampDelta;
        break;
      }
#endif  // CHREX_SENSOR_SUPPORT

      default:
        LOGE("Invalid sample type %" PRIu8, static_cast<uint8_t>(sampleType));
    }

    if (data->sampleIndex == 0) {
      auto *header = reinterpret_cast<chreSensorDataHeader *>(
          data->event.get());
      header->baseTimestamp = data->timeNs;
      *timestampDelta = 0;
    } else {
      uint64_t delta = data->timeNs - data->prevTimeNs;
      if (delta > UINT32_MAX) {
        LOGE("Sensor %" PRIu8 " timestampDelta overflow: prev %" PRIu64
             " curr %" PRIu64, static_cast<uint8_t>(data->sensorType),
             data->prevTimeNs, data->timeNs);
        delta = UINT32_MAX;
      }
      *timestampDelta = static_cast<uint32_t>(delta);
    }
    data->prevTimeNs = data->timeNs;
  }
}

/**
 * Decodes a float array and ensures that the data doesn't go out of bound.
 */
bool decodeFloatData(pb_istream_t *stream, const pb_field_t *field,
                     void **arg) {
  auto *data = static_cast<SeeFloatArg *>(*arg);

  float value;
  float *fltPtr = &value;
  if (data->index >= ARRAY_SIZE(data->val)) {
    LOGE("Float array length exceeds %zu", ARRAY_SIZE(data->val));
  } else {
    // Decode to the provided array only if it doesn't go out of bound.
    fltPtr = &(data->val[data->index]);
  }
  // Increment index whether it's gone out of bounds or not.
  (data->index)++;

  bool success = pb_decode_fixed32(stream, fltPtr);
  if (!success) {
    LOG_NANOPB_ERROR(stream);
  }
  return success;
}

bool decodeSnsStdSensorPhysicalConfigEvent(
    pb_istream_t *stream, const pb_field_t *field, void **arg) {
  SeeBufArg data = {};
  sns_std_sensor_physical_config_event event = {
    .operation_mode.funcs.decode = decodeStringField,
    .operation_mode.arg = &data,
  };

  bool success = pb_decode(stream, sns_std_sensor_physical_config_event_fields,
                           &event);
  if (!success) {
    LOG_NANOPB_ERROR(stream);
  } else {
    auto statusData =
        MakeUniqueZeroFill<SeeHelperCallbackInterface::SamplingStatusData>();
    if (statusData.isNull()) {
      LOG_OOM();
    } else {
      struct chreSensorSamplingStatus *status = &statusData->status;

      if (event.has_sample_rate) {
        statusData->intervalValid = true;
        status->interval = static_cast<uint64_t>(
            ceilf(Seconds(1).toRawNanoseconds() / event.sample_rate));
      }

      // If operation_mode is populated, decoded string length will be > 0.
      if (data.bufLen > 0) {
        statusData->enabledValid = true;
        status->enabled =
            (strncmp(static_cast<const char *>(data.buf), kOpModeOff,
                     std::min(data.bufLen, sizeof(kOpModeOff))) != 0);
      }

      if (event.has_sample_rate || data.bufLen > 0) {
        auto *info = static_cast<SeeInfoArg *>(*arg);
        statusData->sensorType = info->data->sensorType;
        info->data->status = std::move(statusData);
      }
    }
  }
  return success;
}

bool decodeSnsStdSensorEvent(pb_istream_t *stream, const pb_field_t *field,
                             void **arg) {
  SeeFloatArg sample = {};
  sns_std_sensor_event event = {
    .data.funcs.decode = decodeFloatData,
    .data.arg = &sample,
  };

  bool success = pb_decode(stream, sns_std_sensor_event_fields, &event);
  if (!success) {
    LOG_NANOPB_ERROR(stream);
  } else {
    auto *info = static_cast<SeeInfoArg *>(*arg);
    populateEventSample(info->data, sample.val);
  }
  return success;
}

/**
 * Decode messages defined in sns_std_sensor.proto
 */
bool decodeSnsStdSensorProtoEvent(pb_istream_t *stream, const pb_field_t *field,
                                  void **arg) {
  bool success = false;

  auto *info = static_cast<SeeInfoArg *>(*arg);
  switch (info->msgId) {
    case SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_PHYSICAL_CONFIG_EVENT:
      success = decodeSnsStdSensorPhysicalConfigEvent(stream, field, arg);
      break;

    case SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_EVENT:
      success = decodeSnsStdSensorEvent(stream, field, arg);
      break;

    default:
      LOGW("Unhandled sns_std_sensor.proto msg ID %" PRIu32, info->msgId);
  }
  return success;
}

bool decodeSnsCalEvent(pb_istream_t *stream, const pb_field_t *field,
                       void **arg) {
  SeeFloatArg offset = {};
  SeeFloatArg scale = {};
  SeeFloatArg matrix = {};
  sns_cal_event event = {
    .bias.funcs.decode = decodeFloatData,
    .bias.arg = &offset,
    .scale_factor.funcs.decode = decodeFloatData,
    .scale_factor.arg = &scale,
    .comp_matrix.funcs.decode = decodeFloatData,
    .comp_matrix.arg = &matrix,
  };

  bool success = pb_decode(stream, sns_cal_event_fields, &event);
  if (!success) {
    LOG_NANOPB_ERROR(stream);
  } else {
    auto *info = static_cast<SeeInfoArg *>(*arg);
    SeeCalInfo *calInfo = info->calInfo;
    size_t calIndex = getCalIndexFromSuid(info->suid, calInfo);
    if (calIndex >= kNumSeeCalSensors) {
      LOGW("Cal sensor index out of bounds 0x%" PRIx64 " %" PRIx64,
           info->suid.suid_high, info->suid.suid_low);
    } else {
      SeeCalData *cal = &calInfo[calIndex].cal;

      cal->hasBias = (offset.index == 3);
      if (cal->hasBias) {
        memcpy(cal->bias, offset.val, sizeof(cal->bias));
      }

      cal->hasScale = (scale.index == 3);
      if (cal->hasScale) {
        memcpy(cal->scale, scale.val, sizeof(cal->scale));
      }

      cal->hasMatrix = (matrix.index == 9);
      if (cal->hasScale) {
        memcpy(cal->matrix, matrix.val, sizeof(cal->matrix));
      }

      cal->accuracy = static_cast<uint8_t>(event.status);
    }
  }
  return success;
}

/**
 * Decode messages defined in sns_cal.proto
 */
bool decodeSnsCalProtoEvent(pb_istream_t *stream, const pb_field_t *field,
                            void **arg) {
  bool success = false;

  auto *info = static_cast<SeeInfoArg *>(*arg);
  switch (info->msgId) {
    case SNS_CAL_MSGID_SNS_CAL_EVENT:
      success = decodeSnsCalEvent(stream, field, arg);
      break;

    default:
      LOGW("Unhandled sns_cal.proto msg ID %" PRIu32, info->msgId);
  }
  return success;
}

bool decodeSnsProximityEvent(pb_istream_t *stream, const pb_field_t *field,
                             void **arg) {
  sns_proximity_event event = {};

  bool success = pb_decode(stream, sns_proximity_event_fields, &event);
  if (!success) {
    LOG_NANOPB_ERROR(stream);
  } else {
    float value = static_cast<float>(event.proximity_event_type);
    auto *info = static_cast<SeeInfoArg *>(*arg);
    populateEventSample(info->data, &value);
  }
  return success;
}

/**
 * Decode messages defined in sns_proximity.proto
 */
bool decodeSnsProximityProtoEvent(pb_istream_t *stream, const pb_field_t *field,
                                  void **arg) {
  bool success = false;

  auto *info = static_cast<SeeInfoArg *>(*arg);
  switch (info->msgId) {
    case SNS_PROXIMITY_MSGID_SNS_PROXIMITY_EVENT:
      success = decodeSnsProximityEvent(stream, field, arg);
      break;

    default:
      LOGW("Unhandled sns_proximity.proto msg ID %" PRIu32, info->msgId);
  }
  return success;
}

bool decodeSnsRemoteProcStateEvent(
    pb_istream_t *stream, const pb_field_t *field, void **arg) {
  sns_remote_proc_state_event event = sns_remote_proc_state_event_init_default;
  bool success = pb_decode(stream, sns_remote_proc_state_event_fields, &event);
  if (!success) {
    LOG_NANOPB_ERROR(stream);
  } else if (event.proc_type == SNS_STD_CLIENT_PROCESSOR_APSS) {
    auto *info = static_cast<SeeInfoArg *>(*arg);
    info->data->isHostWakeSuspendEvent = true;
    info->data->isHostAwake =
        (event.event_type == SNS_REMOTE_PROC_STATE_AWAKE);
  }
  return success;
}

/**
 * Decode messages defined in sns_remote_proc_state.proto
 */
bool decodeSnsRemoteProcProtoEvent(
    pb_istream_t *stream, const pb_field_t *field, void **arg) {
  bool success = false;
  auto *info = static_cast<SeeInfoArg *>(*arg);
  switch (info->msgId) {
    case SNS_REMOTE_PROC_STATE_MSGID_SNS_REMOTE_PROC_STATE_EVENT:
      success = decodeSnsRemoteProcStateEvent(stream, field, arg);
      break;

    default:
      LOGW("Unhandled sns_remote_proc_state.proto msg ID %" PRIu32,
           info->msgId);
  }
  return success;
}

bool assignPayloadCallback(const SeeInfoArg *info, pb_callback_t *payload) {
  bool success = true;

  payload->arg = const_cast<SeeInfoArg *>(info);

  if (info->remoteProcSuid->has_value()
      && suidsMatch(info->suid, info->remoteProcSuid->value())) {
    payload->funcs.decode = decodeSnsRemoteProcProtoEvent;
  } else if (suidsMatch(info->suid, kSuidLookup)) {
    payload->funcs.decode = decodeSnsSuidProtoEvent;
  } else {
    // Assumed: "real" sensors SUIDs
    switch (info->msgId) {
      case SNS_STD_MSGID_SNS_STD_ATTR_EVENT:
      case SNS_STD_MSGID_SNS_STD_FLUSH_EVENT:
      case SNS_STD_MSGID_SNS_STD_ERROR_EVENT:
        payload->funcs.decode = decodeSnsStdProtoEvent;
        break;

      case SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_PHYSICAL_CONFIG_EVENT:
      case SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_EVENT:
        payload->funcs.decode = decodeSnsStdSensorProtoEvent;
        break;

      case SNS_CAL_MSGID_SNS_CAL_EVENT:
        payload->funcs.decode = decodeSnsCalProtoEvent;
        break;

      case SNS_PROXIMITY_MSGID_SNS_PROXIMITY_EVENT:
        payload->funcs.decode = decodeSnsProximityProtoEvent;
        break;

      default:
        success = false;
        LOGW("Unhandled msg ID %" PRIu32, info->msgId);
    }
  }
  return success;
}

/**
 * Decodes only msg_id and timestamp defined in sns_client_event and converts
 * the timestamp to nanoseconds.
 */
bool decodeMsgIdAndTime(pb_istream_t *stream, uint32_t *msgId,
                        uint64_t *timeNs) {
  sns_client_event_msg_sns_client_event event = {};

  bool success = pb_decode(
      stream, sns_client_event_msg_sns_client_event_fields, &event);
  if (!success) {
    LOG_NANOPB_ERROR(stream);
  } else {
    *msgId = event.msg_id;
    *timeNs = getNanosecondsFromQTimerTicks(event.timestamp);
  }
  return success;
}

/**
 * Decodes pb-encoded message
 */
bool decodeSnsClientEventMsg(pb_istream_t *stream, const pb_field_t *field,
                             void **arg) {
  // Make a copy for data decoding.
  pb_istream_t streamCpy = *stream;

  auto *info = static_cast<SeeInfoArg *>(*arg);
  bool success = decodeMsgIdAndTime(stream, &info->msgId, &info->data->timeNs);

  if (!info->decodeMsgIdOnly) {
    sns_client_event_msg_sns_client_event event = {};

    // Payload callback must be assigned if and only if we want to decode beyond
    // msg ID.
    success = assignPayloadCallback(info, &event.payload);
    if (!success) {
      LOGE("No pb callback assigned");
    } else {
      success = pb_decode(&streamCpy,
                          sns_client_event_msg_sns_client_event_fields, &event);
      if (!success) {
        LOG_NANOPB_ERROR(&streamCpy);
      }
    }
  }

  // Increment sample count only after sensor event decoding.
  if (success
      && (info->msgId == SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_EVENT
          || info->msgId == SNS_PROXIMITY_MSGID_SNS_PROXIMITY_EVENT)) {
    info->data->sampleIndex++;
  }
  return success;
}

/**
 * Obtain the SensorType from the list of registered SensorInfos.
 */
SensorType getSensorTypeFromSensorInfo(
    qmi_client_type qmiHandle, const sns_std_suid& suid,
    const DynamicVector<SeeHelper::SensorInfo>& sensorInfos) {
  for (const auto& sensorInfo : sensorInfos) {
    if (suidsMatch(sensorInfo.suid, suid)
        && sensorInfo.qmiHandle == qmiHandle) {
        return sensorInfo.sensorType;
    }
  }
  return SensorType::Unknown;
}

/**
 * Allocate event memory according to SensorType and the number of samples.
 */
void *allocateEvent(SensorType sensorType, size_t numSamples) {
  SensorSampleType sampleType = getSensorSampleTypeFromSensorType(sensorType);
  size_t sampleSize = 0;
  switch (sampleType) {
    case SensorSampleType::ThreeAxis:
      sampleSize = sizeof(
          chreSensorThreeAxisData::chreSensorThreeAxisSampleData);
      break;

    case SensorSampleType::Float:
      sampleSize = sizeof(
          chreSensorFloatData::chreSensorFloatSampleData);
      break;

    case SensorSampleType::Byte:
      sampleSize = sizeof(
          chreSensorByteData::chreSensorByteSampleData);
      break;

    case SensorSampleType::Occurrence:
      sampleSize = sizeof(
          chreSensorOccurrenceData::chreSensorOccurrenceSampleData);
      break;

#ifdef CHREX_SENSOR_SUPPORT
    case SensorSampleType::Vendor0:
      sampleSize = sizeof(chrexSensorVendor0SampleData);
      break;

    case SensorSampleType::Vendor1:
      sampleSize = sizeof(chrexSensorVendor1SampleData);
      break;

    case SensorSampleType::Vendor2:
      sampleSize = sizeof(chrexSensorVendor2SampleData);
      break;
#endif  // CHREX_SENSOR_SUPPORT

    default:
      LOGW("Unhandled SensorSampleType %" PRIu8,
           static_cast<uint8_t>(sampleType));
  }

  size_t memorySize = (sampleType == SensorSampleType::Unknown)
      ? 0 : (sizeof(chreSensorDataHeader) + numSamples * sampleSize);
  return memoryAlloc(memorySize);
}

// Allocates the sensor event memory and partially populates the header.
bool prepareSensorEvent(SeeInfoArg& info) {
  bool success = false;

  UniquePtr<uint8_t> buf(static_cast<uint8 *>(
      allocateEvent(info.data->sensorType, info.data->sampleIndex)));
  info.data->event = std::move(buf);

  if (info.data->event.isNull()) {
    LOG_OOM();
  } else {
    success = true;

    info.data->prevTimeNs = 0;

    auto *header = reinterpret_cast<chreSensorDataHeader *>(
        info.data->event.get());
    memset(header->reserved, 0, sizeof(header->reserved));
    header->sensorHandle = getSensorHandleFromSensorType(
        info.data->sensorType);
    header->readingCount = info.data->sampleIndex;

    // Protect against out of bounds access in data decoding.
    info.data->totalSamples = info.data->sampleIndex;

    // Reset sampleIndex only after memory has been allocated and header
    // populated.
    info.data->sampleIndex = 0;
  }
  return success;
}

}  // anonymous namespace

SeeHelper::~SeeHelper() {
  for (const auto& handle : mQmiHandles) {
    qmi_client_error_type status = qmi_client_release(handle);
    if (status != QMI_NO_ERR) {
      LOGE("Failed to release sensor client service QMI client: %d", status);
    }
  }
}

void SeeHelper::handleSnsClientEventMsg(
    qmi_client_type qmiHandle, const void *payload, size_t payloadLen) {
  CHRE_ASSERT(payload);

  pb_istream_t stream = pb_istream_from_buffer(
      static_cast<const pb_byte_t *>(payload), payloadLen);

  // Make a copy of the stream for sensor data decoding.
  pb_istream_t streamCpy = stream;

  struct DecodeData {
    SeeSyncArg syncArg = {};
    SeeDataArg dataArg = {};
    SeeInfoArg info = {};
    sns_client_event_msg event = {};
  };
  auto data = MakeUnique<DecodeData>();

  if (data.isNull()) {
    LOG_OOM();
  } else {
    // Only initialize fields that are not accessed in the main CHRE thread.
    data->info.qmiHandle = qmiHandle;
    data->info.sync = &data->syncArg;
    data->info.data = &data->dataArg;
    data->info.decodeMsgIdOnly = true;
    data->info.remoteProcSuid = &mRemoteProcSuid;
    data->info.calInfo = &mCalInfo[0];
    data->event.events.funcs.decode = decodeSnsClientEventMsg;
    data->event.events.arg = &data->info;

    // Decode only SUID and MSG ID to help further decode.
    if (pb_decode(&stream, sns_client_event_msg_fields, &data->event)) {
      data->info.suid = data->event.suid;
      data->info.decodeMsgIdOnly = false;
      data->info.data->cal = getCalDataFromSuid(data->info.suid);
      data->info.data->sensorType = getSensorTypeFromSensorInfo(
          data->info.qmiHandle, data->info.suid, mSensorInfos);

      mMutex.lock();
      bool synchronizedDecode = mWaiting;
      if (!synchronizedDecode) {
        // Early unlock, we're not going to use anything from the main thread.
        mMutex.unlock();
      } else {
        // Populate fields set by the main thread.
        data->info.sync->syncData = mSyncData;
        data->info.sync->syncDataType = mSyncDataType;
        data->info.sync->syncSuid = mSyncSuid;
      }

      if (data->info.data->sampleIndex > 0 && !prepareSensorEvent(data->info)) {
        LOGE("Failed to prepare sensor event");
      }

      if (!pb_decode(&streamCpy, sns_client_event_msg_fields, &data->event)) {
        LOG_NANOPB_ERROR(&streamCpy);
      } else if (synchronizedDecode && data->info.sync->syncIndFound) {
        mWaiting = false;
        mCond.notify_one();
      } else {
        if (data->info.data->isHostWakeSuspendEvent) {
          mCbIf->onHostWakeSuspendEvent(data->info.data->isHostAwake);
        }
        if (!data->info.data->event.isNull()) {
          mCbIf->onSensorDataEvent(
              data->info.data->sensorType, std::move(data->info.data->event));
        }
        if (!data->info.data->status.isNull()) {
          mCbIf->onSamplingStatusUpdate(std::move(data->info.data->status));
        }
      }

      if (synchronizedDecode) {
        mMutex.unlock();
      }
    }
  }
}

bool SeeHelper::findSuidSync(const char *dataType,
                             DynamicVector<sns_std_suid> *suids,
                             uint8_t minNumSuids, uint32_t maxRetries,
                             Milliseconds retryDelay) {
  CHRE_ASSERT(suids != nullptr);
  CHRE_ASSERT(minNumSuids > 0);

  bool success = false;
  if (mQmiHandles.empty()) {
    LOGE("Sensor client service QMI client wasn't initialized");
  } else {
    UniquePtr<pb_byte_t> msg;
    size_t msgLen;
    if (encodeSnsSuidReq(dataType, &msg, &msgLen)) {
      // Sensor client QMI service may come up before SEE sensors are
      // enumerated. A max dwell time is set and retries are performed as
      // currently there's no message indicating that SEE intialization is
      // complete.
      uint32_t trialCount = 0;
      do {
        suids->clear();
        if (++trialCount > 1) {
          timer_sleep(retryDelay.getMilliseconds(), T_MSEC,
                      true /* non_deferrable */);
        }
        // Ignore failures from sendReq, we'll retry anyways (up to maxRetries)
        sendReq(mQmiHandles[0], sns_suid_sensor_init_default,
                suids, dataType,
                SNS_SUID_MSGID_SNS_SUID_REQ, msg.get(), msgLen,
                false /* batchValid */, 0 /* batchPeriodUs */,
                false /* passive */, true /* waitForIndication */);
      } while (suids->size() < minNumSuids && trialCount < maxRetries);

      success = (suids->size() >= minNumSuids);
      if (trialCount > 1) {
        LOGD("Waited %" PRIu32 " ms for %s (found: %d)",
             static_cast<uint32_t>(trialCount * retryDelay.getMilliseconds()),
             dataType, success);
      }
    }
  }

  return success;
}

bool SeeHelper::getAttributesSync(const sns_std_suid& suid,
                                  SeeAttributes *attr) {
  CHRE_ASSERT(attr);
  bool success = false;

  if (mQmiHandles.empty()) {
    LOGE("Sensor client service QMI client wasn't initialized");
  } else {
    UniquePtr<pb_byte_t> msg;
    size_t msgLen;
    success = encodeSnsStdAttrReq(&msg, &msgLen);

    if (success) {
      success = sendReq(mQmiHandles[0], suid,
                        attr, nullptr /* syncDataType */,
                        SNS_STD_MSGID_SNS_STD_ATTR_REQ, msg.get(), msgLen,
                        false /* batchValid */, 0 /* batchPeriodUs */,
                        false /* passive */, true /* waitForIndication */);
    }
  }
  return success;
}

bool SeeHelper::init(SeeHelperCallbackInterface *cbIf, Microseconds timeout) {
  CHRE_ASSERT(cbIf);

  mCbIf = cbIf;
  qmi_client_type qmiHandle;

  // Initialize cal/remote_proc_state sensors before making sensor data request.
  return (waitForService(&qmiHandle, timeout)
          && mQmiHandles.push_back(qmiHandle)
          && initCalSensors()
          && initRemoteProcSensor());
}

bool SeeHelper::makeRequest(const SeeSensorRequest& request) {
  bool success = false;
  const SensorInfo *sensorInfo = getSensorInfo(request.sensorType);
  if (sensorInfo == nullptr) {
    LOGE("SensorType %" PRIu8 " hasn't been registered",
         static_cast<uint8_t>(request.sensorType));
  } else {
    uint32_t msgId;
    UniquePtr<pb_byte_t> msg;
    size_t msgLen = 0;

    if (!request.enable) {
      // An empty message
      msgId = SNS_CLIENT_MSGID_SNS_CLIENT_DISABLE_REQ;
      success = true;
    } else if (sensorTypeIsContinuous(request.sensorType)) {
      msgId = SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_CONFIG;
      success = encodeSnsStdSensorConfig(request, &msg, &msgLen);
    } else {
      msgId = SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG;
      // No sample rate needed to configure on-change or one-shot sensors.
      success = true;
    }

    if (success) {
      success = sendReq(sensorInfo->qmiHandle, sensorInfo->suid,
                        nullptr /* syncData */, nullptr /* syncDataType */,
                        msgId, msg.get(), msgLen,
                        true /* batchValid */, request.batchPeriodUs,
                        request.passive, false /* waitForIndication */);
    }
  }
  return success;
}

const sns_std_suid& SeeHelper::getCalSuidFromSensorType(
    SensorType sensorType) const {
  static sns_std_suid suid = sns_suid_sensor_init_zero;

  size_t calIndex = getCalIndexFromSensorType(sensorType);
  if (calIndex < kNumSeeCalSensors && mCalInfo[calIndex].suid.has_value()) {
    suid = mCalInfo[calIndex].suid.value();
  }
  return suid;
}

bool SeeHelper::sendReq(
    const qmi_client_type& qmiHandle, const sns_std_suid& suid,
    void *syncData, const char *syncDataType,
    uint32_t msgId, void *payload, size_t payloadLen,
    bool batchValid, uint32_t batchPeriodUs, bool passive,
    bool waitForIndication, Nanoseconds timeoutResp, Nanoseconds timeoutInd) {
  CHRE_ASSERT(payload || payloadLen == 0);
  bool success = false;

  // TODO: when enabling uImage support, need to force big image here if still
  // using QMI directly.

  if (!waitForIndication) {
    success = sendSnsClientReq(qmiHandle, suid, msgId, payload, payloadLen,
                               batchValid, batchPeriodUs, passive, timeoutResp);
  } else {
    {
      LockGuard<Mutex> lock(mMutex);
      CHRE_ASSERT(!mWaiting);
      mWaiting = true;

      // Specify members needed for a sync call.
      mSyncSuid = suid;
      mSyncData = syncData;
      mSyncDataType = syncDataType;
    }

    success = sendSnsClientReq(qmiHandle, suid, msgId, payload, payloadLen,
                               batchValid, batchPeriodUs, passive, timeoutResp);

    {
      LockGuard<Mutex> lock(mMutex);
      if (success) {
        bool waitSuccess = true;

        while (mWaiting && waitSuccess) {
          waitSuccess = mCond.wait_for(mMutex, timeoutInd);
        }

        if (!waitSuccess) {
          LOGE("QMI indication timed out after %" PRIu64 " ms",
               Milliseconds(timeoutInd).getMilliseconds());
          success = false;
        }
      }
      mWaiting = false;

      // Reset members needed for a sync call.
      mSyncSuid = sns_suid_sensor_init_zero;
      mSyncData = nullptr;
      mSyncDataType = nullptr;
    }
  }
  return success;
}

void SeeHelper::handleInd(qmi_client_type clientHandle, unsigned int msgId,
                          const void *indBuf, unsigned int indBufLen) {
  CHRE_ASSERT(indBuf || indBufLen == 0);

  switch (msgId) {
    case SNS_CLIENT_REPORT_IND_V01: {
      // Decode sns_client_report_ind_msg_v01 to extract pb-encoded payload.
      auto ind = MakeUniqueZeroFill<sns_client_report_ind_msg_v01>();

      if (ind.isNull()) {
        LOG_OOM();
      } else {
        int status = qmi_client_message_decode(
            clientHandle, QMI_IDL_INDICATION, SNS_CLIENT_REPORT_IND_V01,
            indBuf, indBufLen, ind.get(), sizeof(*ind));
        if (status != QMI_NO_ERR) {
          LOGE("Error parsing SNS_CLIENT_REPORT_IND_V01: %d", status);
        } else {
          handleSnsClientEventMsg(clientHandle, ind->payload, ind->payload_len);
        }
      }
      break;
    }

    default:
      // TODO: handle sns_client_jumbo_report_ind_msg_v01.
      LOGW("Unhandled sns_client_api_v01 msg ID %u", msgId);
  }
}

void SeeHelper::qmiIndCb(qmi_client_type clientHandle, unsigned int msgId,
                         void *indBuf, unsigned int indBufLen,
                         void *indCbData) {
  auto *obj = static_cast<SeeHelper *>(indCbData);
  obj->handleInd(clientHandle, msgId, indBuf, indBufLen);
}

bool SeeHelper::registerSensor(SensorType sensorType, const sns_std_suid& suid,
                               bool *prevRegistered) {
  CHRE_ASSERT(sensorType != SensorType::Unknown);
  CHRE_ASSERT(prevRegistered != nullptr);
  bool success = false;

  // Check whether the SUID/SensorType pair has been previously registered.
  // Also count how many other SensorTypes the SUID has been registered with.
  *prevRegistered = false;
  size_t suidRegCount = 0;
  for (const auto& sensorInfo : mSensorInfos) {
    if (suidsMatch(suid, sensorInfo.suid)) {
      suidRegCount++;
      if (sensorInfo.sensorType == sensorType) {
        *prevRegistered = true;
      }
    }
  }

  // Initialize another QMI client if the SUID has been previously registered
  // with more SensorTypes than the number of QMI clients can disambiguate.
  bool qmiClientAvailable = true;
  if (mQmiHandles.size() <= suidRegCount) {
    qmi_client_type qmiHandle;
    qmiClientAvailable = waitForService(&qmiHandle);
    if (qmiClientAvailable) {
      qmiClientAvailable = mQmiHandles.push_back(qmiHandle);
    }
  }

  // Add a new entry only if this SUID/SensorType pair hasn't been registered.
  if (!*prevRegistered && qmiClientAvailable) {
    SensorInfo sensorInfo = {
      .suid = suid,
      .sensorType = sensorType,
      .qmiHandle = mQmiHandles[suidRegCount],
    };
    success = mSensorInfos.push_back(sensorInfo);
  }
  return success;
}

bool SeeHelper::sensorIsRegistered(SensorType sensorType) const {
  return (getSensorInfo(sensorType) != nullptr);
}

bool SeeHelper::waitForService(qmi_client_type *qmiHandle,
                               Microseconds timeout) {
  CHRE_ASSERT(qmiHandle);
  bool success = false;

  qmi_idl_service_object_type snsSvcObj =
      SNS_CLIENT_SVC_get_service_object_v01();
  if (snsSvcObj == nullptr) {
    LOGE("Failed to obtain the sensor client service instance");
  } else {
    qmi_client_os_params sensorContextOsParams;
    qmi_client_error_type status = qmi_client_init_instance(
        snsSvcObj, QMI_CLIENT_INSTANCE_ANY, SeeHelper::qmiIndCb, this,
        &sensorContextOsParams, timeout.getMicroseconds(), qmiHandle);

    success = (status == QMI_NO_ERR);
    if (!success) {
      LOGE("Failed to initialize the sensor client service QMI client: %d",
           status);
    }
  }
  return success;
}

bool SeeHelper::initCalSensors() {
  bool success = true;

  // Zero out mCalInfo to avoid accidental suid and data match.
  memset(mCalInfo, 0, sizeof(mCalInfo));

  const char *kCalTypes[] = {
    "accel_cal",
    "gyro_cal",
    "mag_cal",
  };

  // Find the cal sensor's SUID, assign it to mCalInfo, and make cal sensor data
  // request.
  DynamicVector<sns_std_suid> suids;
  for (size_t i = 0; i < ARRAY_SIZE(kCalTypes); i++) {
    const char *calType = kCalTypes[i];
    if (!findSuidSync(calType, &suids)) {
      success = false;
      LOGE("Failed to find sensor '%s'", calType);
    } else {
      size_t index = getCalIndexFromDataType(calType);
      if (index >= kNumSeeCalSensors) {
        success = false;
        LOGE("Cal sensor '%s' index out of bounds", calType);
      } else {
        mCalInfo[index].suid = suids[0];

        if (!sendReq(mQmiHandles[0], suids[0],
                     nullptr /* syncData */, nullptr /* syncDataType */,
                     SNS_STD_SENSOR_MSGID_SNS_STD_ON_CHANGE_CONFIG,
                     nullptr /* msg */, 0 /* msgLen */,
                     false /* batchValid */, 0 /* batchPeriodUs */,
                     false /* passive */, false /* waitForIndication */)) {
          success = false;
          LOGE("Failed to request '%s' data", calType);
        }
      }
    }
  }

  return success;
}

bool SeeHelper::initRemoteProcSensor() {
  bool success = false;

  const char *kRemoteProcType = "remote_proc_state";
  DynamicVector<sns_std_suid> suids;
  if (!findSuidSync(kRemoteProcType, &suids)) {
    LOGE("Failed to find sensor '%s'", kRemoteProcType);
  } else {
    mRemoteProcSuid = suids[0];

    uint32_t msgId = SNS_REMOTE_PROC_STATE_MSGID_SNS_REMOTE_PROC_STATE_CONFIG;
    constexpr size_t kBufferSize = sns_remote_proc_state_config_size;
    pb_byte_t msgBuffer[kBufferSize];
    size_t msgLen;
    if (encodeSnsRemoteProcSensorConfig(msgBuffer, kBufferSize, &msgLen,
                                        SNS_STD_CLIENT_PROCESSOR_APSS)) {
      success = sendReq(mQmiHandles[0], mRemoteProcSuid.value(),
                        nullptr /* syncData */, nullptr /* syncDataType */,
                        msgId, msgBuffer, msgLen,
                        false /* batchValid */, 0 /* batchPeriodUs */,
                        false /* passive */, false /* waitForIndication */);
      if (!success) {
        LOGE("Failed to request '%s' config", kRemoteProcType);
      }
    }
  }

  return success;
}

SeeCalData *SeeHelper::getCalDataFromSuid(const sns_std_suid& suid) {
  size_t calIndex = getCalIndexFromSuid(suid, mCalInfo);
  return (calIndex < kNumSeeCalSensors) ? &mCalInfo[calIndex].cal : nullptr;
}

const SeeHelper::SensorInfo *SeeHelper::getSensorInfo(
    SensorType sensorType) const {
  for (const auto& sensorInfo : mSensorInfos) {
    if (sensorInfo.sensorType == sensorType) {
      return &sensorInfo;
    }
  }
  return nullptr;
}

}  // namespace chre
