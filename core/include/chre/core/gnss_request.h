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

#ifndef CHRE_CORE_GNSS_REQUEST_H_
#define CHRE_CORE_GNSS_REQUEST_H_

#include "chre/util/optional.h"
#include "chre/util/time.h"

namespace chre {

/**
 * Tracks the state of a GNSS resource (measurement or location) by nanoapp
 * instance ID and the requested interval. This class implements the API set
 * forth by the RequestMultiplexer container.
 */
class GnssRequest {
 public:
  /**
   * Default constructs a GNSS request to the minimal possible configuration.
   * The resource is not requested and the interval is set to the maximal value.
   */
  GnssRequest();

  /**
   * Constructs a GNSS request given a minimum interval.
   *
   * @param minInterval the minimum interval at which events can be delivered
   *        according to this request.
   */
  GnssRequest(Milliseconds minInterval);

  /**
   * Constructs a GNSS request given an owning nanoapp instance ID and interval.
   *
   * @param nanoappInstanceId the instance ID of the nanoapp that made this
   *        request.
   * @param minInterval the minimum interval at which events can be reported.
   */
  GnssRequest(uint32_t nanoappInstanceId, Milliseconds minInterval);

  /**
   * Performs an equivalency comparison of two GNSS requests. This determines if
   * the effective request for GNSS data is the same as another.
   *
   * @param request the request to compare against.
   * @return true if this request is equivalent to another.
   */
  bool isEquivalentTo(const GnssRequest& request) const;

  /**
   * Assigns the current request to the minimum interval of this request and
   * another.
   *
   * @param request the other request to compare attributes of.
   * @return true if any of the attributes of this request changed.
   */
  bool mergeWith(const GnssRequest& request);

  /**
   * @return the minimum reporting interval of GNSS data.
   */
  Milliseconds getMinInterval() const;

  /**
   * @return true if this requests in an active request for data.
   */
  bool isEnabled() const;

  /**
   * @return the instance ID of the nanoapp requesting this resource. This is
   *         only valid if isEnabled returns true.
   */
  uint32_t getNanoappInstanceId() const;

 private:
  //! The optional owning nanoapp instance ID of this request. This is optional
  //! because in the default state, the request does not have an owner.
  Optional<uint32_t> mNanoappInstanceId;

  //! The minimum reporting interval for this request.
  Milliseconds mMinInterval;
};

}  // namespace chre

#endif  // CHRE_CORE_GNSS_REQUEST_H_
