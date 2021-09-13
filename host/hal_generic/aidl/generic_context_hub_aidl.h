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

#ifndef ANDROID_HARDWARE_CONTEXTHUB_AIDL_CONTEXTHUB_H
#define ANDROID_HARDWARE_CONTEXTHUB_AIDL_CONTEXTHUB_H

#include <aidl/android/hardware/contexthub/BnContextHub.h>

#include "hal_chre_socket_connection.h"

#include <mutex>

namespace aidl {
namespace android {
namespace hardware {
namespace contexthub {

class ContextHub : public BnContextHub,
                   public ::android::hardware::contexthub::common::
                       implementation::IChreSocketCallback {
 public:
  ContextHub()
      : mDeathRecipient(
            AIBinder_DeathRecipient_new(ContextHub::onServiceDied)) {}

  ::ndk::ScopedAStatus getContextHubs(
      std::vector<ContextHubInfo> *out_contextHubInfos) override;
  ::ndk::ScopedAStatus loadNanoapp(int32_t contextHubId,
                                   const NanoappBinary &appBinary,
                                   int32_t transactionId,
                                   bool *_aidl_return) override;
  ::ndk::ScopedAStatus unloadNanoapp(int32_t contextHubId, int64_t appId,
                                     int32_t transactionId,
                                     bool *_aidl_return) override;
  ::ndk::ScopedAStatus disableNanoapp(int32_t contextHubId, int64_t appId,
                                      int32_t transactionId,
                                      bool *_aidl_return) override;
  ::ndk::ScopedAStatus enableNanoapp(int32_t contextHubId, int64_t appId,
                                     int32_t transactionId,
                                     bool *_aidl_return) override;
  ::ndk::ScopedAStatus onSettingChanged(Setting setting, bool enabled) override;
  ::ndk::ScopedAStatus queryNanoapps(int32_t contextHubId,
                                     bool *_aidl_return) override;
  ::ndk::ScopedAStatus registerCallback(
      int32_t contextHubId, const std::shared_ptr<IContextHubCallback> &cb,
      bool *_aidl_return) override;
  ::ndk::ScopedAStatus sendMessageToHub(int32_t contextHubId,
                                        const ContextHubMessage &message,
                                        bool *_aidl_return) override;

  void onNanoappMessage(const ::chre::fbs::NanoappMessageT &message) override;

  void onNanoappListResponse(
      const ::chre::fbs::NanoappListResponseT &response) override;

  void onTransactionResult(uint32_t transactionId, bool success) override;

  void onContextHubRestarted() override;

  void onDebugDumpData(const ::chre::fbs::DebugDumpDataT &data) override;

  void onDebugDumpComplete(
      const ::chre::fbs::DebugDumpResponseT &response) override;

  void handleServiceDeath();
  static void onServiceDied(void *cookie);

 private:
  ::android::hardware::contexthub::common::implementation::
      HalChreSocketConnection mConnection{this};

  // A mutex to protect concurrent modifications to the callback pointer and
  // access (invocations).
  std::mutex mCallbackMutex;
  std::shared_ptr<IContextHubCallback> mCallback;

  ndk::ScopedAIBinder_DeathRecipient mDeathRecipient;
};

}  // namespace contexthub
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // ANDROID_HARDWARE_CONTEXTHUB_AIDL_CONTEXTHUB_H
