/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef CHRE_CORE_BLE_REQUEST_MANAGER_H_
#define CHRE_CORE_BLE_REQUEST_MANAGER_H_

#include "chre/core/ble_request.h"
#include "chre/core/ble_request_multiplexer.h"
#include "chre/core/nanoapp.h"
#include "chre/platform/platform_ble.h"
#include "chre/util/non_copyable.h"

namespace chre {

/**
 * Manages requests for ble resources from nanoapps and multiplexes these
 * requests into the platform-specific implementation of the ble subsystem.
 */
class BleRequestManager : public NonCopyable {
 public:
  /**
   * Initializes the underlying platform-specific BLE module. Must be called
   * prior to invoking any other methods in this class.
   */
  void init();

  /**
   * @return the BLE capabilities exposed by this platform.
   */
  uint32_t getCapabilities();

  /**
   * @return the BLE filter capabilities exposed by this platform.
   */
  uint32_t getFilterCapabilities();

  /**
   * Begins a BLE scan asynchronously. The result is delivered through a
   * CHRE_EVENT_BLE_ASYNC_RESULT event.
   *
   * @param nanoapp The nanoapp starting the request.
   * @param mode Scanning mode selected among enum chreBleScanMode
   * @param reportDelayMs Maximum requested batching delay in ms. 0 indicates no
   *                      batching. Note that the system may deliver results
   *                      before the maximum specified delay is reached.
   * @param filter Pointer to the requested best-effort filter configuration as
   *               defined by struct chreBleScanFilter. The ownership of filter
   *               and its nested elements remains with the caller, and the
   *               caller may release it as soon as chreBleStartScanAsync()
   *               returns.
   * @return true if scan was successfully enabled.
   */
  bool startScanAsync(Nanoapp *nanoapp, chreBleScanMode mode,
                      uint32_t reportDelayMs,
                      const struct chreBleScanFilter *filter);

  /**
   * End a BLE scan asynchronously. The result is delivered through a
   * CHRE_EVENT_BLE_ASYNC_RESULT event.
   *
   * @param nanoapp The nanoapp stopping the request.
   * @return true if scan was successfully ended.
   */
  bool stopScanAsync(Nanoapp *nanoapp);

  /**
   * Frees an advertising event that was previously provided to the BLE
   * manager.
   *
   * @param event the event to release.
   */
  void handleFreeAdvertisingEvent(struct chreBleAdvertisementEvent *event);

  /**
   * Releases BLE Advertising Event after nanoapps have processed it.
   *
   * @param eventType the type of event being freed.
   * @param eventData a pointer to the scan event to release.
   */
  static void freeAdvertisingEventCallback(uint16_t eventType, void *eventData);

  /**
   * Handles a CHRE BLE advertisement event.
   *
   * @param event The BLE advertisement event containing BLE advertising
   *              reports. This memory is guaranteed not to be modified until it
   *              has been explicitly released through the PlatformBle instance.
   */
  void handleAdvertisementEvent(struct chreBleAdvertisementEvent *event);

  /**
   * Handles the result of a request to the PlatformBle to enable or end a scan.
   *
   * @param enable true if the scan is being enabled, false if not.
   * @param errorCode an error code that is used to indicate success or what
   *                  type of error has occurred. See chreError enum in the CHRE
   *                  API for additional details.
   */
  void handlePlatformChange(bool enable, uint8_t errorCode);

  /**
   * Invoked as a result of a requestStateResync() callback from the BLE PAL.
   * Runs asynchronously in the context of the callback immediately.
   */
  void handleRequestStateResyncCallback();

 private:
  //! Multiplexer used to keep track of BLE requests from nanoapps.
  BleRequestMultiplexer mRequests;

  // The platform BLE interface.
  PlatformBle mPlatformBle;

  // True if a request from the PAL is currently pending.
  bool mInternalRequestPending;

  // True if a state resync callback is pending to be processed.
  bool mResyncPending;

  /**
   * Configures BLE platform based on the current maximal BleRequest.
   */
  bool controlPlatform();

  /**
   * Processes nanoapp requests to start and stop a scan and updates BLE
   * platform if necessary.
   *
   * @param request BLE request to start or stop scan.
   * @return true if request was successfully processed.
   */
  bool configure(BleRequest &&request);

  /**
   * Update active BLE scan requests upon successful starting or ending a scan
   * and register or unregister nanoapp for BLE broadcast events.
   *
   * @param request Scan requested by nanoapp, only valid if nanoappEnabled is
   *                true.
   * @param requestChanged Indicates when the new request resulted in a change
   *                       to the underlying maximal request
   * @param requestIndex If equal to mRequests.size(), indicates the request
   *                     wasn't added (perhaps due to removing a non-existent
   *                     request). Otherwise, indicates the correct index for
   *                     the request.
   * @return true if requests were successfully updated.
   */
  bool updateRequests(BleRequest &&request, bool *requestChanged,
                      size_t *requestIndex);

  /**
   * Handles the result of a request to the PlatformBle to enable or end a scan.
   * This method is intended to be invoked on the CHRE event loop thread. The
   * handlePlatformChange method which may be called from any thread. For
   * parameter details,
   * @see handleAdvertisementEvent
   */
  void handlePlatformChangeSync(bool enable, uint8_t errorCode);

  /**
   * Dispatches pending state transitions on the queue until the first one
   * succeeds.
   */
  void dispatchQueuedStateTransitions();

  /**
   * Handles an async result, sending the result to the requesting nanoapp and
   * registering/unregistering it from the appropriate broadcast
   *
   * @param req The original request that resulted in the async result
   * @param success Whether the request was processed by the PAL successfully
   * @param errorCode Error code resulting from the request
   * @param forceUnregister Whether the nanoapp should be force unregistered
   *                        from BLE broadcast events.
   */
  void handleAsyncResult(const BleRequest &req, bool success, uint8_t errorCode,
                         bool forceUnregister = false);

  /**
   * Invoked as a result of a requestStateResync() callback from the BLE PAL.
   * Runs in the context of the CHRE thread.
   */
  void handleRequestStateResyncCallbackSync();

  /**
   * Sends request to resync BLE platform based on current maximal request.
   */
  void resyncPlatform();

  /**
   * @return true if an async response is pending from BLE. This method should
   * be used to check if a BLE platform request is in progress.
   */
  bool asyncResponsePending();

  /**
   * Validates the parameters given to ensure that they can be issued to the
   * PAL.
   *
   * @param request BleRequest sent by a nanoapp.
   */
  static bool validateParams(const BleRequest &request);

  /**
   * Posts the result of a BLE start/stop scan request.
   *
   * @param instanceId The nanoapp instance ID that made the request.
   * @param requestType The type of BLE request the nanoapp issued.
   * @param success true if the operation was successful.
   * @param errorCode the error code as a result of this operation.
   */
  static void postAsyncResultEventFatal(uint32_t instanceId,
                                        uint8_t requestType, bool success,
                                        uint8_t errorCode);

  /**
   * @return Returns CHRE request type for a given request
   */
  static uint8_t getRequestTypeForRequest(const BleRequest &request);

  /**
   * @return True if the given advertisement type is valid
   */
  static bool isValidAdType(uint8_t adType);

  /**
   * @return Valid length associated with a given advertisement type. UINT8_MAX
   *         if ad type is invalid.
   */
  static uint8_t getFilterLenByAdType(uint8_t adType);
};

}  // namespace chre

#endif  // CHRE_CORE_BLE_REQUEST_MANAGER_H_
