/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "chre/core/ble_request.h"
#include "chre/platform/fatal_error.h"
#include "chre/util/memory.h"

namespace chre {

BleRequest::BleRequest() : BleRequest(0, false) {}

BleRequest::BleRequest(uint16_t instanceId, bool enable)
    : BleRequest(instanceId, enable, CHRE_BLE_SCAN_MODE_BACKGROUND,
                 0 /* reportDelayMs */, nullptr /* filter */) {}

BleRequest::BleRequest(uint16_t instanceId, bool enable, chreBleScanMode mode,
                       uint32_t reportDelayMs, const chreBleScanFilter *filter)
    : mReportDelayMs(reportDelayMs),
      mInstanceId(instanceId),
      mMode(mode),
      mEnabled(enable),
      mRssiThreshold(CHRE_BLE_RSSI_THRESHOLD_NONE),
      mStatus(RequestStatus::PENDING_REQ) {
  if (filter != nullptr) {
    mRssiThreshold = filter->rssiThreshold;
    if (filter->scanFilterCount > 0) {
      if (!mFilters.resize(filter->scanFilterCount)) {
        FATAL_ERROR("Unable to reserve filter count");
      }
      for (size_t i = 0; i < filter->scanFilterCount; i++) {
        mFilters[i] = filter->scanFilters[i];
      }
    }
  }
}

BleRequest::BleRequest(BleRequest &&other) {
  *this = std::move(other);
}

BleRequest &BleRequest::operator=(BleRequest &&other) {
  mInstanceId = other.mInstanceId;
  mMode = other.mMode;
  mReportDelayMs = other.mReportDelayMs;
  mRssiThreshold = other.mRssiThreshold;
  mFilters = std::move(other.mFilters);
  mEnabled = other.mEnabled;
  mStatus = other.mStatus;
  return *this;
}

bool BleRequest::mergeWith(const BleRequest &request) {
  bool attributesChanged = false;
  if (!mEnabled && request.mEnabled) {
    mEnabled = true;
    attributesChanged = true;
  }

  // Only merge an enabled request to prevent disabled requests polluting things
  if (request.mEnabled) {
    if (mMode < request.mMode) {
      mMode = request.mMode;
      attributesChanged = true;
    }
    if (mReportDelayMs > request.mReportDelayMs) {
      mReportDelayMs = request.mReportDelayMs;
      attributesChanged = true;
    }
    if (mRssiThreshold > request.mRssiThreshold) {
      mRssiThreshold = request.mRssiThreshold;
      attributesChanged = true;
    }
    const DynamicVector<chreBleGenericFilter> &otherFilters = request.mFilters;
    if (!otherFilters.empty()) {
      attributesChanged = true;
      size_t originalFilterSize = mFilters.size();
      if (!mFilters.resize(originalFilterSize + otherFilters.size())) {
        FATAL_ERROR("Unable to reserve filter count");
      }
      for (size_t i = 0; i < otherFilters.size(); i++) {
        mFilters[originalFilterSize + i] = otherFilters[i];
      }
    }
  }
  return attributesChanged;
}

bool BleRequest::isEquivalentTo(const BleRequest &request) {
  const DynamicVector<chreBleGenericFilter> &otherFilters = request.mFilters;
  bool isEquivalent = (mEnabled && request.mEnabled && mMode == request.mMode &&
                       mReportDelayMs == request.mReportDelayMs &&
                       mRssiThreshold == request.mRssiThreshold &&
                       mFilters.size() == otherFilters.size());
  if (isEquivalent) {
    for (size_t i = 0; i < otherFilters.size(); i++) {
      if (mFilters[i].len != otherFilters[i].len ||
          mFilters[i].type != otherFilters[i].type ||
          mFilters[i].data != otherFilters[i].data ||
          mFilters[i].dataMask != otherFilters[i].dataMask) {
        isEquivalent = false;
        break;
      }
    }
  }
  return isEquivalent;
}

uint16_t BleRequest::getInstanceId() const {
  return mInstanceId;
}

chreBleScanMode BleRequest::getMode() const {
  return mMode;
}

uint32_t BleRequest::getReportDelayMs() const {
  return mReportDelayMs;
}

int8_t BleRequest::getRssiThreshold() const {
  return mRssiThreshold;
}

RequestStatus BleRequest::getRequestStatus() const {
  return mStatus;
}

void BleRequest::setRequestStatus(RequestStatus status) {
  mStatus = status;
}

const DynamicVector<chreBleGenericFilter> &BleRequest::getGenericFilters()
    const {
  return mFilters;
}

chreBleScanFilter BleRequest::getScanFilter() const {
  return chreBleScanFilter{
      mRssiThreshold, static_cast<uint8_t>(mFilters.size()), mFilters.data()};
}

bool BleRequest::isEnabled() const {
  return mEnabled;
}

}  // namespace chre