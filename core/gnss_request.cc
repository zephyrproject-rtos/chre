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

#include "chre/core/gnss_request.h"

namespace chre {

GnssRequest::GnssRequest()
    : mMinInterval(UINT64_MAX) {}

GnssRequest::GnssRequest(Milliseconds minInterval)
    : mMinInterval(minInterval) {}

GnssRequest::GnssRequest(uint32_t nanoappInstanceId, Milliseconds minInterval)
    : mNanoappInstanceId(nanoappInstanceId), mMinInterval(minInterval) {}

bool GnssRequest::isEquivalentTo(const GnssRequest& request) const {
  return (mMinInterval == request.getMinInterval());
}

bool GnssRequest::mergeWith(const GnssRequest& request) {
  bool attributesChanged = false;
  if (request.mMinInterval < mMinInterval) {
    mMinInterval = request.mMinInterval;
    attributesChanged = true;
  }

  return attributesChanged;
}

Milliseconds GnssRequest::getMinInterval() const {
  return mMinInterval;
}

bool GnssRequest::isEnabled() const {
  return mNanoappInstanceId.has_value();
}

}  // namespace chre
