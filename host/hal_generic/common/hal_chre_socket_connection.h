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

#ifndef ANDROID_HARDWARE_CONTEXTHUB_COMMON_CHRE_SOCKET_H
#define ANDROID_HARDWARE_CONTEXTHUB_COMMON_CHRE_SOCKET_H

#include <condition_variable>
#include <mutex>

#include "chre_host/fragmented_load_transaction.h"
#include "chre_host/host_protocol_host.h"
#include "chre_host/socket_client.h"

namespace android {
namespace hardware {
namespace contexthub {
namespace common {
namespace implementation {

/**
 * Callback interface to be used for
 * HalChreSocketConnection::registerCallback().
 */
class IChreSocketCallback {
 public:
  virtual ~IChreSocketCallback() {}

  /**
   * Invoked when a transaction completed
   *
   * @param transactionId The ID of the transaction.
   * @param success true if the transaction succeeded.
   */
  virtual void onTransactionResult(uint32_t transactionId, bool success) = 0;

  /**
   * Invoked when a nanoapp sends a message to this socket client.
   *
   * @param message The message.
   */
  virtual void onNanoappMessage(
      const ::chre::fbs::NanoappMessageT &message) = 0;

  /**
   * Invoked to provide a list of nanoapps previously requested by
   * HalChreSocketConnection::queryNanoapps().
   *
   * @param response The list response.
   */
  virtual void onNanoappListResponse(
      const ::chre::fbs::NanoappListResponseT &response) = 0;

  /**
   * Invoked when CHRE restarts.
   */
  virtual void onContextHubRestarted() = 0;

  /**
   * Invoked when a data is available as a result of a debug dump request
   * through HalChreSocketConnection::requestDebugDump().
   *
   * @param data The debug dump data.
   */
  virtual void onDebugDumpData(const ::chre::fbs::DebugDumpDataT &data) = 0;

  /**
   * Invoked when a debug dump is completed.
   *
   * @param response The debug dump response.
   */
  virtual void onDebugDumpComplete(
      const ::chre::fbs::DebugDumpResponseT &response) = 0;
};

/**
 * A helper class that can be used to connect to the CHRE socket.
 */
class HalChreSocketConnection
    : public ::android::chre::SocketClient::ICallbacks,
      public ::android::chre::IChreMessageHandlers {
 public:
  HalChreSocketConnection(IChreSocketCallback *callback);

  bool getContextHubs(::chre::fbs::HubInfoResponseT *response);

  bool sendMessageToHub(long nanoappId, uint32_t messageType,
                        uint16_t hostEndpointId, const unsigned char *payload,
                        size_t payloadLength);

  bool loadNanoapp(chre::FragmentedLoadTransaction &transaction);

  bool unloadNanoapp(uint64_t appId, uint32_t transactionId);

  bool queryNanoapps();

  bool requestDebugDump();

  bool sendSettingChangedNotification(::chre::fbs::Setting fbsSetting,
                                      ::chre::fbs::SettingState fbsState);

  bool onHostEndpointConnected(uint16_t hostEndpointId, uint8_t type,
                               const std::string &package_name,
                               const std::string &attribution_tag);

  bool onHostEndpointDisconnected(uint16_t hostEndpointId);

  void onMessageReceived(const void *data, size_t length) override;
  void onConnected() override;
  void onDisconnected() override;
  void handleNanoappMessage(
      const ::chre::fbs::NanoappMessageT &message) override;
  void handleHubInfoResponse(
      const ::chre::fbs::HubInfoResponseT &response) override;
  void handleNanoappListResponse(
      const ::chre::fbs::NanoappListResponseT &response) override;
  void handleLoadNanoappResponse(
      const ::chre::fbs::LoadNanoappResponseT &response) override;
  void handleUnloadNanoappResponse(
      const ::chre::fbs::UnloadNanoappResponseT &response) override;
  void handleDebugDumpData(const ::chre::fbs::DebugDumpDataT &data) override;
  void handleDebugDumpResponse(
      const ::chre::fbs::DebugDumpResponseT &response) override;

 private:
  ::android::chre::SocketClient mClient;

  IChreSocketCallback *mCallback = nullptr;

  ::chre::fbs::HubInfoResponseT mHubInfoResponse;
  bool mHubInfoValid = false;
  std::mutex mHubInfoMutex;
  std::condition_variable mHubInfoCond;

  // The pending fragmented load request
  uint32_t mCurrentFragmentId = 0;
  std::optional<chre::FragmentedLoadTransaction> mPendingLoadTransaction;
  std::mutex mPendingLoadTransactionMutex;

  bool mHaveConnected = false;

  /**
   * Checks to see if a load response matches the currently pending
   * fragmented load transaction. mPendingLoadTransactionMutex must
   * be acquired prior to calling this function.
   *
   * @param response the received load response
   *
   * @return true if the response matches a pending load transaction
   *         (if any), false otherwise
   */
  bool isExpectedLoadResponseLocked(
      const ::chre::fbs::LoadNanoappResponseT &response);

  /**
   * Sends a fragmented load request to CHRE. The caller must ensure that
   * transaction.isComplete() returns false prior to invoking this method.
   *
   * @param transaction the FragmentedLoadTransaction object
   *
   * @return true if the load succeeded
   */
  bool sendFragmentedLoadNanoAppRequest(
      chre::FragmentedLoadTransaction &transaction);
};

}  // namespace implementation
}  // namespace common
}  // namespace contexthub
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CONTEXTHUB_COMMON_CHRE_SOCKET_H
