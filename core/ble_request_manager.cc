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
  // TODO(b/215417133): A nanoapp issuing multiple requests before each one
  // has received an async result breaks the current logic. Fix this.

  bool success = validateParams(request);
  if (success) {
    bool requestChanged = false;
    size_t requestIndex = 0;
    uint32_t instanceId = request.getInstanceId();
    uint8_t requestType = getRequestTypeForRequest(request);
    success =
        updateRequests(std::move(request), &requestChanged, &requestIndex);
    if (success) {
      if (!mRequests.hasRequests(RequestStatus::PENDING_RESP)) {
        Nanoapp *nanoapp = EventLoopManagerSingleton::get()
                               ->getEventLoop()
                               .findNanoappByInstanceId(instanceId);
        if (!requestChanged) {
          postAsyncResultEventFatal(instanceId, requestType, true /* success */,
                                    CHRE_ERROR_NONE);
          nanoapp->registerForBroadcastEvent(CHRE_EVENT_BLE_ADVERTISEMENT);
        } else {
          success = controlPlatform();
          if (!success) {
            nanoapp->unregisterForBroadcastEvent(CHRE_EVENT_BLE_ADVERTISEMENT);
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
  bool expectedEnable = false;
  for (BleRequest &req : mRequests.getMutableRequests()) {
    if (req.getRequestStatus() == RequestStatus::PENDING_RESP) {
      if (req.isEnabled()) {
        expectedEnable = true;
      }

      handleAsyncResult(req, success, errorCode);
      if (success) {
        req.setRequestStatus(RequestStatus::APPLIED);
      }
    }
  }

  if (!success) {
    mRequests.removeRequests(RequestStatus::PENDING_RESP);
  } else {
    CHRE_ASSERT_LOG(expectedEnable == enable,
                    "BLE PAL didn't transition to correct state");
    // No need to waste memory for requests that have no effect on the overall
    // maximal request.
    mRequests.removeDisabledRequests();
  }

  dispatchQueuedStateTransitions();
}

void BleRequestManager::dispatchQueuedStateTransitions() {
  if (mRequests.hasRequests(RequestStatus::PENDING_REQ)) {
    if (!controlPlatform()) {
      for (const BleRequest &req : mRequests.getRequests()) {
        if (req.getRequestStatus() == RequestStatus::PENDING_REQ) {
          handleAsyncResult(req, false /* success */, CHRE_ERROR,
                            true /* forceUnregister */);
        }
      }
      mRequests.removeRequests(RequestStatus::PENDING_REQ);
    }
  }
}

void BleRequestManager::handleAsyncResult(const BleRequest &req, bool success,
                                          uint8_t errorCode,
                                          bool forceUnregister) {
  postAsyncResultEventFatal(req.getInstanceId(), getRequestTypeForRequest(req),
                            success, errorCode);
  Nanoapp *nanoapp =
      EventLoopManagerSingleton::get()->getEventLoop().findNanoappByInstanceId(
          req.getInstanceId());
  if (nanoapp != nullptr) {
    if (success && req.isEnabled()) {
      nanoapp->registerForBroadcastEvent(CHRE_EVENT_BLE_ADVERTISEMENT);
    } else if (!req.isEnabled() || forceUnregister) {
      nanoapp->unregisterForBroadcastEvent(CHRE_EVENT_BLE_ADVERTISEMENT);
    }
  }
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

uint8_t BleRequestManager::getRequestTypeForRequest(const BleRequest &request) {
  return request.isEnabled() ? CHRE_BLE_REQUEST_TYPE_START_SCAN
                             : CHRE_BLE_REQUEST_TYPE_STOP_SCAN;
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
