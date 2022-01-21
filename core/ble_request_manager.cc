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

#include "chre/core/ble_request_manager.h"

#include "chre/core/event_loop_manager.h"
#include "chre/platform/fatal_error.h"
#include "chre/platform/log.h"
#include "chre/util/nested_data_ptr.h"

namespace chre {

void BleRequestManager::init() {
  mPlatformBle.init();
}

uint32_t BleRequestManager::getCapabilities() {
  return mPlatformBle.getCapabilities();
}

uint32_t BleRequestManager::getFilterCapabilities() {
  return mPlatformBle.getFilterCapabilities();
}

bool BleRequestManager::updateRequests(BleRequest &&request,
                                       bool *requestChanged,
                                       size_t *requestIndex) {
  bool success = true;
  BleRequest *foundRequest =
      mRequests.findRequest(request.getInstanceId(), requestIndex);
  if (foundRequest != nullptr) {
    if (foundRequest->getRequestStatus() != RequestStatus::APPLIED) {
      handleAsyncResult(request.getInstanceId(), request.isEnabled(),
                        false /* success */, CHRE_ERROR_OBSOLETE_REQUEST,
                        true /* forceUnregister */);
    }
    mRequests.updateRequest(*requestIndex, std::move(request), requestChanged);
  } else if (request.isEnabled()) {
    success =
        mRequests.addRequest(std::move(request), requestIndex, requestChanged);
  } else {
    // Already disabled requests shouldn't result in work for the PAL.
    *requestChanged = false;
    *requestIndex = mRequests.getRequests().size();
  }
  return success;
}

bool BleRequestManager::startScanAsync(Nanoapp *nanoapp, chreBleScanMode mode,
                                       uint32_t reportDelayMs,
                                       const struct chreBleScanFilter *filter) {
  CHRE_ASSERT(nanoapp);
  BleRequest request(nanoapp->getInstanceId(), true, mode, reportDelayMs,
                     filter);
  return configure(std::move(request));
}

bool BleRequestManager::stopScanAsync(Nanoapp *nanoapp) {
  CHRE_ASSERT(nanoapp);
  BleRequest request(nanoapp->getInstanceId(), false /* enable */);
  return configure(std::move(request));
}

bool BleRequestManager::configure(BleRequest &&request) {
  bool success = validateParams(request);
  if (success) {
    bool requestChanged = false;
    size_t requestIndex = 0;
    uint32_t instanceId = request.getInstanceId();
    uint8_t enabled = request.isEnabled();
    success =
        updateRequests(std::move(request), &requestChanged, &requestIndex);
    if (success) {
      if (!asyncResponsePending()) {
        if (!requestChanged) {
          handleAsyncResult(instanceId, enabled, true /* success */,
                            CHRE_ERROR_NONE);
        } else {
          success = controlPlatform();
          if (!success) {
            handleNanoappEventRegistration(instanceId, enabled,
                                           false /* success */,
                                           true /* forceUnregister */);
            mRequests.removeRequest(requestIndex, &requestChanged);
          }
        }
      }
    }
  }
  return success;
}

bool BleRequestManager::controlPlatform() {
  bool success = false;
  const BleRequest &maxRequest = mRequests.getCurrentMaximalRequest();
  if (maxRequest.isEnabled()) {
    chreBleScanFilter filter = maxRequest.getScanFilter();
    success = mPlatformBle.startScanAsync(
        maxRequest.getMode(), maxRequest.getReportDelayMs(), &filter);
  } else {
    success = mPlatformBle.stopScanAsync();
  }
  if (success) {
    mExpectedPlatformState = maxRequest.isEnabled();
    for (BleRequest &req : mRequests.getMutableRequests()) {
      if (req.getRequestStatus() == RequestStatus::PENDING_REQ) {
        req.setRequestStatus(RequestStatus::PENDING_RESP);
      }
    }
  }

  return success;
}

void BleRequestManager::handleFreeAdvertisingEvent(
    struct chreBleAdvertisementEvent *event) {
  mPlatformBle.releaseAdvertisingEvent(event);
}

void BleRequestManager::freeAdvertisingEventCallback(uint16_t /* eventType */,
                                                     void *eventData) {
  auto event = static_cast<chreBleAdvertisementEvent *>(eventData);
  EventLoopManagerSingleton::get()
      ->getBleRequestManager()
      .handleFreeAdvertisingEvent(event);
}

void BleRequestManager::handleAdvertisementEvent(
    struct chreBleAdvertisementEvent *event) {
  EventLoopManagerSingleton::get()->getEventLoop().postEventOrDie(
      CHRE_EVENT_BLE_ADVERTISEMENT, event, freeAdvertisingEventCallback);
}

void BleRequestManager::handlePlatformChange(bool enable, uint8_t errorCode) {
  auto callback = [](uint16_t /*type*/, void *data, void *extraData) {
    bool enable = NestedDataPtr<bool>(data);
    uint8_t errorCode = NestedDataPtr<uint8_t>(extraData);
    EventLoopManagerSingleton::get()
        ->getBleRequestManager()
        .handlePlatformChangeSync(enable, errorCode);
  };

  EventLoopManagerSingleton::get()->deferCallback(
      SystemCallbackType::BleScanResponse, NestedDataPtr<bool>(enable),
      callback, NestedDataPtr<uint8_t>(errorCode));
}

void BleRequestManager::handlePlatformChangeSync(bool enable,
                                                 uint8_t errorCode) {
  bool success = (errorCode == CHRE_ERROR_NONE);
  if (mExpectedPlatformState != enable) {
    errorCode = CHRE_ERROR;
    success = false;
    CHRE_ASSERT_LOG(false, "BLE PAL did not transition to expected state");
  }
  if (mInternalRequestPending) {
    mInternalRequestPending = false;
    if (!success) {
      FATAL_ERROR("Failed to resync BLE platform");
    }
  } else {
    for (BleRequest &req : mRequests.getMutableRequests()) {
      if (req.getRequestStatus() == RequestStatus::PENDING_RESP) {
        handleAsyncResult(req.getInstanceId(), req.isEnabled(), success,
                          errorCode);
        if (success) {
          req.setRequestStatus(RequestStatus::APPLIED);
        }
      }
    }

    if (!success) {
      mRequests.removeRequests(RequestStatus::PENDING_RESP);
    }
  }
  if (success) {
    // No need to waste memory for requests that have no effect on the overall
    // maximal request.
    mRequests.removeDisabledRequests();
  }

  dispatchPendingRequests();

  // Only clear mResyncPending if the request succeeded or after all pending
  // requests are dispatched and a resync request can be issued with only the
  // requests that were previously applied.
  if (mResyncPending) {
    if (success) {
      mResyncPending = false;
    } else if (!success && !asyncResponsePending()) {
      mResyncPending = false;
      resyncPlatform();
    }
  }
}

void BleRequestManager::dispatchPendingRequests() {
  if (mRequests.hasRequests(RequestStatus::PENDING_REQ)) {
    if (!controlPlatform()) {
      for (const BleRequest &req : mRequests.getRequests()) {
        if (req.getRequestStatus() == RequestStatus::PENDING_REQ) {
          handleAsyncResult(req.getInstanceId(), req.isEnabled(),
                            false /* success */, CHRE_ERROR,
                            true /* forceUnregister */);
        }
      }
      mRequests.removeRequests(RequestStatus::PENDING_REQ);
    }
  }
}

void BleRequestManager::handleAsyncResult(uint32_t instanceId, bool enabled,
                                          bool success, uint8_t errorCode,
                                          bool forceUnregister) {
  uint8_t requestType = enabled ? CHRE_BLE_REQUEST_TYPE_START_SCAN
                                : CHRE_BLE_REQUEST_TYPE_STOP_SCAN;
  postAsyncResultEventFatal(instanceId, requestType, success, errorCode);
  handleNanoappEventRegistration(instanceId, enabled, success, forceUnregister);
}

void BleRequestManager::handleNanoappEventRegistration(uint32_t instanceId,
                                                       bool enabled,
                                                       bool success,
                                                       bool forceUnregister) {
  Nanoapp *nanoapp =
      EventLoopManagerSingleton::get()->getEventLoop().findNanoappByInstanceId(
          instanceId);
  if (nanoapp != nullptr) {
    if (success && enabled) {
      nanoapp->registerForBroadcastEvent(CHRE_EVENT_BLE_ADVERTISEMENT);
    } else if (!enabled || forceUnregister) {
      nanoapp->unregisterForBroadcastEvent(CHRE_EVENT_BLE_ADVERTISEMENT);
    }
  }
}

void BleRequestManager::handleRequestStateResyncCallback() {
  auto callback = [](uint16_t /* eventType */, void * /* eventData */,
                     void * /* extraData */) {
    EventLoopManagerSingleton::get()
        ->getBleRequestManager()
        .handleRequestStateResyncCallbackSync();
  };
  EventLoopManagerSingleton::get()->deferCallback(
      SystemCallbackType::BleRequestResyncEvent, nullptr /* data */, callback);
}

void BleRequestManager::handleRequestStateResyncCallbackSync() {
  if (asyncResponsePending()) {
    mResyncPending = true;
  } else {
    resyncPlatform();
  }
}

void BleRequestManager::resyncPlatform() {
  if (controlPlatform()) {
    mInternalRequestPending = true;
  } else {
    FATAL_ERROR("Failed to send resync request to BLE platform");
  }
}

bool BleRequestManager::asyncResponsePending() {
  return (mInternalRequestPending ||
          mRequests.hasRequests(RequestStatus::PENDING_RESP));
}

bool BleRequestManager::validateParams(const BleRequest &request) {
  bool valid = true;
  if (request.isEnabled()) {
    for (const chreBleGenericFilter &filter : request.getGenericFilters()) {
      if (!isValidAdType(filter.type)) {
        valid = false;
        break;
      }

      uint8_t expectedLen = getFilterLenByAdType(filter.type);
      if (expectedLen != filter.len) {
        valid = false;
        break;
      }
    }
  }
  return valid;
}

void BleRequestManager::postAsyncResultEventFatal(uint32_t instanceId,
                                                  uint8_t requestType,
                                                  bool success,
                                                  uint8_t errorCode) {
  chreAsyncResult *event = memoryAlloc<chreAsyncResult>();
  if (event == nullptr) {
    FATAL_ERROR("Failed to alloc BLE async result");
  } else {
    event->requestType = requestType;
    event->success = success;
    event->errorCode = errorCode;
    event->reserved = 0;

    EventLoopManagerSingleton::get()->getEventLoop().postEventOrDie(
        CHRE_EVENT_BLE_ASYNC_RESULT, event, freeEventDataCallback, instanceId);
  }
}

bool BleRequestManager::isValidAdType(uint8_t adType) {
  return adType == CHRE_BLE_FILTER_TYPE_SERVICE_DATA_UUID_16 ||
         adType == CHRE_BLE_FILTER_TYPE_SERVICE_DATA_UUID_32 ||
         adType == CHRE_BLE_FILTER_TYPE_SERVICE_DATA_UUID_128;
}

uint8_t BleRequestManager::getFilterLenByAdType(uint8_t adType) {
  switch (adType) {
    case CHRE_BLE_FILTER_TYPE_SERVICE_DATA_UUID_16:
      return 2;
    case CHRE_BLE_FILTER_TYPE_SERVICE_DATA_UUID_32:
      return 4;
    case CHRE_BLE_FILTER_TYPE_SERVICE_DATA_UUID_128:
      return 16;
    default:
      CHRE_ASSERT(false);
      return UINT8_MAX;
  }
}

}  // namespace chre
