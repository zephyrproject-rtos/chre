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

#ifndef CHRE_PLATFORM_TIMER_H_
#define CHRE_PLATFORM_TIMER_H_

/**
 * @file
 * Abstracts a system timer from the underlying platform, which will invoke the
 * supplied callback after at least the given amount of time has passed. The
 * calling context for the callback is undefined, and may be inside an
 * interrupt, or in a different thread, etc. Therefore, the callback is
 * responsible for ensuring that it handles this potential concurrency
 * appropriately.
 */

#include <stdint.h>

#include "chre/target_platform/system_timer_base.h"
#include "chre/util/non_copyable.h"

namespace chre {

typedef void (SystemTimerCallback)(void *data);

class SystemTimer : private SystemTimerBase,
                    public NonCopyable {
 public:
  SystemTimer(SystemTimerCallback *callback, void *data);
  ~SystemTimer();

  /**
   * Initializes the timer. This must be called before other methods in this
   * function are called. It is an error
   *
   * @return true on successful, false on failure
   */
  bool init();

  /**
   * Sets the timer to expire after the given delay. If the timer was already
   * running, its expiry time is updated to this value.
   *
   * Note that it is possible for the timer to fire before this function
   * returns.
   *
   * @param delayNs Minimum delay until the first firing of the timer, in
   *        nanoseconds.
   * @param intervalNs Minimum delay for periodic firing of the timer after the
   *        first firing. If set to 0, the timer is only fires once and then
   *        stops.
   *
   * @return true on success, false on failure
   */
  bool set(uint64_t delayNs, uint64_t intervalNs=0);

  /**
   * Disarms the timer. If it was armed and is not currently in the process of
   * firing, this prevents the callback from being invoked until the timer is
   * restarted by a subsequent call to set().
   */
  bool cancel();

 private:
  friend class SystemTimerBase;

  SystemTimerCallback *mCallback;
  void *mData;
  bool mInitialized = false;
};

}

#endif  // CHRE_PLATFORM_TIMER_H_
