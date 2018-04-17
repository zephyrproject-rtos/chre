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

#ifndef CHRE_PLATFORM_SLPI_SEE_SEE_HELPER_H_
#define CHRE_PLATFORM_SLPI_SEE_SEE_HELPER_H_

#include "qmi_client.h"

#include "chre/core/sensor_type.h"
#include "chre/platform/condition_variable.h"
#include "chre/platform/mutex.h"
#include "chre/platform/slpi/see/see_helper_internal.h"
#include "chre/util/dynamic_vector.h"
#include "chre/util/non_copyable.h"
#include "chre/util/time.h"
#include "chre/util/unique_ptr.h"

namespace chre {

inline bool suidsMatch(const sns_std_suid& suid0, const sns_std_suid& suid1) {
  return (suid0.suid_high == suid1.suid_high
          && suid0.suid_low == suid1.suid_low);
}

//! A callback interface for receiving SeeHelper data events.
class SeeHelperCallbackInterface {
 public:
  struct SamplingStatusData {
    SensorType sensorType;
    struct chreSensorSamplingStatus status;
    bool enabledValid;
    bool intervalValid;
    bool latencyValid;
  };

  virtual ~SeeHelperCallbackInterface() {}

  //! Invoked by the SEE thread to update sampling status.
  virtual void onSamplingStatusUpdate(
      UniquePtr<SamplingStatusData>&& status) = 0;

  //! Invoked by the SEE thread to provide sensor data events. The event data
  //! format is one of the chreSensorXXXData defined in the CHRE API, implicitly
  //! specified by sensorType.
  virtual void onSensorDataEvent(
      SensorType sensorType, UniquePtr<uint8_t>&& eventData) = 0;

  //! Invoked by the SEE thread to update the AP wake/suspend status.
  virtual void onHostWakeSuspendEvent(bool apAwake) = 0;
};

//! Default timeout for waitForService. Have a longer timeout since there may be
//! external dependencies blocking SEE initialization.
constexpr Nanoseconds kDefaultSeeWaitTimeout = Seconds(5);

//! Default timeout for sendReq response
constexpr Nanoseconds kDefaultSeeRespTimeout = Seconds(1);

//! Default timeout for sendReq indication
constexpr Nanoseconds kDefaultSeeIndTimeout = Seconds(2);

//! Length of the char array to store sensor string attributes.
constexpr size_t kSeeAttrStrValLen = 64;

//! A struct to facilitate getAttributesSync().
struct SeeAttributes {
  char vendor[kSeeAttrStrValLen];
  char name[kSeeAttrStrValLen];
  char type[kSeeAttrStrValLen];
  int64_t hwId;
  float maxSampleRate;
  uint8_t streamType;
  bool passiveRequest;
};

//! A struct to facilitate making sensor request
struct SeeSensorRequest {
  SensorType sensorType;
  bool enable;
  bool passive;
  float samplingRateHz;
  uint32_t batchPeriodUs;
};

// TODO(P2-aa0089): Replace QMI with an interface that doesn't introduce big
// image wakeups.

/**
 * A helper class for making requests to Qualcomm's Sensors Execution
 * Environment (SEE) via QMI and waiting for the corresponding indication
 * message if applicable. Not safe to use from multiple threads.
 * Only one synchronous request can be made at a time.
 */
class SeeHelper : public NonCopyable {
 public:
  //! A struct to facilitate mapping between 'SUID + qmiHandle' and SensorType.
  struct SensorInfo {
    sns_std_suid suid;
    SensorType sensorType;
    qmi_client_type qmiHandle;
  };

  /**
   * Deinits QMI clients before destructing this object.
   */
  ~SeeHelper();

  /**
   * A synchronous call to discover SUID(s) that supports the specified data
   * type. This API will clear the provided dynamic vector before populating it.
   *
   * @param dataType A data type string, "accel" for example.
   * @param suids A non-null pointer to a list of sensor UIDs that support the
   *              specified data type.
   * @param minNumSuids The minimum number of SUIDs it needs to find before
   *                    returning true. Otherwise, it'll re-try internally
   *                    until it times out. It's illegal to set it to 0.
   * @param maxRetries Maximum amount of times to retry looking up the SUID
   *                   until giving up.
   * @param retryDelay Time delay between retry attempts (msec).
   *
   * @return true if at least minNumSuids were successfully found
   */
  bool findSuidSync(const char *dataType, DynamicVector<sns_std_suid> *suids,
                    uint8_t minNumSuids = 1, uint32_t maxRetries = 20,
                    Milliseconds retryDelay = Milliseconds(500));

  /**
   * A synchronous call to obtain the attributes of the specified SUID.
   *
   * @param suid The SUID of the sensor
   * @param attr A non-null pointer to the attibutes of the specified SUID that
   *             include sensor vendor, name and max sampling rate, etc.
   *
   * @return true if the attribute was successfully obtained and attr populated.
   */
  bool getAttributesSync(const sns_std_suid& suid, SeeAttributes *attr);

  /**
   * Initializes and waits for the sensor client QMI service to become
   * available, and obtains remote_proc and cal sensors' info for future
   * operations. This function must be called first to initialize the object and
   * be called only once.
   *
   * @param cbIf A pointer to the callback interface that will be invoked to
   *             handle all async requests with callback data type defined in
   *             the interface.
   * @param timeout The wait timeout in microseconds.
   *
   * @return true if all initialization steps succeeded.
   */
  bool init(SeeHelperCallbackInterface *cbIf,
            Microseconds timeout = kDefaultSeeWaitTimeout);

  /**
   * Makes a sensor request to SEE.
   *
   * @param request The sensor request to make.
   *
   * @return true if the QMI request has been successfully made.
   */
  bool makeRequest(const SeeSensorRequest& request);

  /**
   * Register a SensorType with the SUID of the SEE sensor/driver.
   *
   * Only registered SUIDs will call the indication callback provided in init()
   * with populated CHRE sensor events. Each SUID/SensorType pair can only be
   * registered once. It's illegal to register SensorType::Unknown.
   *
   * If an SUID is registered with a second SensorType, another QMI client may
   * be created to disambiguate the SUID representation.
   *
   * @param sensorType The SensorType to register.
   * @param suid The SUID of the sensor.
   * @param prevRegistered A non-null pointer to a boolean that indicates
   *        whether the SUID/SensorType pair has been previously registered.
   *
   * @return true if the SUID/SensorType pair was successfully registered.
   */
  bool registerSensor(SensorType sensorType, const sns_std_suid& suid,
                      bool *prevRegistered);

  /**
   * Checks whether the given SensorType has been successfully registered
   * already via registerSensor().
   *
   * @param sensorType The SensorType to check.
   *
   * @return true if the given sensor type has been registered, false otherwise
   */
  bool sensorIsRegistered(SensorType sensorType) const;

 protected:
  /**
   * Get the cached SUID of a calibration sensor that corresponds to the
   * specified sensorType.
   *
   * @param sensorType The sensor type of the calibration sensor.
   *
   * @return A constant reference to the calibration sensor's SUID if present.
   *         Otherwise, a reference to sns_suid_sensor_init_zero is returned.
   */
  const sns_std_suid& getCalSuidFromSensorType(SensorType sensorType) const;

  /**
   * A convenience method to send a QMI request and wait for the indication if
   * it's a synchronous one using the default QMI handle obtained in init().
   *
   * @see sendReq
   */
  bool sendReq(
      const sns_std_suid& suid,
      void *syncData, const char *syncDataType,
      uint32_t msgId, void *payload, size_t payloadLen,
      bool batchValid, uint32_t batchPeriodUs, bool passive,
      bool waitForIndication,
      Nanoseconds timeoutResp = kDefaultSeeRespTimeout,
      Nanoseconds timeoutInd = kDefaultSeeIndTimeout) {
    return sendReq(mQmiHandles[0], suid,
                   syncData, syncDataType,
                   msgId, payload, payloadLen,
                   batchValid, batchPeriodUs, passive,
                   waitForIndication,
                   timeoutResp, timeoutInd);
  }

 private:
  //! Used to synchronize indications.
  ConditionVariable mCond;

  //! Used with mCond, and to protect access to member variables from other
  //! threads.
  Mutex mMutex;

  //! Callback interface for sensor events.
  SeeHelperCallbackInterface *mCbIf = nullptr;

  //! The list of QMI handles initiated by SeeHelper.
  DynamicVector<qmi_client_type> mQmiHandles;

  //! The list of SensorTypes registered and their corresponding SUID and
  //! QMI handle.
  DynamicVector<SensorInfo> mSensorInfos;

  //! Data struct to store sync APIs data.
  void *mSyncData = nullptr;

  //! The data type whose indication this SeeHelper is waiting for in
  //! findSuidSync.
  const char *mSyncDataType = nullptr;

  //! The SUID whose indication this SeeHelper is waiting for in a sync call.
  sns_std_suid mSyncSuid = sns_suid_sensor_init_zero;

  //! true if we are waiting on an indication for a sync call.
  bool mWaiting = false;

  //! The SUID for the remote_proc sensor.
  Optional<sns_std_suid> mRemoteProcSuid;

  //! Cal info of all the cal sensors.
  SeeCalInfo mCalInfo[kNumSeeCalSensors];

  /**
   * Initializes SEE calibration sensors and makes data request.
   *
   * @return true if cal sensor have been succcessfully initialized.
   */
  bool initCalSensors();

  /**
   * Initializes the SEE remote processor sensor and makes a data request.
   *
   * @return true if the remote proc sensor was successfully initialized.
   */
  bool initRemoteProcSensor();

  /**
   * Wrapper to send a QMI request and wait for the indication if it's a
   * synchronous one.
   *
   * Only one request can be pending at a time per instance of SeeHelper.
   *
   * @param qmiHandle The QMI Handle to make QMI requests with.
   * @param suid The SUID of the sensor the request is sent to
   * @param syncData The data struct or container to receive a sync call's data
   * @param syncDataType The data type we are waiting for.
   * @param msgId Message ID of the request to send
   * @param payload A non-null pointer to the pb-encoded message
   * @param payloadLen The length of payload
   * @param batchValid Whether batchPeriodUs is valid and applicable to this
   *                   request
   * @param batchPeriodUs The batch period in microseconds
   * @param passive Whether this is a passive request
   * @param waitForIndication Whether to wait for the indication of the
   *                          specified SUID or not.
   * @param timeoutRresp How long to wait for the response before abandoning it
   * @param timeoutInd How long to wait for the indication before abandoning it
   *
   * @return true if the request has been sent and the response/indication it's
   *         waiting for has been successfully received
   */
  bool sendReq(
      const qmi_client_type& qmiHandle, const sns_std_suid& suid,
      void *syncData, const char *syncDataType,
      uint32_t msgId, void *payload, size_t payloadLen,
      bool batchValid, uint32_t batchPeriodUs, bool passive,
      bool waitForIndication,
      Nanoseconds timeoutResp = kDefaultSeeRespTimeout,
      Nanoseconds timeoutInd = kDefaultSeeIndTimeout);

  /**
   * Handles the payload of a sns_client_report_ind_msg_v01 message.
   */
  void handleSnsClientEventMsg(
      qmi_client_type clientHandle, const void *payload, size_t payloadLen);

  /**
   * Processes a QMI indication callback
   *
   * @see qmi_client_ind_cb
   */
  void handleInd(qmi_client_type client_handle, unsigned int msg_id,
                 const void *ind_buf, unsigned int ind_buf_len);

  /**
   * Extracts "this" from ind_cb_data and calls through to handleInd()
   *
   * @see qmi_client_ind_cb
   */
  static void qmiIndCb(qmi_client_type client_handle, unsigned int msg_id,
                       void *ind_buf, unsigned int ind_buf_len,
                       void *ind_cb_data);

  /**
   * A wrapper to initialize a QMI client.
   *
   * @ see qmi_client_init_instance
   */
  bool waitForService(qmi_client_type *qmiHandle,
                      Microseconds timeout = kDefaultSeeWaitTimeout);

  /**
   * Obtains the pointer to cal data by SUID.
   */
  SeeCalData *getCalDataFromSuid(const sns_std_suid& suid);

  /**
   * @return SensorInfo instance found in mSensorInfos with the given
   *         SensorType, or nullptr if not found
   */
  const SensorInfo *getSensorInfo(SensorType sensorType) const;
};

}  // namespace chre

#endif  // CHRE_PLATFORM_SLPI_SEE_SEE_HELPER_H_
