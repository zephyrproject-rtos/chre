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

#ifndef CHRE_CORE_REQUEST_MULTIPLEXER_IMPL_H_
#define CHRE_CORE_REQUEST_MULTIPLEXER_IMPL_H_

#include "chre/platform/assert.h"

namespace chre {

template<typename RequestType>
bool RequestMultiplexer<RequestType>::addRequest(const RequestType& request,
                                                 bool *maximalRequestChanged) {
  CHRE_ASSERT(maximalRequestChanged);

  bool requestStored = mRequests.push_back(request);
  if (requestStored) {
    RequestType newMaximalRequest = mCurrentMaximalRequest
        .generateIntersectionOf(request);
    *maximalRequestChanged = !mCurrentMaximalRequest.isEquivalentTo(
        newMaximalRequest);
    if (*maximalRequestChanged) {
      mCurrentMaximalRequest = newMaximalRequest;
    }
  }

  return requestStored;
}

template<typename RequestType>
void RequestMultiplexer<RequestType>::removeRequest(
    size_t index, bool *maximalRequestChanged) {
  CHRE_ASSERT(maximalRequestChanged);
  CHRE_ASSERT(index < mRequests.size());

  if (index < mRequests.size()) {
    mRequests.erase(index);

    // TODO: We build the maximal request from scratch when removing which is
    // an O(n) operation. Consider adding an API surface to the RequestType
    // object along the lines of isLowerPriorityThan which returns true if all
    // attributes of the request are lower priority than the current maximal and
    // the request can be removed without re-computing the maximal.
    RequestType newMaximalRequest = computeMaximalRequest();
    *maximalRequestChanged = !mCurrentMaximalRequest.isEquivalentTo(
        newMaximalRequest);
    if (*maximalRequestChanged) {
      mCurrentMaximalRequest = newMaximalRequest;
    }
  }
}

template<typename RequestType>
const DynamicVector<RequestType>&
    RequestMultiplexer<RequestType>::getRequests() const {
  return mRequests;
}

template<typename RequestType>
RequestType RequestMultiplexer<RequestType>::getCurrentMaximalRequest() const {
  return mCurrentMaximalRequest;
}

template<typename RequestType>
RequestType RequestMultiplexer<RequestType>::computeMaximalRequest() const {
  RequestType maximalRequest;
  for (size_t i = 0; i < mRequests.size(); i++) {
    maximalRequest = maximalRequest.generateIntersectionOf(mRequests[i]);
  }

  return maximalRequest;
}

}  // namespace chre

#endif  // CHRE_CORE_REQUEST_MULTIPLEXER_IMPL_H_
