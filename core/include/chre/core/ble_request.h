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

#ifndef CHRE_CORE_BLE_REQUEST_H_
#define CHRE_CORE_BLE_REQUEST_H_

#include "chre/util/dynamic_vector.h"
#include "chre/util/non_copyable.h"
#include "chre_api/chre/ble.h"

namespace chre {

class BleRequest : public NonCopyable {
 public:
  BleRequest();

  BleRequest(uint32_t instanceId, bool enable);

  BleRequest(uint32_t instanceId, bool enable, chreBleScanMode mode,
             uint32_t reportDelayMs, const chreBleScanFilter *filter);

  BleRequest(uint32_t instanceId, bool enable, chreBleScanMode mode,
             uint32_t reportDelayMs, int8_t rssiThreshold);

  BleRequest(BleRequest &&other);

  BleRequest &operator=(BleRequest &&other);

  /**
   * Merges current request with other request. Takes maximum value of mode and
   * minimum value of reportDelayMs and rssiThreshold. Takes superset of generic
   * filters from both requests.
   *
   * @param request The other request to compare the attributes of.
   * @return true if any of the attributes of this request changed.
   */
  bool mergeWith(const BleRequest &request);

  /**
   * Checks whether current request is equivalent to the other request.
   *
   * @param request The other request to compare the attributes of.
   * @return true if the requests are equivalent.
   */
  bool isEquivalentTo(const BleRequest &request);

  /**
   * @return The instance id of the nanoapp that owns this request
   */
  uint32_t getInstanceId() const;

  /**
   * @return The scan mode of this request.
   */
  chreBleScanMode getMode() const;

  /**
   * @return The report delay of this request.
   */
  uint32_t getReportDelayMs() const;

  /**
   * @return The RSSI threshold of this request.
   */
  int8_t getRssiThreshold() const;

  /**
   * @return Generic filters of this request.
   */
  const DynamicVector<chreBleGenericFilter> &getGenericFilters() const;

  /**
   * @return chreBleScanFilter that is valid only as long as the internal
   *    contents of this class are not modified
   */
  chreBleScanFilter getScanFilter() const;

  /**
   * @return true if nanoapp intends to enable a request.
   */
  bool isEnabled() const;

 private:
  // Instance id of nanoapp that sent the request.
  uint32_t mInstanceId;

  // Whether a nanoapp intends to enable this request. If set to false, the
  // following members are invalid: mMode, mReportDelayMs, mFilter.
  bool mEnabled;

  // Scanning mode selected among enum chreBleScanMode.
  chreBleScanMode mMode;

  // Maximum requested batching delay in ms.
  uint32_t mReportDelayMs;

  // RSSI threshold filter.
  int8_t mRssiThreshold;

  // Generic scan filters.
  DynamicVector<chreBleGenericFilter> mFilters;
};

}  // namespace chre

#endif  // CHRE_CORE_BLE_REQUEST_H_