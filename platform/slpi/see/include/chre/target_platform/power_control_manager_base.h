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

#ifndef CHRE_PLATFORM_SLPI_SEE_POWER_CONTROL_MANAGER_BASE_H_
#define CHRE_PLATFORM_SLPI_SEE_POWER_CONTROL_MANAGER_BASE_H_

#include "chre/platform/mutex.h"
#include "chre/util/time.h"

extern "C" {

#include "sns_island_util.h"

} // extern "C"

namespace chre {

class PowerControlManagerBase {
 public:
  PowerControlManagerBase();
  ~PowerControlManagerBase();

  /**
   * Makes a power mode request. An actual vote to the SLPI power manager may
   * not be cast depending on current power mode and mBigImageRefCount.
   *
   * @param bigImage Whether to request bigImage or not.
   *
   * @return true if the vote returned success.
   */
  bool voteBigImage(bool bigImage);

  /**
   * Sets the AP awake/suspended state and posts an event to interested
   * nanoapps. This method should only be invoked by the SEE helper as a
   * result of an event from the remote_proc_state sensor.
   *
   * @param awake true if the AP is awake, false otherwise
   */
  void onHostWakeSuspendEvent(bool awake);

#ifdef CHRE_SLPI_UIMG_ENABLED
  /**
   * Increment the big image reference count when client needs to perform some
   * big image activity and holds the system in big image. A big image vote is
   * cast when the count increments from 0.
   */
  void incrementBigImageRefCount();

  /**
   * Decrement the big image reference count when client finishes some activity
   * that has to be performed in big image. A big image vote may be cast or
   * removed when the count decrements to 0, depending on the system's intended
   * power state.
   */
  void decrementBigImageRefCount();
#endif  // CHRE_SLPI_UIMG_ENABLED

 protected:
  //! Set to true if the host is awake, false if suspended.
  bool mHostIsAwake = true;

#ifdef CHRE_SLPI_UIMG_ENABLED
 private:
  //! Last big image request made through voteBigImage().
  bool mLastBigImageRequest = false;

  //! Last big image vote cast to sns_island_aggregator.
  bool mLastBigImageVote = false;

  //! Client handle for the island aggregator registration.
  sns_island_client_handle mClientHandle = nullptr;

  //! The system time mBigImageRefCount increments from 0.
  Milliseconds mRefCountStart = Milliseconds(0);

  //! The count of big image activities.
  uint32_t mBigImageRefCount = 0;

  //! Used to protect access to member variables from other threads.
  Mutex mMutex;

  /**
   * Cast a vote to sns_island_aggregator.
   *
   * @param bigImage Whether to vote for bigImage or not.
   *
   * @return true if the vote returned success.
   */
  bool voteSnsPowerMode(bool bigImage);
#endif  // CHRE_SLPI_UIMG_ENABLED
};

} // namespace chre

#endif // CHRE_PLATFORM_SLPI_SEE_POWER_CONTROL_MANAGER_BASE_H_
