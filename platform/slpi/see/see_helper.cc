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

#include <cfloat>
#include <cinttypes>

#include "chre/platform/assert.h"
#include "chre/platform/log.h"
#include "chre/platform/slpi/power_control_util.h"
#include "chre/util/lock_guard.h"

namespace chre {
namespace {

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
  bool success = false;

  sns_std_suid suid = sns_std_suid_init_zero;
  if (!pb_decode(stream, sns_std_suid_fields, &suid)) {
    LOGE("Error decoding sns_std_suid: %s", PB_GET_ERROR(stream));
  } else {
    success = true;

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
      sns_suid_event event = {
        .data_type.funcs.decode = decodeStringField,
        .data_type.arg = &data,
        .suid.funcs.decode = decodeSnsSuidEventSuid,
      };

      DynamicVector<sns_std_suid> suids;
      if (info->syncData == nullptr) {
        // TODO: add a QMI transaction ID to identify whether this indication
        // is the one we are waiting for.
        LOGW("SNS_SUID_MSGID_SNS_SUID_EVENT received without sync data");
        event.suid.arg = &suids;
      } else {
        event.suid.arg = info->syncData;
      }

      if (!pb_decode(stream, sns_suid_event_fields, &event)) {
        LOGE("Error decoding SUID Event: %s", PB_GET_ERROR(stream));
      } else {
        success = true;
        char data_type[data.bufLen + 1];
        memcpy(data_type, data.buf, data.bufLen);
        data_type[data.bufLen] = '\0';

        LOGD("Finished sns_suid_event with data type '%s'", data_type);
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
  bool success = false;

  auto *attrVal = static_cast<SeeAttrArg *>(*arg);

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

  if (!pb_decode(stream, sns_std_attr_value_data_fields, &value)) {
    LOGE("Error decoding sns_std_attr_value_data: %s", PB_GET_ERROR(stream));
  } else {
    success = true;
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
  bool success = false;

  SeeAttrArg attrArg = {
    .initialized = false,
  };
  sns_std_attr attr = {
    .value.values.funcs.decode = decodeSnsStdAttrValue,
    .value.values.arg = &attrArg,
  };

  if (!pb_decode(stream, sns_std_attr_fields, &attr)) {
    LOGE("Error decoding sns_std_attr: %s", PB_GET_ERROR(stream));
  } else {
    success = true;

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
  bool success = false;

  auto *info = static_cast<SeeInfoArg *>(*arg);

  sns_std_attr_event event = {
    .attributes.funcs.decode = decodeSnsStrAttr,
  };

  SeeAttributes attr;
  if (info->syncData == nullptr) {
    LOGW("SNS_STD_MSGID_SNS_STD_ATTR_EVENT received without sync data");
    event.attributes.arg = &attr;
  } else {
    event.attributes.arg = info->syncData;
  }

  if (!pb_decode(stream, sns_std_attr_event_fields, &event)) {
    LOGE("Error decoding sns_std_attr_event: %s", PB_GET_ERROR(stream));
  } else {
    success = true;
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
      if (!pb_decode(stream, sns_std_error_event_fields, &event)) {
        LOGE("Error decoding sns_std_error_event: %s", PB_GET_ERROR(stream));
      } else {
        success = true;
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
  bool success = false;

  sns_client_event_msg_sns_client_event event =
    sns_client_event_msg_sns_client_event_init_zero;

  if (!pb_decode(stream, sns_client_event_msg_sns_client_event_fields,
                 &event)) {
    LOGE("Error decoding msg ID: %s", PB_GET_ERROR(stream));
  } else {
    success = true;
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
    if (info->suid.suid_high == suidLookup.suid_high
        && info->suid.suid_low == suidLookup.suid_low
        && info->msgId == SNS_SUID_MSGID_SNS_SUID_EVENT) {
      event.payload.funcs.decode = &decodeSnsSuidEvent;
    } else {
      event.payload.funcs.decode = &decodeSnsStdEvent;
    }

    if (!pb_decode(stream, sns_client_event_msg_sns_client_event_fields,
                   &event)) {
      LOGE("Error decoding sns_client_event_msg_sns_client_event: %s",
           PB_GET_ERROR(stream));
    } else {
      success = true;
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
    SeeInfoArg info = {
      .indCb = mIndCb,
      .syncData = mSyncData,
      .suid = event.suid,
    };
    event.events.funcs.decode = decodeSnsClientEventMsg;
    event.events.arg = &info;

    stream = pb_istream_from_buffer(
        static_cast<const pb_byte_t *>(payload), payloadLen);
    if (!pb_decode(&stream, sns_client_event_msg_fields, &event)) {
      LOGE("Error decoding sns_client_event_msg: %s", PB_GET_ERROR(&stream));
    } else {
      // TODO: add more checks besides SUID.
      // Try unblock only if the whole pb message is successfully decoded even
      // though only SUID is needed to determine whether this is the blocking
      // event we were waiting for.
      unblockIfPendingSuid(event.suid);
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
    mSyncData = suids;

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
          success = sendReq(sns_suid_sensor_init_default,
                            SNS_SUID_MSGID_SNS_SUID_REQ, msg, msgLen,
                            true /* waitForIndication */);
        }
      }
      memoryFree(msg);
    }
    mSyncData = nullptr;
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
    mSyncData = attr;

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
          success = sendReq(suid, SNS_STD_MSGID_SNS_STD_ATTR_REQ, msg, msgLen,
                            true /* waitForIndication */);
        }
      }
      memoryFree(msg);
    }
    mSyncData = nullptr;
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
    const sns_std_suid& suid, uint32_t msgId, void *payload, size_t payloadLen,
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

    // Specify the SUID to wait for.
    mWaitingSuid = suid;

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
  }
  return success;
}

void SeeHelper::unblockIfPendingSuid(const sns_std_suid& suid) {
  LockGuard<Mutex> lock(mMutex);

  if (mWaiting) {
    // Unblock the waiting thread if the indication message has the same SUID
    // as the one we are waiting for.
    if (suid.suid_high == mWaitingSuid.suid_high
        && suid.suid_low == mWaitingSuid.suid_low) {
      mWaiting = false;
      mCond.notify_one();
    }
  }
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
