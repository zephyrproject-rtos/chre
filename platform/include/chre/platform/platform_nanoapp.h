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

#ifndef CHRE_PLATFORM_PLATFORM_NANOAPP_H_
#define CHRE_PLATFORM_PLATFORM_NANOAPP_H_

#include <cstdint>

#include "chre/util/non_copyable.h"
#include "chre/target_platform/platform_nanoapp_base.h"

namespace chre {

/**
 * An interface for calling into nanoapp entry points and managing the
 * lifecycle of a nanoapp.
 *
 * TODO: Look at unloading the app and freeing events that originate from this
 * nanoapp.
 */
class PlatformNanoapp : public PlatformNanoappBase, public NonCopyable {
 public:
  ~PlatformNanoapp();

  /**
   * Calls the start function of the nanoapp.
   *
   * @return true if the app was able to start successfully
   */
  bool start();

  /**
   * Calls the handleEvent function of the nanoapp.
   *
   * @param the instance ID of the sender
   * @param the type of the event being sent
   * @param the data passed in
   */
  void handleEvent(uint32_t senderInstanceId,
                   uint16_t eventType,
                   const void *eventData);

  /**
   * Calls the nanoapp's end callback.
   */
  void end();

  uint64_t getAppId() const;
  uint32_t getAppVersion() const;
  uint32_t getInstanceId() const;
  uint32_t getTargetApiVersion() const;
  bool isSystemNanoapp() const;
};

}  // namespace chre

#endif  // CHRE_PLATFORM_PLATFORM_NANOAPP_H_
