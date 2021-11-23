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

#define LOG_TAG "ContextHubHal"
#define LOG_NDEBUG 1

#include "hal_chre_socket_connection.h"

#include <log/log.h>

namespace android {
namespace hardware {
namespace contexthub {
namespace common {
namespace implementation {

using chre::FragmentedLoadRequest;
using chre::FragmentedLoadTransaction;
using chre::HostProtocolHost;
using flatbuffers::FlatBufferBuilder;

HalChreSocketConnection::HalChreSocketConnection(
    IChreSocketCallback *callback) {
  constexpr char kChreSocketName[] = "chre";

  if (!mClient.connectInBackground(kChreSocketName, this)) {
    ALOGE("Couldn't start socket client");
  }

  mCallback = callback;
}

bool HalChreSocketConnection::getContextHubs(
    ::chre::fbs::HubInfoResponseT *response) {
  constexpr auto kHubInfoQueryTimeout = std::chrono::seconds(5);
  ALOGV("%s", __func__);

  // If we're not connected yet, give it some time
  // TODO refactor from polling into conditional wait
  int maxSleepIterations = 250;
  while (!mHubInfoValid && !mClient.isConnected() && --maxSleepIterations > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  if (!mClient.isConnected()) {
    ALOGE("Couldn't connect to hub daemon");
  } else if (!mHubInfoValid) {
    // We haven't cached the hub details yet, so send a request and block
    // waiting on a response
    std::unique_lock<std::mutex> lock(mHubInfoMutex);
    FlatBufferBuilder builder;
    HostProtocolHost::encodeHubInfoRequest(builder);

    ALOGD("Sending hub info request");
    if (!mClient.sendMessage(builder.GetBufferPointer(), builder.GetSize())) {
      ALOGE("Couldn't send hub info request");
    } else {
      mHubInfoCond.wait_for(lock, kHubInfoQueryTimeout,
                            [this]() { return mHubInfoValid; });
    }
  }

  if (mHubInfoValid) {
    *response = mHubInfoResponse;
  } else {
    ALOGE("Unable to get hub info from CHRE");
  }

  return mHubInfoValid;
}

bool HalChreSocketConnection::sendMessageToHub(long nanoappId,
                                               uint32_t messageType,
                                               uint16_t hostEndpointId,
                                               const unsigned char *payload,
                                               size_t payloadLength) {
  FlatBufferBuilder builder(1024);
  HostProtocolHost::encodeNanoappMessage(
      builder, nanoappId, messageType, hostEndpointId, payload, payloadLength);
  return mClient.sendMessage(builder.GetBufferPointer(), builder.GetSize());
}

bool HalChreSocketConnection::loadNanoapp(
    FragmentedLoadTransaction &transaction) {
  bool success = false;
  std::lock_guard<std::mutex> lock(mPendingLoadTransactionMutex);

  if (mPendingLoadTransaction.has_value()) {
    ALOGE("Pending load transaction exists. Overriding pending request");
  }

  mPendingLoadTransaction = transaction;
  success = sendFragmentedLoadNanoAppRequest(mPendingLoadTransaction.value());
  if (!success) {
    mPendingLoadTransaction.reset();
  }

  return success;
}

bool HalChreSocketConnection::unloadNanoapp(uint64_t appId,
                                            uint32_t transactionId) {
  FlatBufferBuilder builder(64);
  HostProtocolHost::encodeUnloadNanoappRequest(
      builder, transactionId, appId, false /* allowSystemNanoappUnload */);
  return mClient.sendMessage(builder.GetBufferPointer(), builder.GetSize());
}

bool HalChreSocketConnection::queryNanoapps() {
  FlatBufferBuilder builder(64);
  HostProtocolHost::encodeNanoappListRequest(builder);
  return mClient.sendMessage(builder.GetBufferPointer(), builder.GetSize());
}

bool HalChreSocketConnection::requestDebugDump() {
  FlatBufferBuilder builder;
  HostProtocolHost::encodeDebugDumpRequest(builder);
  return mClient.sendMessage(builder.GetBufferPointer(), builder.GetSize());
}

bool HalChreSocketConnection::sendSettingChangedNotification(
    ::chre::fbs::Setting fbsSetting, ::chre::fbs::SettingState fbsState) {
  FlatBufferBuilder builder(64);
  HostProtocolHost::encodeSettingChangeNotification(builder, fbsSetting,
                                                    fbsState);
  return mClient.sendMessage(builder.GetBufferPointer(), builder.GetSize());
}

bool HalChreSocketConnection::onHostEndpointConnected(uint16_t hostEndpointId) {
  FlatBufferBuilder builder(64);
  HostProtocolHost::encodeHostEndpointConnected(builder, hostEndpointId);
  return mClient.sendMessage(builder.GetBufferPointer(), builder.GetSize());
}

bool HalChreSocketConnection::onHostEndpointDisconnected(
    uint16_t hostEndpointId) {
  FlatBufferBuilder builder(64);
  HostProtocolHost::encodeHostEndpointDisconnected(builder, hostEndpointId);
  return mClient.sendMessage(builder.GetBufferPointer(), builder.GetSize());
}

void HalChreSocketConnection::onMessageReceived(const void *data,
                                                size_t length) {
  if (!chre::HostProtocolHost::decodeMessageFromChre(data, length, *this)) {
    ALOGE("Failed to decode message");
  }
}

void HalChreSocketConnection::onConnected() {
  ALOGI("Reconnected to CHRE daemon");
  if (mHaveConnected) {
    ALOGI("Reconnected to CHRE daemon");
    mCallback->onContextHubRestarted();
  }
  mHaveConnected = true;
}

void HalChreSocketConnection::onDisconnected() {
  ALOGW("Lost connection to CHRE daemon");
}

void HalChreSocketConnection::handleNanoappMessage(
    const ::chre::fbs::NanoappMessageT &message) {
  ALOGD("Got message from nanoapp: ID 0x%" PRIx64, message.app_id);
  mCallback->onNanoappMessage(message);
}

void HalChreSocketConnection::handleHubInfoResponse(
    const ::chre::fbs::HubInfoResponseT &response) {
  ALOGD("Got hub info response");

  std::lock_guard<std::mutex> lock(mHubInfoMutex);
  if (mHubInfoValid) {
    ALOGI("Ignoring duplicate/unsolicited hub info response");
  } else {
    mHubInfoResponse = response;
    mHubInfoValid = true;
    mHubInfoCond.notify_all();
  }
}

void HalChreSocketConnection::handleNanoappListResponse(
    const ::chre::fbs::NanoappListResponseT &response) {
  ALOGD("Got nanoapp list response with %zu apps", response.nanoapps.size());
  mCallback->onNanoappListResponse(response);
}

void HalChreSocketConnection::handleLoadNanoappResponse(
    const ::chre::fbs::LoadNanoappResponseT &response) {
  ALOGD("Got load nanoapp response for transaction %" PRIu32
        " fragment %" PRIu32 " with result %d",
        response.transaction_id, response.fragment_id, response.success);
  std::unique_lock<std::mutex> lock(mPendingLoadTransactionMutex);

  // TODO: Handle timeout in receiving load response
  if (!mPendingLoadTransaction.has_value()) {
    ALOGE(
        "Dropping unexpected load response (no pending transaction "
        "exists)");
  } else {
    FragmentedLoadTransaction &transaction = mPendingLoadTransaction.value();

    if (!isExpectedLoadResponseLocked(response)) {
      ALOGE("Dropping unexpected load response, expected transaction %" PRIu32
            " fragment %" PRIu32 ", received transaction %" PRIu32
            " fragment %" PRIu32,
            transaction.getTransactionId(), mCurrentFragmentId,
            response.transaction_id, response.fragment_id);
    } else {
      bool success = false;
      bool continueLoadRequest = false;
      if (response.success && !transaction.isComplete()) {
        if (sendFragmentedLoadNanoAppRequest(transaction)) {
          continueLoadRequest = true;
          success = true;
        }
      } else {
        success = response.success;
      }

      if (!continueLoadRequest) {
        mPendingLoadTransaction.reset();
        lock.unlock();
        mCallback->onTransactionResult(response.transaction_id, success);
      }
    }
  }
}

void HalChreSocketConnection::handleUnloadNanoappResponse(
    const ::chre::fbs::UnloadNanoappResponseT &response) {
  ALOGV("Got unload nanoapp response for transaction %" PRIu32
        " with result %d",
        response.transaction_id, response.success);
  mCallback->onTransactionResult(response.transaction_id, response.success);
}

void HalChreSocketConnection::handleDebugDumpData(
    const ::chre::fbs::DebugDumpDataT &data) {
  ALOGV("Got debug dump data, size %zu", data.debug_str.size());
  mCallback->onDebugDumpData(data);
}

void HalChreSocketConnection::handleDebugDumpResponse(
    const ::chre::fbs::DebugDumpResponseT &response) {
  ALOGV("Got debug dump response, success %d, data count %" PRIu32,
        response.success, response.data_count);
  mCallback->onDebugDumpComplete(response);
}

bool HalChreSocketConnection::isExpectedLoadResponseLocked(
    const ::chre::fbs::LoadNanoappResponseT &response) {
  return mPendingLoadTransaction.has_value() &&
         (mPendingLoadTransaction->getTransactionId() ==
          response.transaction_id) &&
         (response.fragment_id == 0 ||
          mCurrentFragmentId == response.fragment_id);
}

bool HalChreSocketConnection::sendFragmentedLoadNanoAppRequest(
    FragmentedLoadTransaction &transaction) {
  bool success = false;
  const FragmentedLoadRequest &request = transaction.getNextRequest();

  FlatBufferBuilder builder(128 + request.binary.size());
  HostProtocolHost::encodeFragmentedLoadNanoappRequest(builder, request);

  if (!mClient.sendMessage(builder.GetBufferPointer(), builder.GetSize())) {
    ALOGE("Failed to send load request message (fragment ID = %zu)",
          request.fragmentId);
  } else {
    mCurrentFragmentId = request.fragmentId;
    success = true;
  }

  return success;
}

}  // namespace implementation
}  // namespace common
}  // namespace contexthub
}  // namespace hardware
}  // namespace android
