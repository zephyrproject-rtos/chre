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

#include "generic_context_hub_aidl.h"

#include <log/log.h>

#include "chre_host/fragmented_load_transaction.h"
#include "chre_host/host_protocol_host.h"
#include "permissions_util.h"

namespace aidl {
namespace android {
namespace hardware {
namespace contexthub {

// Aliased for consistency with the way these symbols are referenced in
// CHRE-side code
namespace fbs = ::chre::fbs;

using ::android::chre::FragmentedLoadTransaction;
using ::android::chre::getStringFromByteVector;
using ::android::hardware::contexthub::common::implementation::
    chreToAndroidPermissions;
using ::android::hardware::contexthub::common::implementation::
    kSupportedPermissions;

namespace {
constexpr uint32_t kDefaultHubId = 0;

inline constexpr uint8_t extractChreApiMajorVersion(uint32_t chreVersion) {
  return static_cast<uint8_t>(chreVersion >> 24);
}

inline constexpr uint8_t extractChreApiMinorVersion(uint32_t chreVersion) {
  return static_cast<uint8_t>(chreVersion >> 16);
}

inline constexpr uint16_t extractChrePatchVersion(uint32_t chreVersion) {
  return static_cast<uint16_t>(chreVersion);
}

// TODO(b/194285834): Implement debug dump
// TODO(b/194285834): Implement service death handling

}  // anonymous namespace

::ndk::ScopedAStatus ContextHub::getContextHubs(
    std::vector<ContextHubInfo> *out_contextHubInfos) {
  ::chre::fbs::HubInfoResponseT response;
  bool success = mConnection.getContextHubs(&response);
  if (success) {
    ContextHubInfo hub;
    hub.name = getStringFromByteVector(response.name);
    hub.vendor = getStringFromByteVector(response.vendor);
    hub.toolchain = getStringFromByteVector(response.toolchain);
    hub.id = kDefaultHubId;
    hub.peakMips = response.peak_mips;
    hub.maxSupportedMessageLengthBytes = response.max_msg_len;
    hub.chrePlatformId = response.platform_id;

    uint32_t version = response.chre_platform_version;
    hub.chreApiMajorVersion = extractChreApiMajorVersion(version);
    hub.chreApiMinorVersion = extractChreApiMinorVersion(version);
    hub.chrePatchVersion = extractChrePatchVersion(version);

    hub.supportedPermissions = kSupportedPermissions;

    out_contextHubInfos->push_back(hub);
  }

  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::loadNanoapp(int32_t contextHubId,
                                             const NanoappBinary &appBinary,
                                             int32_t transactionId,
                                             bool *_aidl_return) {
  bool success = false;
  if (contextHubId != kDefaultHubId) {
    ALOGE("Invalid ID %" PRId32, contextHubId);
  } else {
    uint32_t targetApiVersion = (appBinary.targetChreApiMajorVersion << 24) |
                                (appBinary.targetChreApiMinorVersion << 16);
    FragmentedLoadTransaction transaction(
        transactionId, appBinary.nanoappId, appBinary.nanoappVersion,
        appBinary.flags, targetApiVersion, appBinary.customBinary);
    success = mConnection.loadNanoapp(transaction);
  }

  *_aidl_return = success;

  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::unloadNanoapp(int32_t contextHubId,
                                               int64_t appId,
                                               int32_t transactionId,
                                               bool *_aidl_return) {
  bool success = false;
  if (contextHubId != kDefaultHubId) {
    ALOGE("Invalid ID %" PRId32, contextHubId);
  } else {
    success = mConnection.unloadNanoapp(appId, transactionId);
  }

  *_aidl_return = success;

  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::disableNanoapp(int32_t /* contextHubId */,
                                                int64_t appId,
                                                int32_t /* transactionId */,
                                                bool *_aidl_return) {
  ALOGW("Attempted to disable app ID 0x%016" PRIx64 ", but not supported",
        appId);
  *_aidl_return = false;

  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::enableNanoapp(int32_t /* contextHubId */,
                                               int64_t appId,
                                               int32_t /* transactionId */,
                                               bool *_aidl_return) {
  ALOGW("Attempted to enable app ID 0x%016" PRIx64 ", but not supported",
        appId);
  *_aidl_return = false;

  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::onSettingChanged(Setting /* setting */,
                                                  bool /* enabled */) {
  // TODO(b/194285834): Implement this
  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::queryNanoapps(int32_t contextHubId,
                                               bool *_aidl_return) {
  bool success = false;
  if (contextHubId != kDefaultHubId) {
    ALOGE("Invalid ID %" PRId32, contextHubId);
  } else {
    success = mConnection.queryNanoapps();
  }

  *_aidl_return = success;

  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::registerCallback(
    int32_t contextHubId, const std::shared_ptr<IContextHubCallback> &cb,
    bool *_aidl_return) {
  bool success = false;
  if (contextHubId != kDefaultHubId) {
    ALOGE("Invalid ID %" PRId32, contextHubId);
  } else {
    mCallback = cb;
    success = true;
  }

  *_aidl_return = success;

  return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ContextHub::sendMessageToHub(
    int32_t contextHubId, const ContextHubMessage &message,
    bool *_aidl_return) {
  bool success = false;
  if (contextHubId != kDefaultHubId) {
    ALOGE("Invalid ID %" PRId32, contextHubId);
  } else {
    success = mConnection.sendMessageToHub(
        message.nanoappId, message.messageType, message.hostEndPoint,
        message.messageBody.data(), message.messageBody.size());
  }

  *_aidl_return = success;

  return ndk::ScopedAStatus::ok();
}

void ContextHub::onNanoappMessage(const ::chre::fbs::NanoappMessageT &message) {
  if (mCallback != nullptr) {
    ContextHubMessage outMessage;
    outMessage.nanoappId = message.app_id;
    outMessage.hostEndPoint = message.host_endpoint;
    outMessage.messageType = message.message_type;
    outMessage.messageBody = message.message;
    outMessage.permissions = chreToAndroidPermissions(message.permissions);

    std::vector<std::string> messageContentPerms =
        chreToAndroidPermissions(message.message_permissions);
    mCallback->handleContextHubMessage(outMessage, messageContentPerms);
  }
}

void ContextHub::onNanoappListResponse(
    const ::chre::fbs::NanoappListResponseT &response) {
  if (mCallback != nullptr) {
    std::vector<NanoappInfo> appInfoList;

    for (const std::unique_ptr<::chre::fbs::NanoappListEntryT> &nanoapp :
         response.nanoapps) {
      // TODO: determine if this is really required, and if so, have
      // HostProtocolHost strip out null entries as part of decode
      if (nanoapp == nullptr) {
        continue;
      }

      ALOGV("App 0x%016" PRIx64 " ver 0x%" PRIx32 " permissions 0x%" PRIx32
            " enabled %d system %d",
            nanoapp->app_id, nanoapp->version, nanoapp->permissions,
            nanoapp->enabled, nanoapp->is_system);
      if (!nanoapp->is_system) {
        NanoappInfo appInfo;

        appInfo.nanoappId = nanoapp->app_id;
        appInfo.nanoappVersion = nanoapp->version;
        appInfo.enabled = nanoapp->enabled;
        appInfo.permissions = chreToAndroidPermissions(nanoapp->permissions);

        appInfoList.push_back(appInfo);
      }
    }

    mCallback->handleNanoappInfo(appInfoList);
  }
}

void ContextHub::onTransactionResult(uint32_t transactionId, bool success) {
  if (mCallback != nullptr) {
    mCallback->handleTransactionResult(transactionId, success);
  }
}

void ContextHub::onContextHubRestarted() {
  if (mCallback != nullptr) {
    mCallback->handleContextHubAsyncEvent(AsyncEventType::RESTARTED);
  }
}

void ContextHub::onDebugDumpData(
    const ::chre::fbs::DebugDumpDataT & /* data */) {}

void ContextHub::onDebugDumpComplete(
    const ::chre::fbs::DebugDumpResponseT & /* response */) {}

}  // namespace contexthub
}  // namespace hardware
}  // namespace android
}  // namespace aidl
