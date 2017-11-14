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

#include "stringl.h"
#include "timer.h"

#include <cfloat>
#include <cinttypes>

#include "chre/platform/assert.h"
#include "chre/platform/log.h"
#include "chre/platform/slpi/power_control_util.h"
#include "chre/util/lock_guard.h"

namespace chre {
namespace {

//! Interval between SUID request retry.
constexpr Milliseconds kSuidReqIntervalMsec = Milliseconds(100);

//! Maximum dwell time to try a data type's SUID request.
constexpr Seconds kSuidReqMaxDwellSec = Seconds(10);

//! A struct to facilitate pb encode/decode
struct SeeBufArg {
  const void *buf;
  size_t bufLen;
};

//! A struct to facilitate pb decode
struct SeeInfoArg {
  SeeIndCallback *indCb;
  void *syncData;
  sns_std_suid suid;
  uint32_t msgId;
  bool syncIndFound;
  const char *syncDataType;
  sns_std_suid syncSuid;
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
    struct {
      int64_t intMin;
      int64_t intMax;
    };
  };
  bool initialized;
};

bool suidsMatch(const sns_std_suid& suid0, const sns_std_suid& suid1) {
  return (suid0.suid_high == suid1.suid_high
          && suid0.suid_low == suid1.suid_low);
}

/**
 * Copy an encoded pb message to a wrapper proto's field.
 */
bool copyPayload(pb_ostream_t *stream, const pb_field_t *field,
                 void *const *arg) {
  bool success = false;

  auto *data = static_cast<const SeeBufArg *>(*arg);
  if (!pb_encode_tag_for_field(stream, field)) {
    LOGE("Failed encoding pb tag");
  } else if (!pb_encode_string(
      stream, static_cast<const pb_byte_t *>(data->buf), data->bufLen)) {
    LOGE("Failed encoding pb string");
  } else {
    success = true;
  }
  return success;
}

/**
 * Encodes sns_std_attr_req pb message.
 *
 * @param msgLenOnly Obtain only the message lengh or not.
 * @param msg A pointer to the address where the pb message will be encoded to.
 * @param msgLen A non-null pointer to the size of the encoded pb message.
 *
 * @return true if the pb message length and/or pb message is obtained.
 */
bool encodeSnsStdAttrReq(bool msgLenOnly, pb_byte_t *msg, size_t *msgLen) {
  CHRE_ASSERT(msgLen);
  bool success = false;

  // Initialize the pb message
  sns_std_attr_req req = sns_std_attr_req_init_zero;

  if (msgLenOnly) {
    success = pb_get_encoded_size(msgLen, sns_std_attr_req_fields, &req);
    if (!success) {
      LOGE("pb_get_encoded_size failed for sns_str_attr_req");
    }
  } else {
    // The encoded size can be 0 as there's only one optional field.
    CHRE_ASSERT(msg || *msgLen == 0);
    if (msg == nullptr && *msgLen > 0) {
      LOGE("No memory allocated to encode sns_std_attr_req");
    } else {
      pb_ostream_t stream = pb_ostream_from_buffer(msg, *msgLen);

      success = pb_encode(&stream, sns_std_attr_req_fields, &req);
      if (!success) {
        LOGE("Error encoding sns_std_attr_req: %s", PB_GET_ERROR(&stream));
      }
    }
  }
  return success;
}

/**
 * Encodes sns_suid_req pb message.
 *
 * @param dataType Sensor data type, "accel" for example.
 * @param msgLenOnly Obtain only the message lengh or not.
 * @param msg A non-null pointer to the address where the pb message will be
 *            encoded to.
 * @param msgLen A non-null pointer to the size of the encoded pb message.
 *
 * @return true if the pb message length and/or pb message is obtained.
 */
bool encodeSnsSuidReq(const char *dataType, bool msgLenOnly,
                      pb_byte_t *msg, size_t *msgLen) {
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

  if (msgLenOnly) {
    if (!pb_get_encoded_size(msgLen, sns_suid_req_fields, &req)) {
      LOGE("pb_get_encoded_size failed for sns_suid_req: %s", dataType);
    } else if (*msgLen == 0) {
      LOGE("Invalid pb encoded size for sns_suid_req");
    } else {
      success = true;
    }
  } else {
    CHRE_ASSERT(msg);
    if (msg == nullptr) {
      LOGE("No memory allocated to encode sns_suid_req");
    } else {
      pb_ostream_t stream = pb_ostream_from_buffer(msg, *msgLen);

      success = pb_encode(&stream, sns_suid_req_fields, &req);
      if (!success) {
        LOGE("Error encoding sns_suid_req: %s", PB_GET_ERROR(&stream));
      }
    }
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
    // TODO: Remove bypass and uncomment logging when b/68825825 is resolved.
    // LOGE("Client request failed with error %d", resp.resp.error);
    success = true;
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
                      Nanoseconds timeoutResp) {
  CHRE_ASSERT(payload || payloadLen == 0);
  bool success = false;

  // Initialize pb message to be encoded
  SeeBufArg data = {
    .buf = payload,
    .bufLen = payloadLen,
  };
  sns_client_request_msg pbMsg = {
    .suid = suid,
    .msg_id = msgId,
    .request.payload.funcs.encode = copyPayload,
    .request.payload.arg = &data,
  };

  auto qmiMsg = MakeUnique<sns_client_req_msg_v01>();
  if (qmiMsg.isNull()) {
    LOGE("Failed to allocate memory for sns_client_req_msg_v01");
  } else {
    // Create pb stream
    pb_ostream_t stream = pb_ostream_from_buffer(
        qmiMsg->payload, SNS_CLIENT_REQ_LEN_MAX_V01);

    // Encode pb message
    if (!pb_encode(&stream, sns_client_request_msg_fields, &pbMsg)) {
      LOGE("Error Encoding request: %s", PB_GET_ERROR(&stream));
    } else {
      qmiMsg->payload_len = stream.bytes_written;
      success = sendQmiReq(qmiHandle, *qmiMsg, timeoutResp);
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
  sns_std_suid suid = sns_std_suid_init_zero;
  bool success = pb_decode(stream, sns_std_suid_fields, &suid);
  if (!success) {
    LOGE("Error decoding sns_std_suid: %s", PB_GET_ERROR(stream));
  } else {
    auto *suids = static_cast<DynamicVector<sns_std_suid> *>(*arg);
    suids->push_back(suid);
    LOGD("Received SUID 0x%" PRIx64 " %" PRIx64, suid.suid_high, suid.suid_low);
  }
  return success;
}

/**
 * Decode messages defined in sns_suid.proto
 */
bool decodeSnsSuidEvent(pb_istream_t *stream, const pb_field_t *field,
                        void **arg) {
  bool success = false;

  auto *info = static_cast<SeeInfoArg *>(*arg);
  switch (info->msgId) {
    case SNS_SUID_MSGID_SNS_SUID_EVENT: {
      SeeBufArg data;
      DynamicVector<sns_std_suid> suids;
      sns_suid_event event = {
        .data_type.funcs.decode = decodeStringField,
        .data_type.arg = &data,
        .suid.funcs.decode = decodeSnsSuidEventSuid,
        .suid.arg = &suids,
      };

      success = pb_decode(stream, sns_suid_event_fields, &event);
      if (!success) {
        LOGE("Error decoding sns_suid_event: %s", PB_GET_ERROR(stream));
      } else {
        // TODO: remove dataType once initial development is complete.
        char dataType[data.bufLen + 1];
        memcpy(dataType, data.buf, data.bufLen);
        dataType[data.bufLen] = '\0';

        // If syncData == nullptr, this indication is received outside of a sync
        // call. If the decoded data type doesn't match the one we are waiting
        // for, this indication is from a previous call (may be findSuidSync)
        // and happens to arrive between another sync req/ind pair.
        // Note that req/ind misalignment can still happen if findSuidSync is
        // called again with the same data type.
        // Note that there's no need to compare the SUIDs as no other calls
        // but findSuidSync populate mWaitingDataType and can lead to a data
        // type match.
        if (info->syncData == nullptr
            || strncmp(info->syncDataType, dataType,
                       std::min(data.bufLen, kSeeAttrStrValLen)) != 0) {
          LOGW("Received late SNS_SUID_MSGID_SNS_SUID_EVENT indication");
        } else {
          info->syncIndFound = true;
          auto *outputSuids = static_cast<DynamicVector<sns_std_suid> *>(
              info->syncData);
          for (const auto& suid : suids) {
            outputSuids->push_back(suid);
          }
        }
        LOGD("Finished sns_suid_event of data type '%s'", dataType);
      }
      break;
    }

    default:
      LOGW("Unhandled msg ID for SUID: %" PRIu32, info->msgId);
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
  SeeBufArg strData = {
    .buf = nullptr,
    .bufLen = 0,
  };
  SeeAttrArg subtypeAttrArg;
  sns_std_attr_value_data value = {
    .str.funcs.decode = decodeStringField,
    .str.arg = &strData,
    .subtype.values.funcs.decode = decodeSnsStdAttrValue,
    .subtype.values.arg = &subtypeAttrArg,
  };

  bool success = pb_decode(stream, sns_std_attr_value_data_fields, &value);
  if (!success) {
    LOGE("Error decoding sns_std_attr_value_data: %s", PB_GET_ERROR(stream));
  } else {
    auto *attrVal = static_cast<SeeAttrArg *>(*arg);
    if (value.has_flt) {
      // If this is a float (repeated) field, initialize the union as floats
      // to store the maximum and minmum values of the repeated fields.
      if (!attrVal->initialized) {
        attrVal->initialized = true;
        attrVal->fltMin = FLT_MAX;
        attrVal->fltMax = FLT_MIN;
      }
      if (value.flt < attrVal->fltMin) {
        attrVal->fltMin = value.flt;
      }
      if (value.flt > attrVal->fltMax) {
        attrVal->fltMax = value.flt;
      }
    } else if (value.has_sint) {
      // If this is a intger (repeated) field, initialize the union as integers
      // to store the maximum and minmum values of the repeated fields.
      if (!attrVal->initialized) {
        attrVal->initialized = true;
        attrVal->intMin = INT64_MAX;
        attrVal->intMax = INT64_MIN;
      }
      if (value.sint < attrVal->intMin) {
        attrVal->intMin = value.sint;
      }
      if (value.sint > attrVal->intMax) {
        attrVal->intMax = value.sint;
      }
    } else if (value.has_boolean) {
      attrVal->boolVal = value.boolean;
    } else if (strData.buf != nullptr) {
      strlcpy(attrVal->strVal, static_cast<const char *>(strData.buf),
              sizeof(attrVal->strVal));
    } else if (!value.has_subtype) {
      LOGW("Unknown attr type");
    }
  }
  return success;
}

bool decodeSnsStrAttr(pb_istream_t *stream, const pb_field_t *field,
                      void **arg) {
  SeeAttrArg attrArg = {
    .initialized = false,
  };
  sns_std_attr attr = {
    .value.values.funcs.decode = decodeSnsStdAttrValue,
    .value.values.arg = &attrArg,
  };

  bool success = pb_decode(stream, sns_std_attr_fields, &attr);
  if (!success) {
    LOGE("Error decoding sns_std_attr: %s", PB_GET_ERROR(stream));
  } else {
    auto *attrData = static_cast<SeeAttributes *>(*arg);
    if (attr.attr_id == SNS_STD_SENSOR_ATTRID_VENDOR) {
      strlcpy(attrData->vendor, attrArg.strVal, sizeof(attrData->vendor));
    } else if (attr.attr_id == SNS_STD_SENSOR_ATTRID_NAME) {
      strlcpy(attrData->name, attrArg.strVal, sizeof(attrData->name));
    } else if (attr.attr_id == SNS_STD_SENSOR_ATTRID_TYPE) {
      LOGI("%s: '%s'", getAttrNameFromAttrId(attr.attr_id), attrArg.strVal);
      strlcpy(attrData->type, attrArg.strVal, sizeof(attrData->type));
    } else if (attr.attr_id == SNS_STD_SENSOR_ATTRID_RATES) {
      attrData->maxSampleRate = attrArg.fltMax;
    }
  }
  return success;
}

bool decodeSnsStdAttrEvent(pb_istream_t *stream, const pb_field_t *field,
                           void **arg) {
  SeeAttributes attr;
  sns_std_attr_event event = {
    .attributes.funcs.decode = decodeSnsStrAttr,
    .attributes.arg = &attr,
  };

  bool success = pb_decode(stream, sns_std_attr_event_fields, &event);
  if (!success) {
    LOGE("Error decoding sns_std_attr_event: %s", PB_GET_ERROR(stream));
  } else {
    auto *info = static_cast<SeeInfoArg *>(*arg);

    // If syncData == nullptr, this indication is received outside of a sync
    // call. If the decoded SUID doesn't match the one we are waiting for,
    // this indication is from a previous getAttributes call and happens to
    // arrive between a later findAttributesSync req/ind pair.
    // Note that req/ind misalignment can still happen if getAttributesSync is
    // called again with the same SUID.
    if (info->syncData == nullptr
        || !suidsMatch(info->suid, info->syncSuid)) {
      LOGW("Received late SNS_STD_MSGID_SNS_STD_ATTR_EVENT indication");
    } else {
      info->syncIndFound = true;
      memcpy(info->syncData, &attr, sizeof(attr));
    }
  }
  return success;
}

/**
 * Decode messages defined in sns_std.proto
 */
bool decodeSnsStdEvent(pb_istream_t *stream, const pb_field_t *field,
                      void **arg) {
  bool success = false;

  auto *info = static_cast<SeeInfoArg *>(*arg);
  switch (info->msgId) {
    case SNS_STD_MSGID_SNS_STD_ATTR_EVENT:
      success = decodeSnsStdAttrEvent(stream, field, arg);
      break;

    case SNS_STD_MSGID_SNS_STD_ERROR_EVENT: {
      sns_std_error_event event = sns_std_error_event_init_zero;
      success = pb_decode(stream, sns_std_error_event_fields, &event);
      if (!success) {
        LOGE("Error decoding sns_std_error_event: %s", PB_GET_ERROR(stream));
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

/**
 * Decodes pb-encoded msg_id defined in sns_client_event
 */
bool getMsgId(pb_istream_t *stream, uint32_t *msgId) {
  sns_client_event_msg_sns_client_event event =
    sns_client_event_msg_sns_client_event_init_zero;

  bool success = pb_decode(stream, sns_client_event_msg_sns_client_event_fields,
                           &event);
  if (!success) {
    LOGE("Error decoding msg ID: %s", PB_GET_ERROR(stream));
  } else {
    *msgId = event.msg_id;
  }
  return success;
}

/**
 * Decodes pb-encoded message
 */
bool decodeSnsClientEventMsg(pb_istream_t *stream, const pb_field_t *field,
                             void **arg) {
  bool success = false;

  // Decode msg_id first, which is required to further decode the payload of
  // sns_client_event.
  pb_istream_t stream_cpy = *stream;
  uint32_t msgId;
  if (getMsgId(&stream_cpy, &msgId)) {
    auto *info = static_cast<SeeInfoArg *>(*arg);
    info->msgId = msgId;

    sns_client_event_msg_sns_client_event event = {
      .payload.arg = info,
    };

    const sns_std_suid suidLookup = sns_suid_sensor_init_default;
    if (suidsMatch(info->suid, suidLookup)
        && info->msgId == SNS_SUID_MSGID_SNS_SUID_EVENT) {
      event.payload.funcs.decode = decodeSnsSuidEvent;
    } else {
      event.payload.funcs.decode = decodeSnsStdEvent;
    }

    success = pb_decode(stream, sns_client_event_msg_sns_client_event_fields,
                        &event);
    if (!success) {
      LOGE("Error decoding sns_client_event_msg_sns_client_event: %s",
           PB_GET_ERROR(stream));
    }
  }
  return success;
}

}  // anonymous


void SeeHelper::handleSnsClientEventMsg(const void *payload,
                                        size_t payloadLen) {
  CHRE_ASSERT(payload);

  // Decode SUID of the indication message first to help further decode.
  sns_client_event_msg event = sns_client_event_msg_init_zero;

  pb_istream_t stream = pb_istream_from_buffer(
      static_cast<const pb_byte_t *>(payload), payloadLen);
  if (!pb_decode(&stream, sns_client_event_msg_fields, &event)) {
    LOGE("Error decoding sns_client_event_msg: %s", PB_GET_ERROR(&stream));
  } else {
    // Only initialize fields that are not accessed in the main CHRE thread.
    SeeInfoArg info = {
      .indCb = mIndCb,
      .suid = event.suid,
      .syncIndFound = false,
    };
    event.events.funcs.decode = decodeSnsClientEventMsg;
    event.events.arg = &info;
    stream = pb_istream_from_buffer(
        static_cast<const pb_byte_t *>(payload), payloadLen);

    mMutex.lock();
    bool synchronizedDecode = mWaiting;
    if (!synchronizedDecode) {
      // Early unlock, we're not going to use anything from the main thread.
      mMutex.unlock();
    } else {
      // Populate fields set by the main thread.
      info.syncData = mSyncData;
      info.syncDataType = mWaitingDataType;
      info.syncSuid = mWaitingSuid;
    }

    if (!pb_decode(&stream, sns_client_event_msg_fields, &event)) {
      LOGE("Error decoding sns_client_event_msg: %s", PB_GET_ERROR(&stream));
    } else if (synchronizedDecode && info.syncIndFound) {
      mWaiting = false;
      mCond.notify_one();
    }

    if (synchronizedDecode) {
      mMutex.unlock();
    }
  }
}

bool SeeHelper::findSuidSync(const char *dataType,
                             DynamicVector<sns_std_suid> *suids) {
  CHRE_ASSERT(suids);
  bool success = false;

  if (mQmiHandle == nullptr) {
    LOGE("Sensor client service QMI client wasn't initialized.");
  } else {
    suids->clear();

    pb_byte_t *msg = nullptr;
    size_t msgLen;
    // Obtain only pb-encoded message size to allocate memory first.
    success = encodeSnsSuidReq(dataType, true, msg, &msgLen);

    if (success) {
      msg = static_cast<pb_byte_t *>(memoryAlloc(msgLen));
      if (msg == nullptr) {
        LOGE("Failed to allocate memory to encode sns_suid_req: %zu", msgLen);
      } else {
        success = encodeSnsSuidReq(dataType, false, msg, &msgLen);

        if (success) {
          // TODO: modify retry implementation  when b/69066253 is resolved.
          // Sensor client QMI service may come up before SEE sensors are
          // enumerated. A max dwell time is set and retries are performed as
          // currently there's no message indicating that SEE intialization is
          // complete.
          constexpr time_timetick_type kSuidReqIntervalUsec =
              kSuidReqIntervalMsec.getMicroseconds();
          constexpr uint32_t kSuidReqMaxTrialCount =
              kSuidReqMaxDwellSec.toRawNanoseconds()
              / kSuidReqIntervalMsec.toRawNanoseconds();

          uint32_t trialCount = 0;
          do {
            if (++trialCount > 1) {
              timer_sleep(kSuidReqIntervalUsec, T_USEC,
                          true /* non_deferrable */);
            }
            success = sendReq(sns_suid_sensor_init_default, suids, dataType,
                              SNS_SUID_MSGID_SNS_SUID_REQ, msg, msgLen,
                              true /* waitForIndication */);
          } while (suids->empty() && trialCount < kSuidReqMaxTrialCount);
          if (trialCount > 1) {
            LOGD("%" PRIu32 " trials took %" PRIu32 " msec", trialCount,
                   static_cast<uint32_t>(
                       trialCount * kSuidReqIntervalMsec.getMilliseconds()));
          }
        }
      }
      memoryFree(msg);
    }
  }
  return success;
}

bool SeeHelper::getAttributesSync(const sns_std_suid& suid,
                                  SeeAttributes *attr) {
  CHRE_ASSERT(attr);
  bool success = false;

  if (mQmiHandle == nullptr) {
    LOGE("Sensor client service QMI client wasn't initialized.");
  } else {
    pb_byte_t *msg = nullptr;
    size_t msgLen;
    // Obtain only pb-encoded message size to allocate memory first.
    success = encodeSnsStdAttrReq(true, msg, &msgLen);

    if (success) {
      msg = static_cast<pb_byte_t *>(memoryAlloc(msgLen));
      if (msg == nullptr && msgLen > 0) {
        LOGE("Failed to allocate memory for sns_std_attr_req: %zu", msgLen);
      } else {
        success = encodeSnsStdAttrReq(false, msg, &msgLen);

        if (success) {
          success = sendReq(suid, attr, nullptr,
                            SNS_STD_MSGID_SNS_STD_ATTR_REQ, msg, msgLen,
                            true /* waitForIndication */);
        }
      }
      memoryFree(msg);
    }
  }
  return success;
}

bool SeeHelper::release() {
  qmi_client_error_type status = qmi_client_release(mQmiHandle);
  if (status != QMI_NO_ERR) {
    LOGE("Failed to release sensor client service QMI client: %d", status);
  }

  mQmiHandle = nullptr;
  return (status == QMI_NO_ERR);
}

bool SeeHelper::initService(SeeIndCallback *indCb, Microseconds timeout) {
  bool success = false;

  mIndCb = indCb;
  if (indCb == nullptr) {
    LOGW("SeeHelper indication callback not provided");
  }

  qmi_idl_service_object_type snsSvcObj =
      SNS_CLIENT_SVC_get_service_object_v01();
  if (snsSvcObj == nullptr) {
    LOGE("Failed to obtain the sensor client service instance");
  } else {
    qmi_client_os_params sensorContextOsParams;
    qmi_client_error_type status = qmi_client_init_instance(
        snsSvcObj, QMI_CLIENT_INSTANCE_ANY, SeeHelper::qmiIndCb, this,
        &sensorContextOsParams, timeout.getMicroseconds(), &mQmiHandle);
    if (status != QMI_NO_ERR) {
      LOGE("Failed to initialize the sensor client service QMI client: %d",
           status);
    } else {
      success = true;
    }
  }
  return success;
}

bool SeeHelper::sendReq(
    const sns_std_suid& suid, void *syncData, const char *dataType,
    uint32_t msgId, void *payload, size_t payloadLen,
    bool waitForIndication, Nanoseconds timeoutResp, Nanoseconds timeoutInd) {
  CHRE_ASSERT(payload || payloadLen == 0);
  bool success = false;

  // Force big image as the future QMI-replacement interface may not be
  // supported in micro-image.
  slpiForceBigImage();

  if (!waitForIndication) {
    success = sendSnsClientReq(mQmiHandle, suid, msgId, payload, payloadLen,
                               timeoutResp);
  } else {
    LockGuard<Mutex> lock(mMutex);
    CHRE_ASSERT(!mWaiting);

    // Specify members needed for a sync call.
    mWaitingSuid = suid;
    mSyncData = syncData;
    mWaitingDataType = dataType;

    success = sendSnsClientReq(mQmiHandle, suid, msgId, payload, payloadLen,
                               timeoutResp);

    if (success) {
      bool waitSuccess = true;
      mWaiting = true;

      while (mWaiting && waitSuccess) {
        waitSuccess = mCond.wait_for(mMutex, timeoutInd);
      }

      if (!waitSuccess) {
        LOGE("QMI indication timed out after %" PRIu64 " ms",
             Milliseconds(timeoutInd).getMilliseconds());
        success = false;
        mWaiting = false;
      }
    }

    // Reset members needed for a sync call.
    mWaitingSuid = sns_suid_sensor_init_zero;
    mSyncData = nullptr;
    mWaitingDataType = nullptr;
  }
  return success;
}

void SeeHelper::handleInd(qmi_client_type clientHandle, unsigned int msgId,
                          const void *indBuf, unsigned int indBufLen) {
  CHRE_ASSERT(indBuf || indBufLen == 0);

  switch (msgId) {
    case SNS_CLIENT_REPORT_IND_V01: {
      // Decode sns_client_report_ind_msg_v01 to extract pb-encoded payload.
      auto ind = MakeUnique<sns_client_report_ind_msg_v01>();

      if (ind.isNull()) {
        LOGE("Failed to allocate memory for sns_client_report_ind_msg_v01");
      } else {
        int status = qmi_client_message_decode(
            clientHandle, QMI_IDL_INDICATION, SNS_CLIENT_REPORT_IND_V01,
            indBuf, indBufLen, ind.get(), sizeof(*ind));
        if (status != QMI_NO_ERR) {
          LOGE("Error parsing SNS_CLIENT_REPORT_IND_V01: %d", status);
        } else {
          handleSnsClientEventMsg(ind->payload, ind->payload_len);
        }
      }
      break;
    }

    default:
      // TODO: handle sns_client_jumbo_report_ind_msg_v01.
      LOGE("Unhandled sns_client_api_v01 msg ID %u", msgId);
  }
}

void SeeHelper::qmiIndCb(qmi_client_type clientHandle, unsigned int msgId,
                         void *indBuf, unsigned int indBufLen,
                         void *indCbData) {
  if (msgId != SNS_CLIENT_REPORT_IND_V01) {
    LOGW("Unexpected sns_client_api_v01 msg ID %u", msgId);
  } else {
    auto *obj = static_cast<SeeHelper *>(indCbData);
    obj->handleInd(clientHandle, msgId, indBuf, indBufLen);
  }
}

}  // namespace chre
