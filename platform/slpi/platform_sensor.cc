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

#include <algorithm>
#include <cinttypes>

#include "chre/platform/platform_sensor.h"

extern "C" {

#include "fixed_point.h"
#include "qmi_client.h"
#include "sns_smgr_api_v01.h"
#include "sns_smgr_internal_api_v02.h"
#include "timetick.h"

}  // extern "C"

#include "chre_api/chre/sensor.h"
#include "chre/core/event_loop_manager.h"
#include "chre/core/sensor.h"
#include "chre/platform/assert.h"
#include "chre/platform/fatal_error.h"
#include "chre/platform/log.h"
#include "chre/platform/slpi/platform_sensor_util.h"
#include "chre/platform/slpi/smgr_client.h"
#include "chre/util/macros.h"

// TODO: [Passive] explain passive sensor design

namespace chre {
namespace {

//! The timeout for QMI messages in milliseconds.
constexpr uint32_t kQmiTimeoutMs = 1000;

//! The constant used to convert from SMGR to Android unit for magnetometer.
constexpr float kMicroTeslaPerGauss = 100.0f;

//! The maximum number of CHRE sensors that share the same SMGR sensor ID.
constexpr size_t kMaxNumSensorsPerSensorId = 3;

//! The value to override a default interval request.
constexpr uint64_t kDefaultInterval = Seconds(1).toRawNanoseconds();

//! The QMI sensor service client handle.
qmi_client_type gPlatformSensorServiceQmiClientHandle = nullptr;

//! The QMI sensor internal service client handle.
qmi_client_type gPlatformSensorInternalServiceQmiClientHandle = nullptr;

//! A sensor report indication for deserializing sensor sample indications
//! into. This global instance is used to avoid thrashy use of the heap by
//! allocating and freeing this on the heap for every new sensor sample. This
//! relies on the assumption that the QMI callback is not reentrant.
sns_smgr_buffering_ind_msg_v01 gSmgrBufferingIndMsg;

//! A struct to store the number of SMGR clients of a sensor ID.
struct SensorMonitor {
  uint8_t sensorId;
  uint8_t numClients;
};

//! A vector that tracks the SensorMonitor of each supported sensor ID.
DynamicVector<SensorMonitor> gSensorMonitors;

//! Forward declarations
bool makeAllPendingRequests(uint8_t sensorId);
bool removeAllPassiveRequests(uint8_t sensorId);

/**
 * Obtains the element index of gSensorMonitors that corresponds to the
 * specified sensor ID. If it's not present, gSensorMonitors.size() is returned.
 *
 * @return The index of the element that belongs to sensorId.
 */
size_t getSensorMonitorIndex(uint8_t sensorId) {
  size_t i;
  for (i = 0; i < gSensorMonitors.size(); i++) {
    if (gSensorMonitors[i].sensorId == sensorId) {
      break;
    }
  }
  return i;
}

/**
 * Converts a sensorId, dataType and calType as provided by SMGR to a
 * SensorType as used by platform-independent CHRE code. This is useful in
 * sensor discovery.
 *
 * @param sensorId The sensorID as provided by the SMGR request for sensor info.
 * @param dataType The dataType for the sesnor as provided by the SMGR request
 *                 for sensor info.
 * @param calType The calibration type (CAL_SEL) as defined in the SMGR API.
 * @return Returns the platform-independent sensor type or Unknown if no
 *         match is found.
 */
SensorType getSensorTypeFromSensorId(uint8_t sensorId, uint8_t dataType,
                                     uint8_t calType) {
  // Here be dragons. These constants below are defined in
  // sns_smgr_common_v01.h. Refer to the section labelled "Define sensor
  // identifier" for more details. This function relies on the ordering of
  // constants provided by their API. Do not change these values without care.
  // You have been warned!
  if (dataType == SNS_SMGR_DATA_TYPE_PRIMARY_V01) {
    if (sensorId >= SNS_SMGR_ID_ACCEL_V01
        && sensorId < SNS_SMGR_ID_GYRO_V01) {
      if (calType == SNS_SMGR_CAL_SEL_FULL_CAL_V01) {
        return SensorType::Accelerometer;
      } else if (calType == SNS_SMGR_CAL_SEL_FACTORY_CAL_V01) {
        return SensorType::UncalibratedAccelerometer;
      }
    } else if (sensorId >= SNS_SMGR_ID_GYRO_V01
        && sensorId < SNS_SMGR_ID_MAG_V01) {
      if (calType == SNS_SMGR_CAL_SEL_FULL_CAL_V01) {
        return SensorType::Gyroscope;
      } else if (calType == SNS_SMGR_CAL_SEL_FACTORY_CAL_V01) {
        return SensorType::UncalibratedGyroscope;
      }
    } else if (sensorId >= SNS_SMGR_ID_MAG_V01
        && sensorId < SNS_SMGR_ID_PRESSURE_V01) {
      if (calType == SNS_SMGR_CAL_SEL_FULL_CAL_V01) {
        return SensorType::GeomagneticField;
      } else if (calType == SNS_SMGR_CAL_SEL_FACTORY_CAL_V01) {
        return SensorType::UncalibratedGeomagneticField;
      }
    } else if (sensorId >= SNS_SMGR_ID_PRESSURE_V01
        && sensorId < SNS_SMGR_ID_PROX_LIGHT_V01) {
      return SensorType::Pressure;
    } else if (sensorId >= SNS_SMGR_ID_PROX_LIGHT_V01
        && sensorId < SNS_SMGR_ID_HUMIDITY_V01) {
      return SensorType::Proximity;
    } else if (sensorId == SNS_SMGR_ID_OEM_SENSOR_09_V01) {
      return SensorType::StationaryDetect;
    } else if (sensorId == SNS_SMGR_ID_OEM_SENSOR_10_V01) {
      return SensorType::InstantMotion;
    }
  } else if (dataType == SNS_SMGR_DATA_TYPE_SECONDARY_V01) {
    if (sensorId >= SNS_SMGR_ID_ACCEL_V01
        && sensorId < SNS_SMGR_ID_GYRO_V01) {
      return SensorType::AccelerometerTemperature;
    } else if (sensorId >= SNS_SMGR_ID_GYRO_V01
        && sensorId < SNS_SMGR_ID_MAG_V01) {
      return SensorType::GyroscopeTemperature;
    } else if ((sensorId >= SNS_SMGR_ID_PROX_LIGHT_V01
        && sensorId < SNS_SMGR_ID_HUMIDITY_V01)
        || (sensorId >= SNS_SMGR_ID_ULTRA_VIOLET_V01
            && sensorId < SNS_SMGR_ID_OBJECT_TEMP_V01)) {
      return SensorType::Light;
    }
  }

  return SensorType::Unknown;
}

/**
 * Converts a reportId as provided by SMGR to a SensorType.
 *
 * @param reportId The reportID as provided by the SMGR buffering index.
 * @return Returns the sensorType that corresponds to the reportId.
 */
SensorType getSensorTypeFromReportId(uint8_t reportId) {
  SensorType sensorType;
  if (reportId < static_cast<uint8_t>(SensorType::SENSOR_TYPE_COUNT)) {
    sensorType = static_cast<SensorType>(reportId);
  } else {
    sensorType = SensorType::Unknown;
  }
  return sensorType;
}

/**
 * Converts a PlatformSensor to a unique report ID through SensorType. This is
 * useful in making sensor request.
 *
 * @param sensorId The sensorID as provided by the SMGR request for sensor info.
 * @param dataType The dataType for the sesnor as provided by the SMGR request
 *                 for sensor info.
 * @param calType The calibration type (CAL_SEL) as defined in the SMGR API.
 * @return Returns a unique report ID that is based on SensorType.
 */
uint8_t getReportId(uint8_t sensorId, uint8_t dataType, uint8_t calType) {
  SensorType sensorType = getSensorTypeFromSensorId(
      sensorId, dataType, calType);

  CHRE_ASSERT_LOG(sensorType != SensorType::Unknown,
                  "sensorId %" PRIu8 ", dataType %" PRIu8 ", calType %" PRIu8,
                  sensorId, dataType, calType);
  return static_cast<uint8_t>(sensorType);
}

/**
 * Checks whether the corresponding sensor is a sencondary temperature sensor.
 *
 * @param reportId The reportID as provided by the SMGR buffering index.
 * @return true if the sensor is a secondary temperature sensor.
 */
bool isSecondaryTemperature(uint8_t reportId) {
  SensorType sensorType = getSensorTypeFromReportId(reportId);
  return (sensorType == SensorType::AccelerometerTemperature
          || sensorType == SensorType::GyroscopeTemperature);
}

/**
 * Verifies whether the buffering index's report ID matches the expected
 * indices length.
 *
 * @return true if it's a valid pair of indices length and report ID.
 */
bool isValidIndicesLength() {
  return ((gSmgrBufferingIndMsg.Indices_len == 1
           && !isSecondaryTemperature(gSmgrBufferingIndMsg.ReportId))
          || (gSmgrBufferingIndMsg.Indices_len == 2
              && isSecondaryTemperature(gSmgrBufferingIndMsg.ReportId)));
}

/**
 * Allocates memory and specifies the memory size for an on-change sensor to
 * store its last data event.
 *
 * @param sensorType The sensorType of this sensor.
 * @param eventSize A non-null pointer to indicate the memory size allocated.
 * @return Pointer to the memory allocated.
 */
ChreSensorData *allocateLastEvent(SensorType sensorType, size_t *eventSize) {
  CHRE_ASSERT(eventSize);

  *eventSize = 0;
  ChreSensorData *event = nullptr;
  if (sensorTypeIsOnChange(sensorType)) {
    SensorSampleType sampleType = getSensorSampleTypeFromSensorType(sensorType);
    switch (sampleType) {
      case SensorSampleType::ThreeAxis:
        *eventSize = sizeof(chreSensorThreeAxisData);
        break;
      case SensorSampleType::Float:
        *eventSize = sizeof(chreSensorFloatData);
        break;
      case SensorSampleType::Byte:
        *eventSize = sizeof(chreSensorByteData);
        break;
      case SensorSampleType::Occurrence:
        *eventSize = sizeof(chreSensorOccurrenceData);
        break;
      default:
        CHRE_ASSERT_LOG(false, "Unhandled sample type");
        break;
    }

    event = static_cast<ChreSensorData *>(memoryAlloc(*eventSize));
    if (event == nullptr) {
      *eventSize = 0;
      FATAL_ERROR("Failed to allocate last event memory for SensorType %d",
                  static_cast<int>(sensorType));
    }
  }
  return event;
}

/**
 * Constructs and initializes a sensor, and adds it to the sensor list.
 *
 * @param sensorInfo The sensorInfo as provided by the SMGR.
 * @param calType The calibration type (CAL_SEL) as defined in the SMGR API.
 * @param sensor The sensor list.
 */
void addSensor(const sns_smgr_sensor_datatype_info_s_v01& sensorInfo,
               uint8_t calType, DynamicVector<Sensor> *sensors) {
  Sensor sensor;
  sensor.sensorId = sensorInfo.SensorID;
  sensor.dataType = sensorInfo.DataType;
  sensor.calType = calType;
  size_t bytesToCopy = std::min(sizeof(sensor.sensorName) - 1,
                                static_cast<size_t>(sensorInfo.SensorName_len));
  memcpy(sensor.sensorName, sensorInfo.SensorName, bytesToCopy);
  sensor.sensorName[bytesToCopy] = '\0';
  sensor.minInterval = static_cast<uint64_t>(
      Seconds(1).toRawNanoseconds() / sensorInfo.MaxSampleRate);

  // Allocates memory for on-change sensor's last event.
  SensorType sensorType = getSensorTypeFromSensorId(
      sensorInfo.SensorID, sensorInfo.DataType, calType);
  sensor.lastEvent = allocateLastEvent(sensorType, &sensor.lastEventSize);

  if (!sensors->push_back(std::move(sensor))) {
    FATAL_ERROR("Failed to allocate new sensor: out of memory");
  }
}

/**
 * Converts SMGR ticks to nanoseconds as a uint64_t.
 *
 * @param ticks The number of ticks.
 * @return The number of nanoseconds represented by the ticks value.
 */
uint64_t getNanosecondsFromSmgrTicks(uint32_t ticks) {
  return (ticks * Seconds(1).toRawNanoseconds()) / TIMETICK_NOMINAL_FREQ_HZ;
}

void populateSensorDataHeader(
    SensorType sensorType, chreSensorDataHeader *header,
    const sns_smgr_buffering_sample_index_s_v01& sensorIndex) {
  uint64_t baseTimestamp = getNanosecondsFromSmgrTicks(
      sensorIndex.FirstSampleTimestamp);
  memset(header->reserved, 0, sizeof(header->reserved));
  header->baseTimestamp = baseTimestamp;
  header->sensorHandle = getSensorHandleFromSensorType(sensorType);
  header->readingCount = sensorIndex.SampleCount;
}

void populateThreeAxisEvent(
    SensorType sensorType, chreSensorThreeAxisData *data,
    const sns_smgr_buffering_sample_index_s_v01& sensorIndex) {
  populateSensorDataHeader(sensorType, &data->header, sensorIndex);

  for (size_t i = 0; i < sensorIndex.SampleCount; i++) {
    const sns_smgr_buffering_sample_s_v01& sensorData =
        gSmgrBufferingIndMsg.Samples[i + sensorIndex.FirstSampleIdx];

    // TimeStampOffset has max value of < 2 sec so it will not overflow here.
    data->readings[i].timestampDelta =
        getNanosecondsFromSmgrTicks(sensorData.TimeStampOffset);

    // Convert from SMGR's NED coordinate to Android coordinate.
    data->readings[i].x = FX_FIXTOFLT_Q16(sensorData.Data[1]);
    data->readings[i].y = FX_FIXTOFLT_Q16(sensorData.Data[0]);
    data->readings[i].z = -FX_FIXTOFLT_Q16(sensorData.Data[2]);

    // Convert from Gauss to micro Tesla
    if (sensorType == SensorType::GeomagneticField
        || sensorType == SensorType::UncalibratedGeomagneticField) {
      data->readings[i].x *= kMicroTeslaPerGauss;
      data->readings[i].y *= kMicroTeslaPerGauss;
      data->readings[i].z *= kMicroTeslaPerGauss;
    }
  }
}

void populateFloatEvent(
    SensorType sensorType, chreSensorFloatData *data,
    const sns_smgr_buffering_sample_index_s_v01& sensorIndex) {
  populateSensorDataHeader(sensorType, &data->header, sensorIndex);

  for (size_t i = 0; i < sensorIndex.SampleCount; i++) {
    const sns_smgr_buffering_sample_s_v01& sensorData =
        gSmgrBufferingIndMsg.Samples[i + sensorIndex.FirstSampleIdx];

    // TimeStampOffset has max value of < 2 sec so it will not overflow.
    data->readings[i].timestampDelta =
        getNanosecondsFromSmgrTicks(sensorData.TimeStampOffset);
    data->readings[i].value = FX_FIXTOFLT_Q16(sensorData.Data[0]);
  }
}

void populateByteEvent(
    SensorType sensorType, chreSensorByteData *data,
    const sns_smgr_buffering_sample_index_s_v01& sensorIndex) {
  populateSensorDataHeader(sensorType, &data->header, sensorIndex);

  for (size_t i = 0; i < sensorIndex.SampleCount; i++) {
    const sns_smgr_buffering_sample_s_v01& sensorData =
        gSmgrBufferingIndMsg.Samples[i + sensorIndex.FirstSampleIdx];

    // TimeStampOffset has max value of < 2 sec so it will not overflow.
    data->readings[i].timestampDelta =
        getNanosecondsFromSmgrTicks(sensorData.TimeStampOffset);
    // Zero out fields invalid and padding0.
    data->readings[i].value = 0;
    // SMGR reports 1 in Q16 for near, and 0 for far.
    data->readings[i].isNear = sensorData.Data[0] ? 1 : 0;
  }
}

void populateOccurrenceEvent(
    SensorType sensorType, chreSensorOccurrenceData *data,
    const sns_smgr_buffering_sample_index_s_v01& sensorIndex) {
  populateSensorDataHeader(sensorType, &data->header, sensorIndex);

  for (size_t i = 0; i < sensorIndex.SampleCount; i++) {
    const sns_smgr_buffering_sample_s_v01& sensorData =
        gSmgrBufferingIndMsg.Samples[i + sensorIndex.FirstSampleIdx];

    // TimeStampOffset has max value of < 2 sec so it will not overflow.
    data->readings[i].timestampDelta =
        getNanosecondsFromSmgrTicks(sensorData.TimeStampOffset);
  }
}

/**
 * Allocate event memory according to SensorType and populate event readings.
 */
void *allocateAndPopulateEvent(
    SensorType sensorType,
    const sns_smgr_buffering_sample_index_s_v01& sensorIndex) {
  SensorSampleType sampleType = getSensorSampleTypeFromSensorType(sensorType);
  size_t memorySize = sizeof(chreSensorDataHeader);
  switch (sampleType) {
    case SensorSampleType::ThreeAxis: {
      memorySize += sensorIndex.SampleCount *
          sizeof(chreSensorThreeAxisData::chreSensorThreeAxisSampleData);
      auto *event =
          static_cast<chreSensorThreeAxisData *>(memoryAlloc(memorySize));
      if (event != nullptr) {
        populateThreeAxisEvent(sensorType, event, sensorIndex);
      }
      return event;
    }

    case SensorSampleType::Float: {
      memorySize += sensorIndex.SampleCount *
          sizeof(chreSensorFloatData::chreSensorFloatSampleData);
      auto *event =
          static_cast<chreSensorFloatData *>(memoryAlloc(memorySize));
      if (event != nullptr) {
        populateFloatEvent(sensorType, event, sensorIndex);
      }
      return event;
    }

    case SensorSampleType::Byte: {
      memorySize += sensorIndex.SampleCount *
          sizeof(chreSensorByteData::chreSensorByteSampleData);
      auto *event =
          static_cast<chreSensorByteData *>(memoryAlloc(memorySize));
      if (event != nullptr) {
        populateByteEvent(sensorType, event, sensorIndex);
      }
      return event;
    }

    case SensorSampleType::Occurrence: {
      memorySize += sensorIndex.SampleCount *
          sizeof(chreSensorOccurrenceData::chreSensorOccurrenceSampleData);
      auto *event =
          static_cast<chreSensorOccurrenceData *>(memoryAlloc(memorySize));
      if (event != nullptr) {
        populateOccurrenceEvent(sensorType, event, sensorIndex);
      }
      return event;
    }

    default:
      LOGW("Unhandled sensor data %" PRIu8, sensorType);
      return nullptr;
  }
}

void smgrSensorDataEventFree(uint16_t eventType, void *eventData) {
  // Events are allocated using the simple memoryAlloc/memoryFree platform
  // functions.
  // TODO: Consider using a MemoryPool.
  memoryFree(eventData);

  // Remove all requests if it's a one-shot sensor and only after data has been
  // delivered to all clients.
  SensorType sensorType = getSensorTypeForSampleEventType(eventType);
  if (sensorTypeIsOneShot(sensorType)) {
    EventLoopManagerSingleton::get()->getSensorRequestManager()
        .removeAllRequests(sensorType);
  }
}

/**
 * A helper function that updates the last event of a sensor in the main thread.
 * Platform should call this function only for an on-change sensor.
 *
 * @param sensorType The SensorType of the sensor.
 * @param eventData A non-null pointer to the sensor's CHRE event data.
 */
void updateLastEvent(SensorType sensorType, const void *eventData) {
  CHRE_ASSERT(eventData);

  auto *header = static_cast<const chreSensorDataHeader *>(eventData);
  if (header->readingCount != 1) {
    // TODO: better error handling when SMGR behavior changes.
    // SMGR delivers one sample per report for on-change sensors.
    LOGE("%" PRIu16 " samples in an event for on-change sensor %d",
         header->readingCount, static_cast<int>(sensorType));
  } else {
    struct CallbackData {
      SensorType sensorType;
      const ChreSensorData *event;
    };
    auto *callbackData = memoryAlloc<CallbackData>();
    if (callbackData == nullptr) {
      LOGE("Failed to allocate deferred callback memory");
    } else {
      callbackData->sensorType = sensorType;
      callbackData->event = static_cast<const ChreSensorData *>(eventData);

      auto callback = [](uint16_t /* type */, void *data) {
        auto *cbData = static_cast<CallbackData *>(data);

        Sensor *sensor = EventLoopManagerSingleton::get()
            ->getSensorRequestManager().getSensor(cbData->sensorType);

        // Mark last event as valid only if the sensor is enabled. Event data
        // may arrive after sensor is disabled.
        if (sensor != nullptr
            && sensor->getRequest().getMode() != SensorMode::Off) {
          sensor->setLastEvent(cbData->event);
        }
        memoryFree(cbData);
      };

      // Schedule a deferred callback.
      if (!EventLoopManagerSingleton::get()->deferCallback(
          SystemCallbackType::SensorLastEventUpdate, callbackData, callback)) {
        LOGE("Failed to schedule a deferred callback for sensorType %d",
             static_cast<int>(sensorType));
        memoryFree(callbackData);
      }
    }  // if (callbackData == nullptr)
  }
}

/**
 * Handles sensor data provided by the SMGR framework.
 *
 * @param userHandle The userHandle is used by the QMI decode function.
 * @param buffer The buffer to decode sensor data from.
 * @param bufferLength The size of the buffer to decode.
 */
void handleSensorDataIndication(void *userHandle, void *buffer,
                                unsigned int bufferLength) {
  int status = qmi_client_message_decode(
      userHandle, QMI_IDL_INDICATION, SNS_SMGR_BUFFERING_IND_V01, buffer,
      bufferLength, &gSmgrBufferingIndMsg,
      sizeof(sns_smgr_buffering_ind_msg_v01));
  if (status != QMI_NO_ERR) {
    LOGE("Error parsing sensor data indication %d", status);
  } else {
    // We only requested one sensor per request except for a secondary
    // temperature sensor.
    bool validReport = isValidIndicesLength();
    CHRE_ASSERT_LOG(validReport,
                    "Got buffering indication from %" PRIu32
                    " sensors with report ID %" PRIu8,
                    gSmgrBufferingIndMsg.Indices_len,
                    gSmgrBufferingIndMsg.ReportId);
    if (validReport) {
      // Identify the index for the desired sensor. It is always 0 except
      // possibly for a secondary temperature sensor.
      uint32_t index = 0;
      if (isSecondaryTemperature(gSmgrBufferingIndMsg.ReportId)) {
        index = (gSmgrBufferingIndMsg.Indices[0].DataType
                 == SNS_SMGR_DATA_TYPE_SECONDARY_V01) ? 0 : 1;
      }
      const sns_smgr_buffering_sample_index_s_v01& sensorIndex =
          gSmgrBufferingIndMsg.Indices[index];

      // Use ReportID to identify sensors as
      // gSmgrBufferingIndMsg.Samples[i].Flags are not populated.
      SensorType sensorType = getSensorTypeFromReportId(
          gSmgrBufferingIndMsg.ReportId);
      if (sensorType == SensorType::Unknown) {
        LOGW("Received sensor sample for unknown sensor %" PRIu8 " %" PRIu8,
             sensorIndex.SensorId, sensorIndex.DataType);
      } else if (sensorIndex.SampleCount == 0) {
        LOGW("Received sensorType %d event with 0 sample",
             static_cast<int>(sensorType));
      } else {
        void *eventData = allocateAndPopulateEvent(sensorType, sensorIndex);
        if (eventData == nullptr) {
          LOGW("Dropping event due to allocation failure");
        } else {
          // Schedule a deferred callback to update on-change sensor's last
          // event in the main thread.
          if (sensorTypeIsOnChange(sensorType)) {
            updateLastEvent(sensorType, eventData);
          }

          EventLoopManagerSingleton::get()->postEvent(
              getSampleEventTypeForSensorType(sensorType), eventData,
              smgrSensorDataEventFree);
        }
      }
    }  // if (validReport)
  }
}

/**
 * This callback is invoked by the QMI framework when an asynchronous message is
 * delivered. Unhandled messages are logged. The signature is defined by the QMI
 * library.
 *
 * @param userHandle The userHandle is used by the QMI library.
 * @param messageId The type of the message to decode.
 * @param buffer The buffer to decode.
 * @param bufferLength The length of the buffer to decode.
 * @param callbackData Data that is provided as a context to this callback. This
 *                     is not used in this context.
 */
void platformSensorServiceQmiIndicationCallback(void *userHandle,
                                                unsigned int messageId,
                                                void *buffer,
                                                unsigned int bufferLength,
                                                void *callbackData) {
  switch (messageId) {
    case SNS_SMGR_BUFFERING_IND_V01:
      handleSensorDataIndication(userHandle, buffer, bufferLength);
      break;
    default:
      LOGW("Received unhandled sensor service message: 0x%x", messageId);
      break;
  };
}

/**
 * Populates the supplied SensorTypes array with SensorTypes of the specified
 * sensor ID.
 *
 * @param sensorId The sensor ID as provided by the SMGR.
 * @param sensorTypes A non-null pointer to a SensorType array of size at least
 *        kMaxNumSensorsPerSensorId.
 */
size_t populateSensorTypeArrayFromSensorId(uint8_t sensorId,
                                           SensorType *sensorTypes) {
  static_assert(kMaxNumSensorsPerSensorId >= 3,
                "This function assumes kMaxNumSensorsPerSensorId >= 3");
  CHRE_ASSERT(sensorTypes);

  size_t numSensorTypes = 0;
  if (sensorTypes != nullptr) {
    if (sensorId >= SNS_SMGR_ID_ACCEL_V01
          && sensorId < SNS_SMGR_ID_GYRO_V01) {
      sensorTypes[0] = SensorType::Accelerometer;
      sensorTypes[1] = SensorType::UncalibratedAccelerometer;
      sensorTypes[2] = SensorType::AccelerometerTemperature;
      numSensorTypes = 3;
    } else if (sensorId >= SNS_SMGR_ID_GYRO_V01
          && sensorId < SNS_SMGR_ID_MAG_V01) {
      sensorTypes[0] = SensorType::Gyroscope;
      sensorTypes[1] = SensorType::UncalibratedGyroscope;
      sensorTypes[2] = SensorType::GyroscopeTemperature;
      numSensorTypes = 3;
    } else if (sensorId >= SNS_SMGR_ID_MAG_V01
          && sensorId < SNS_SMGR_ID_PRESSURE_V01) {
      sensorTypes[0] = SensorType::GeomagneticField;
      sensorTypes[1] = SensorType::UncalibratedGeomagneticField;
      numSensorTypes = 2;
    } else {
      SensorType sensorType = getSensorTypeFromSensorId(sensorId,
          SNS_SMGR_DATA_TYPE_PRIMARY_V01, SNS_SMGR_CAL_SEL_FULL_CAL_V01);
      if (sensorType != SensorType::Unknown) {
        sensorTypes[0] = sensorType;
        numSensorTypes = 1;
      }
    }
  }
  return numSensorTypes;
}

/**
 * Obtains the number of SMGR clients of a sensor ID that were originated by
 * CHRE.
 *
 * @param sensorId The sensor ID as provided by the SMGR.
 * @return The number of CHRE clients.
 */
size_t getNumChreClients(uint8_t sensorId) {
  size_t numChreClients = 0;

  // Identify sensor types of the specified sensor ID
  SensorType sensorTypes[kMaxNumSensorsPerSensorId];
  size_t numSensorTypes = populateSensorTypeArrayFromSensorId(
      sensorId, sensorTypes);
  for (size_t i = 0; i < numSensorTypes; i++) {
    const Sensor *sensor = EventLoopManagerSingleton::get()
        ->getSensorRequestManager().getSensor(sensorTypes[i]);
    if (sensor != nullptr && !sensor->isSensorOff) {
      numChreClients++;
    }
  }
  return numChreClients;
}

/**
 * Obtains the merged SensorMode of the specified sensor ID, with sensorType's
 * sensor request replaced by the supplied request.
 *
 * @param sensorId The sensor ID as provided by the SMGR.
 * @param sensorType The SensorType whose sensor request is to be replaced by
 *        the supplied request.
 * @param request The sensor request to replace the existing one.
 * @return The merged SensorMode.
 */
SensorMode getMergedMode(uint8_t sensorId, SensorType sensorType,
                         const SensorRequest& request) {
  // Identify sensor requests to merge
  SensorType sensorTypes[kMaxNumSensorsPerSensorId];
  size_t numSensorTypes = populateSensorTypeArrayFromSensorId(
      sensorId, sensorTypes);

  // merge requests
  SensorRequest mergedRequest;
  for (size_t i = 0; i < numSensorTypes; i++) {
    const Sensor *sensor = EventLoopManagerSingleton::get()
      ->getSensorRequestManager().getSensor(sensorTypes[i]);
    if (sensor != nullptr) {
      mergedRequest.mergeWith(
          (sensorTypes[i] == sensorType) ? request : sensor->getRequest());
    }
  }
  return mergedRequest.getMode();
}

/**
 * Makes or removes passive sensor requests when the number of SMGR clients
 * changes.
 *
 * @param sensorID The sensor ID being monitored.
 * @param prevNumClients The previous number of SMGR clients.
 * @param currNumclients The current number of SMGR clients.
 */
void onNumSmgrClientsChange(uint8_t sensorId, uint8_t prevNumClients,
                            uint8_t currNumClients) {
  bool makeAllRequests = (prevNumClients == 0 && currNumClients > 0);
  SensorRequest dummyRequest;
  SensorMode mode = getMergedMode(sensorId, SensorType::Unknown, dummyRequest);
  bool removeAllRequests = (sensorModeIsPassive(mode)
                            && currNumClients < prevNumClients
                            && currNumClients == getNumChreClients(sensorId)
                            && currNumClients > 0);
  bool qmiRequestMade = false;
  if (makeAllRequests) {
    qmiRequestMade = makeAllPendingRequests(sensorId);
  } else if (removeAllRequests) {
    qmiRequestMade = removeAllPassiveRequests(sensorId);
  }

  if (qmiRequestMade) {
    LOGD("%s: id %" PRIu8 ", prev %" PRIu8 " curr %" PRIu8 ", mode %d, chre %d",
         makeAllRequests ? "+" : "-", sensorId, prevNumClients, currNumClients,
         static_cast<size_t>(mode), getNumChreClients(sensorId));
  }
}

/**
 * Processes sensor status monitor indication message.
 *
 * @param status The new sensor status indication from SMGR
 */
void onStatusChange(const sns_smgr_sensor_status_monitor_ind_msg_v02& status) {
  size_t index = getSensorMonitorIndex(status.sensor_id);
  if (index == gSensorMonitors.size()) {
    LOGE("Sensor status monitor update of invalid sensor ID %" PRIu64,
         status.sensor_id);
  } else {
    uint8_t numClients = gSensorMonitors[index].numClients;
    if (numClients != status.num_clients) {
      onNumSmgrClientsChange(status.sensor_id, numClients, status.num_clients);
      gSensorMonitors[index].numClients = status.num_clients;
    }
  }
}

/**
 * Handles sensor status provided by the SMGR framework.
 *
 * @param userHandle The userHandle is used by the QMI decode function.
 * @param buffer The buffer to decode sensor data from.
 * @param bufferLength The size of the buffer to decode.
 */
void handleSensorStatusMonitorIndication(void *userHandle, void *buffer,
                                         unsigned int bufferLength) {
  sns_smgr_sensor_status_monitor_ind_msg_v02 smgrMonitorIndMsg;

  int status = qmi_client_message_decode(
      userHandle, QMI_IDL_INDICATION, SNS_SMGR_SENSOR_STATUS_MONITOR_IND_V02,
      buffer, bufferLength, &smgrMonitorIndMsg, sizeof(smgrMonitorIndMsg));
  if (status != QMI_NO_ERR) {
    LOGE("Error parsing sensor status monitor indication %d", status);
  } else {
    auto *callbackData = memoryAlloc<
      sns_smgr_sensor_status_monitor_ind_msg_v02>();
    if (callbackData == nullptr) {
      LOGE("Failed to allocate status update deferred callback memory");
    } else {
      *callbackData = smgrMonitorIndMsg;
      auto callback = [](uint16_t /* type */, void *data) {
        auto *cbData =
            static_cast<sns_smgr_sensor_status_monitor_ind_msg_v02 *>(data);
        onStatusChange(*cbData);
        memoryFree(cbData);
      };

      // Schedule a deferred callback to handle sensor status change in the main
      // thread.
      if (!EventLoopManagerSingleton::get()->deferCallback(
          SystemCallbackType::SensorStatusUpdate, callbackData, callback)) {
        LOGE("Failed to schedule a deferred callback for status update");
        memoryFree(callbackData);
      }
    }
  }
}

/**
 * This callback is invoked by the QMI framework when an asynchronous message is
 * delivered. Unhandled messages are logged. The signature is defined by the QMI
 * library.
 *
 * @param userHandle The userHandle is used by the QMI library.
 * @param messageId The type of the message to decode.
 * @param buffer The buffer to decode.
 * @param bufferLength The length of the buffer to decode.
 * @param callbackData Data that is provided as a context to this callback. This
 *                     is not used in this context.
 */
void platformSensorInternalServiceQmiIndicationCallback(void *userHandle,
    unsigned int messageId, void *buffer, unsigned int bufferLength,
    void *callbackData) {
  switch (messageId) {
    case SNS_SMGR_SENSOR_STATUS_MONITOR_IND_V02:
      handleSensorStatusMonitorIndication(userHandle, buffer, bufferLength);
      break;
    default:
      LOGW("Received unhandled sensor internal service message: 0x%x",
           messageId);
      break;
  };
}

/**
 * Adds or removes an SMGR sensor monitor for the specified sensor ID.
 *
 * @param sensorId The sensor ID to add/remove sensor status monitor for.
 * @param enable true to add and false to remove the status monitor.
 */
void setSensorMonitorRequest(uint8_t sensorId, bool enable) {
  sns_smgr_sensor_status_monitor_req_msg_v02 monitorRequest;
  sns_smgr_sensor_status_monitor_resp_msg_v02 monitorResponse;
  monitorRequest.sensor_id = sensorId;
  monitorRequest.registering = enable ? TRUE : FALSE;

  qmi_client_error_type status = qmi_client_send_msg_sync(
      gPlatformSensorInternalServiceQmiClientHandle,
      SNS_SMGR_SENSOR_STATUS_MONITOR_REQ_V02,
      &monitorRequest, sizeof(monitorRequest),
      &monitorResponse, sizeof(monitorResponse), kQmiTimeoutMs);

  if (status != QMI_NO_ERR) {
    LOGE("Error setting sensor status monitor: %d", status);
  } else if (monitorResponse.resp.sns_result_t != SNS_RESULT_SUCCESS_V01) {
    LOGE("Sensor status monitor request failed with error: %" PRIu8
         " sensor ID %" PRIu8 " enable %d",
         monitorResponse.resp.sns_err_t, sensorId, enable);
  }
}

/**
 * Adds and initializes a sensor monitor for the specified sensor ID if it
 * doesn't exist yet.
 *
 * @param sensorId The sensor ID to request monitor for.
 */
void addSensorMonitor(uint8_t sensorId) {
  size_t index = getSensorMonitorIndex(sensorId);
  if (index == gSensorMonitors.size()) {
    LOGD("Adding sensor status monitor for sensor ID %" PRIu8, sensorId);

    // Initialize sensor monitor status before making a QMI request.
    SensorMonitor monitor;
    monitor.sensorId = sensorId;
    monitor.numClients = 0;
    gSensorMonitors.push_back(monitor);

    // Make a QMI request to add the status monitor
    setSensorMonitorRequest(sensorId, true);
  }
}

/**
 * Requests the sensors for a given sensor ID and appends them to the provided
 * list of sensors. If an error occurs, false is returned.
 *
 * @param sensorId The sensor ID to request sensor info for.
 * @param sensors The list of sensors to append newly found sensors to.
 * @return Returns false if an error occurs.
 */
bool getSensorsForSensorId(uint8_t sensorId,
                           DynamicVector<Sensor> *sensors) {
  sns_smgr_single_sensor_info_req_msg_v01 sensorInfoRequest;
  sns_smgr_single_sensor_info_resp_msg_v01 sensorInfoResponse;

  sensorInfoRequest.SensorID = sensorId;

  qmi_client_error_type status = qmi_client_send_msg_sync(
      gPlatformSensorServiceQmiClientHandle, SNS_SMGR_SINGLE_SENSOR_INFO_REQ_V01,
      &sensorInfoRequest, sizeof(sns_smgr_single_sensor_info_req_msg_v01),
      &sensorInfoResponse, sizeof(sns_smgr_single_sensor_info_resp_msg_v01),
      kQmiTimeoutMs);

  bool success = false;
  if (status != QMI_NO_ERR) {
    LOGE("Error requesting single sensor info: %d", status);
  } else if (sensorInfoResponse.Resp.sns_result_t != SNS_RESULT_SUCCESS_V01) {
    LOGE("Single sensor info request failed with error: %d",
         sensorInfoResponse.Resp.sns_err_t);
  } else {
    const sns_smgr_sensor_info_s_v01& sensorInfoList =
        sensorInfoResponse.SensorInfo;
    for (uint32_t i = 0; i < sensorInfoList.data_type_info_len; i++) {
      const sns_smgr_sensor_datatype_info_s_v01& sensorInfo =
          sensorInfoList.data_type_info[i];
      LOGD("SensorID %" PRIu8 ", DataType %" PRIu8 ", MaxRate %" PRIu16
           "Hz, SensorName %s",
           sensorInfo.SensorID, sensorInfo.DataType,
           sensorInfo.MaxSampleRate, sensorInfo.SensorName);

      SensorType sensorType = getSensorTypeFromSensorId(
          sensorInfo.SensorID, sensorInfo.DataType,
          SNS_SMGR_CAL_SEL_FULL_CAL_V01);
      if (sensorType != SensorType::Unknown) {
        addSensor(sensorInfo, SNS_SMGR_CAL_SEL_FULL_CAL_V01, sensors);

        // Add an uncalibrated version if defined.
        SensorType uncalibratedType = getSensorTypeFromSensorId(
            sensorInfo.SensorID, sensorInfo.DataType,
            SNS_SMGR_CAL_SEL_FACTORY_CAL_V01);
        if (sensorType != uncalibratedType) {
          addSensor(sensorInfo, SNS_SMGR_CAL_SEL_FACTORY_CAL_V01, sensors);
        }
      }
    }
    success = true;
  }
  return success;
}

/**
 * Converts a SensorMode into an SMGR request action. When the net request for
 * a sensor is considered to be active an add operation is required for the
 * SMGR request. When the sensor becomes inactive the request is deleted.
 *
 * @param mode The sensor mode.
 * @return Returns the SMGR request action given the sensor mode.
 */
uint8_t getSmgrRequestActionForMode(SensorMode mode) {
  if (mode != SensorMode::Off) {
    return SNS_SMGR_BUFFERING_ACTION_ADD_V01;
  } else {
    return SNS_SMGR_BUFFERING_ACTION_DELETE_V01;
  }
}

/**
 * Specify the sensor decimation type.
 *
 * @param sensorId The sensorID as provided by the SMGR.
 * @param dataType The dataType for the sesnor as provided by the SMGR.
 * return The decimation type as defined by the SMGR.
 */
uint8_t getDecimationType(uint8_t sensorId, uint8_t dataType) {
  // Request filtered data for accel and gyro to reduce noise aliasing in case
  // SMGR has other higher ODR clients.
  if ((sensorId == SNS_SMGR_ID_ACCEL_V01 || sensorId == SNS_SMGR_ID_GYRO_V01)
      && dataType == SNS_SMGR_DATA_TYPE_PRIMARY_V01) {
    return SNS_SMGR_DECIMATION_FILTER_V01;
  } else {
    return SNS_SMGR_DECIMATION_RECENT_SAMPLE_V01;
  }
}

/**
 * Populates a sns_smgr_buffering_req_msg_v01 struct to request sensor data.
 *
 * @param request The new request to set this sensor to.
 * @param sensorId The sensorID as provided by the SMGR request for sensor info.
 * @param dataType The dataType for the sesnor as provided by the SMGR request
 *                 for sensor info.
 * @param calType The calibration type (CAL_SEL) as defined in the SMGR API.
 * @param minInterval The minimum interval allowed by this sensor.
 * @param sensorDataRequest The pointer to the data request to be populated.
 */
void populateSensorRequest(
    const SensorRequest& chreRequest, uint8_t sensorId, uint8_t dataType,
    uint8_t calType, uint64_t minInterval,
    sns_smgr_buffering_req_msg_v01 *sensorRequest) {
  // Zero the fields in the request. All mandatory and unused fields are
  // specified to be set to false or zero so this is safe.
  memset(sensorRequest, 0, sizeof(*sensorRequest));

  // Reconstructs a request to deliver one-shot sensors' data ASAP and set
  // default interval to some meaningful number.
  bool isOneShot = sensorTypeIsOneShot(getSensorTypeFromSensorId(
      sensorId, dataType, calType));
  uint64_t cappedInterval = chreRequest.getInterval().toRawNanoseconds();
  if (cappedInterval == CHRE_SENSOR_INTERVAL_DEFAULT) {
    cappedInterval = std::max(minInterval, kDefaultInterval);
  }
  SensorRequest request(chreRequest.getMode(), Nanoseconds(cappedInterval),
                        isOneShot ? Nanoseconds(0) : chreRequest.getLatency());

  // Build the request for one sensor at the requested rate. An add action for a
  // ReportID that is already in use causes a replacement of the last request.
  sensorRequest->ReportId = getReportId(sensorId, dataType, calType);
  sensorRequest->Action = getSmgrRequestActionForMode(request.getMode());

  // SMGR report interval should be (interval + latency). However, to handle
  // fractional-interval latency setting and to guarantee meeting chre request,
  // report interval is set to latency only. Also, lower-bound batchInterval as
  // request to SMGR would fail if batchInterval < interval.
  Nanoseconds batchInterval =
      std::max(request.getLatency(), request.getInterval());
  sensorRequest->ReportRate = intervalToSmgrQ16ReportRate(batchInterval);
  sensorRequest->Item_len = 1;  // One sensor per request if possible.
  sensorRequest->Item[0].SensorId = sensorId;
  sensorRequest->Item[0].DataType = dataType;
  sensorRequest->Item[0].Decimation = getDecimationType(sensorId, dataType);
  sensorRequest->Item[0].Calibration = calType;
  sensorRequest->Item[0].SamplingRate =
      intervalToSmgrSamplingRate(request.getInterval());

  // Add a dummy primary sensor to accompany a secondary temperature sensor.
  // This is requred by the SMGR. The primary sensor is requested with the same
  // (low) rate and the same latency, whose response data will be ignored.
  if (isSecondaryTemperature(sensorRequest->ReportId)) {
    sensorRequest->Item_len = 2;
    sensorRequest->Item[1].SensorId = sensorId;
    sensorRequest->Item[1].DataType = SNS_SMGR_DATA_TYPE_PRIMARY_V01;
    sensorRequest->Item[1].Decimation = getDecimationType(
        sensorId, SNS_SMGR_DATA_TYPE_PRIMARY_V01);
    sensorRequest->Item[1].Calibration = SNS_SMGR_CAL_SEL_FULL_CAL_V01;
    sensorRequest->Item[1].SamplingRate = sensorRequest->Item[0].SamplingRate;
  }

  // Synchronize fifo flushes with other clients that have SSC proc_type.
  // send_indications_during_suspend has no effect on data sent to SLPI.
  // Default is to synchronize with AP clients, which may have undesirable
  // effects on sensor hal batching.
  sensorRequest->notify_suspend_valid = true;
  sensorRequest->notify_suspend.proc_type = SNS_PROC_SSC_V01;
  sensorRequest->notify_suspend.send_indications_during_suspend = true;
}

/**
 * Obtains the number of SMGR clients that were not originated by CHRE.
 *
 * @param sensorId The sensorID as provided by the SMGR.
 * @return The number of non-CHRE clients.
 */
size_t getNumNonChreClients(uint8_t sensorId) {
  size_t numChreClients = getNumChreClients(sensorId);
  size_t numSmgrClients = 0;
  size_t index = getSensorMonitorIndex(sensorId);
  if (index == gSensorMonitors.size()) {
    LOGE("Accessing sensor monitor with invalid sensorId %" PRIu8, sensorId);
  } else {
    numSmgrClients = gSensorMonitors[index].numClients;
  }

  size_t numNonChreClients = 0;
  if (numChreClients > numSmgrClients) {
    // SMGR status monitor indication may lag behind if back-to-back requests
    // are made.
    LOGW("numChreClients %zu > numSmgrClients %zu",
         numChreClients, numSmgrClients);
  } else {
    numNonChreClients = numSmgrClients - numChreClients;
  }
  return numNonChreClients;
}

/**
 * Determines whether a requet is allowed. A passive request is not always
 * allowed.
 *
 * @param sensorType The SensorType of this request
 * @param request The intended sensor request
 * @return true if the request is allowed.
 */
bool isRequestAllowed(SensorType sensorType, const SensorRequest& request) {
  bool allowed = false;

  const Sensor *sensor = EventLoopManagerSingleton::get()
      ->getSensorRequestManager().getSensor(sensorType);
  if (sensor != nullptr) {
    // If it's an ACTIVE or an OFF request, it's always allowed.
    allowed = true;
    if (sensorModeIsPassive(request.getMode())) {
      size_t numNonChreClients = getNumNonChreClients(sensor->sensorId);
      SensorMode mode = getMergedMode(sensor->sensorId, sensorType, request);
      allowed = (numNonChreClients > 0 || sensorModeIsActive(mode));
      LOGD("sensorType %d allowed %d: mergedMode %d, numNonChreClients %zu",
           static_cast<size_t>(sensorType), allowed, static_cast<int>(mode),
           numNonChreClients);
    }
  }
  return allowed;
}

/**
 * Makes a QMI SNS_SMGR_BUFFERING_REQ request based on the arguments provided.
 *
 * @param sensorId The sensorID as provided by the SMGR.
 * @param dataType The dataType for the sesnor as provided by the MSGR.
 * @param calType The calibration type (CAL_SEL) as defined in the SMGR API.
 * @param minInterval The minimum interval of this sensor.
 * @param request The sensor request
 * @return true if the request has been made successfully.
 */
bool makeQmiRequest(uint8_t sensorId, uint8_t dataType, uint8_t calType,
                    uint64_t minInterval, const SensorRequest& request) {
  bool success = false;

  // Allocate request and response for the sensor request.
  auto *sensorRequest = memoryAlloc<sns_smgr_buffering_req_msg_v01>();
  auto *sensorResponse = memoryAlloc<sns_smgr_buffering_resp_msg_v01>();

  if (sensorRequest == nullptr || sensorResponse == nullptr) {
    LOGE("Failed to allocate sensor request/response: out of memory");
  } else {
    populateSensorRequest(request, sensorId, dataType, calType,
                          minInterval, sensorRequest);

    qmi_client_error_type status = qmi_client_send_msg_sync(
        gPlatformSensorServiceQmiClientHandle, SNS_SMGR_BUFFERING_REQ_V01,
        sensorRequest, sizeof(*sensorRequest),
        sensorResponse, sizeof(*sensorResponse),
        kQmiTimeoutMs);

    if (status != QMI_NO_ERR) {
      LOGE("Error requesting sensor data: %d", status);
    } else if (sensorResponse->Resp.sns_result_t != SNS_RESULT_SUCCESS_V01
        || (sensorResponse->AckNak != SNS_SMGR_RESPONSE_ACK_SUCCESS_V01
            && sensorResponse->AckNak != SNS_SMGR_RESPONSE_ACK_MODIFIED_V01)) {
      LOGE("Sensor data request failed with error: %d, AckNak: %d",
           sensorResponse->Resp.sns_err_t, sensorResponse->AckNak);
    } else {
      success = true;
    }
  }
  memoryFree(sensorRequest);
  memoryFree(sensorResponse);
  return success;
}

/**
 * Makes a QMI SNS_SMGR_BUFFERING_REQ request if necessary.
 *
 * @param sensorType The sensor type of the request.
 * @param request The sensor request to be made.
 * @return true if the request has been accepted.
 */
bool makeRequest(SensorType sensorType, const SensorRequest& request) {
  bool success = false;

  Sensor *sensor = EventLoopManagerSingleton::get()->getSensorRequestManager()
      .getSensor(sensorType);
  if (sensor == nullptr) {
    LOGE("Invalid sensorType %d", static_cast<size_t>(sensorType));
  } else {
    // Do not make a QMI off request if the sensor is off. Otherwise, SMGR
    // returns an error.
    if (request.getMode() == SensorMode::Off) {
      success = sensor->isSensorOff;
    }

    // Make a QMI buffering request if necessary.
    if (!success) {
      success = makeQmiRequest(sensor->sensorId, sensor->dataType,
                               sensor->calType, sensor->minInterval, request);
    }
  }

  // TODO: handle makeQmiRequest failures
  if (success) {
    // Update internal states if request was accepted by SMGR.
    sensor->isSensorOff = (request.getMode() == SensorMode::Off);

    if (request.getMode() == SensorMode::Off) {
      sensor->lastEventValid = false;
    }
  }
  return success;
}

/**
 * Makes all pending requests of the specified sensor ID to SMGR.
 *
 * @param sensorId The sensor ID whose pending requests are to be made.
 * @return true if an ADD request has been accepted.
 */
bool makeAllPendingRequests(uint8_t sensorId) {
  // Identify sensor types to check for pending requests
  SensorType sensorTypes[kMaxNumSensorsPerSensorId];
  size_t numSensorTypes = populateSensorTypeArrayFromSensorId(
      sensorId, sensorTypes);
  bool accepted = false;
  for (size_t i = 0; i < numSensorTypes; i++) {
    const Sensor *sensor = EventLoopManagerSingleton::get()
        ->getSensorRequestManager().getSensor(sensorTypes[i]);

    // If sensor is off and the request is not off, it's a pending request.
    if (sensor != nullptr && sensor->isSensorOff
        && sensor->getRequest().getMode() != SensorMode::Off) {
      accepted |= makeRequest(sensorTypes[i], sensor->getRequest());
    }
  }
  return accepted;
}

/**
 * Identifies and removes passive requests that have been made to the SMGR, and
 * adds them to the sensor monitor.
 *
 * @param sensorId The sensor ID whose passive QMI requests are to be removed.
 * @return true if a DELETE request has been accepted.
 */
bool removeAllPassiveRequests(uint8_t sensorId) {
  // Specify sensor types to check for passive requests
  SensorType sensorTypes[kMaxNumSensorsPerSensorId];
  size_t numSensorTypes = populateSensorTypeArrayFromSensorId(
      sensorId, sensorTypes);
  bool accepted = false;
  for (size_t i = 0; i < numSensorTypes; i++) {
    const Sensor *sensor = EventLoopManagerSingleton::get()
        ->getSensorRequestManager().getSensor(sensorTypes[i]);

    // Turn off sensors that have a passive request
    if (sensor != nullptr
        && sensorModeIsPassive(sensor->getRequest().getMode())) {
      SensorRequest offRequest;
      accepted |= makeRequest(sensorTypes[i], offRequest);
    }
  }
  return accepted;
}

}  // anonymous namespace

PlatformSensor::~PlatformSensor() {
  if (lastEvent != nullptr) {
    LOGD("Releasing lastEvent: 0x%p, id %" PRIu8 ", type %" PRIu8 ", cal %"
         PRIu8 ", size %zu",
         lastEvent, sensorId, dataType, calType, lastEventSize);
    memoryFree(lastEvent);
  }
}

void PlatformSensor::init() {
  // sns_smgr_api_v01
  qmi_idl_service_object_type sensorServiceObject =
      SNS_SMGR_SVC_get_service_object_v01();
  if (sensorServiceObject == nullptr) {
    FATAL_ERROR("Failed to obtain the SNS SMGR service instance");
  }

  qmi_client_os_params sensorContextOsParams;
  qmi_client_error_type status = qmi_client_init_instance(sensorServiceObject,
      QMI_CLIENT_INSTANCE_ANY, &platformSensorServiceQmiIndicationCallback,
      nullptr, &sensorContextOsParams, kQmiTimeoutMs,
      &gPlatformSensorServiceQmiClientHandle);
  if (status != QMI_NO_ERR) {
    FATAL_ERROR("Failed to initialize the sensor service QMI client: %d",
                status);
  }

  // sns_smgr_interal_api_v02
  sensorServiceObject = SNS_SMGR_INTERNAL_SVC_get_service_object_v02();
  if (sensorServiceObject == nullptr) {
    FATAL_ERROR("Failed to obtain the SNS SMGR internal service instance");
  }

  status = qmi_client_init_instance(sensorServiceObject,
      QMI_CLIENT_INSTANCE_ANY,
      &platformSensorInternalServiceQmiIndicationCallback, nullptr,
      &sensorContextOsParams, kQmiTimeoutMs,
      &gPlatformSensorInternalServiceQmiClientHandle);
  if (status != QMI_NO_ERR) {
    FATAL_ERROR("Failed to initialize the sensor internal service QMI client: "
                "%d", status);
  }
}

void PlatformSensor::deinit() {
  qmi_client_error_type err = qmi_client_release(
      gPlatformSensorServiceQmiClientHandle);
  if (err != QMI_NO_ERR) {
    LOGE("Failed to release SensorService QMI client: %d", err);
  }
  gPlatformSensorServiceQmiClientHandle = nullptr;

  err = qmi_client_release(gPlatformSensorInternalServiceQmiClientHandle);
  if (err != QMI_NO_ERR) {
    LOGE("Failed to release SensorInternalService QMI client: %d", err);
  }
  gPlatformSensorInternalServiceQmiClientHandle = nullptr;

  // Clearing all sensor status monitors. Releaseing a QMI client also
  // releases all sensor status monitor requests.
  gSensorMonitors.clear();
}

bool PlatformSensor::getSensors(DynamicVector<Sensor> *sensors) {
  CHRE_ASSERT(sensors);

  sns_smgr_all_sensor_info_req_msg_v01 sensorListRequest;
  sns_smgr_all_sensor_info_resp_msg_v01 sensorListResponse;

  qmi_client_error_type status = qmi_client_send_msg_sync(
      gPlatformSensorServiceQmiClientHandle, SNS_SMGR_ALL_SENSOR_INFO_REQ_V01,
      &sensorListRequest, sizeof(sns_smgr_all_sensor_info_req_msg_v01),
      &sensorListResponse, sizeof(sns_smgr_all_sensor_info_resp_msg_v01),
      kQmiTimeoutMs);

  bool success = false;
  if (status != QMI_NO_ERR) {
    LOGE("Error requesting sensor list: %d", status);
  } else if (sensorListResponse.Resp.sns_result_t != SNS_RESULT_SUCCESS_V01) {
    LOGE("Sensor list lequest failed with error: %d",
         sensorListResponse.Resp.sns_err_t);
  } else {
    success = true;
    for (uint32_t i = 0; i < sensorListResponse.SensorInfo_len; i++) {
      uint8_t sensorId = sensorListResponse.SensorInfo[i].SensorID;
      if (!getSensorsForSensorId(sensorId, sensors)) {
        success = false;
        break;
      }
    }
  }

  return success;
}

bool PlatformSensor::applyRequest(const SensorRequest& request) {
  // Adds a sensor monitor the first time this sensor is requested.
  addSensorMonitor(this->sensorId);

  // Determines whether a (passive) request is allowed at this point.
  bool requestAllowed = isRequestAllowed(getSensorType(), request);

  // If request is not allowed, turn off the sensor. Otherwise, make request.
  SensorRequest offRequest;
  bool success = makeRequest(getSensorType(),
                             requestAllowed ? request : offRequest);
  return success;
}

SensorType PlatformSensor::getSensorType() const {
  return getSensorTypeFromSensorId(this->sensorId, this->dataType,
                                   this->calType);
}

uint64_t PlatformSensor::getMinInterval() const {
  return minInterval;
}

const char *PlatformSensor::getSensorName() const {
  return sensorName;
}

PlatformSensor::PlatformSensor(PlatformSensor&& other) {
  // Our move assignment operator doesn't assume that "this" is initialized, so
  // we can just use that here
  *this = std::move(other);
}

PlatformSensor& PlatformSensor::operator=(PlatformSensor&& other) {
  // Note: if this implementation is ever changed to depend on "this" containing
  // initialized values, the move constructor implemenation must be updated
  sensorId = other.sensorId;
  dataType = other.dataType;
  calType = other.calType;
  memcpy(sensorName, other.sensorName, SNS_SMGR_MAX_SENSOR_NAME_SIZE_V01);
  minInterval = other.minInterval;

  lastEvent = other.lastEvent;
  other.lastEvent = nullptr;

  lastEventSize = other.lastEventSize;
  other.lastEventSize = 0;

  lastEventValid = other.lastEventValid;
  isSensorOff = other.isSensorOff;

  return *this;
}

ChreSensorData *PlatformSensor::getLastEvent() const {
  return (this->lastEventValid) ? this->lastEvent : nullptr;
}

void PlatformSensorBase::setLastEvent(const ChreSensorData *event) {
  memcpy(this->lastEvent, event, this->lastEventSize);
  this->lastEventValid = true;
}

qmi_client_type getSensorServiceQmiClientHandle() {
  return gPlatformSensorServiceQmiClientHandle;
}

}  // namespace chre
