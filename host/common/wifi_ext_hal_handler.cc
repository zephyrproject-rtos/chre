/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "chre_host/wifi_ext_hal_handler.h"

#ifdef WIFI_EXT_V_1_3_HAS_MERGED

namespace android {
namespace chre {

WifiExtHalHandler::~WifiExtHalHandler() {
  wifiExtHandlerThreadNotifyToExit();
  mThread.join();
}

WifiExtHalHandler::WifiExtHalHandler() {
  mEnableConfig.reset();
  mThread = std::thread(&WifiExtHalHandler::wifiExtHandlerThreadEntry, this);
  auto cb = [&]() { onWifiExtHalServiceDeath(); };
  mDeathRecipient = new WifiExtHalDeathRecipient(cb);
}

void WifiExtHalHandler::init(
    const std::function<void(bool)> &statusChangeCallback) {
  mStatusChangeCallback = std::move(statusChangeCallback);
}

void WifiExtHalHandler::handleConfigurationRequest(bool enable) {
  std::lock_guard<std::mutex> lock(mMutex);
  mEnableConfig = enable;
  mCondVar.notify_one();
}

void WifiExtHalHandler::dispatchConfigurationRequest(bool enable) {
  auto hidlCb = [this, enable](const WifiStatus &status) {
    bool success = (status.code == WifiStatusCode::SUCCESS) ? true : false;
    if (!success) {
      LOGE("wifi ext hal config request for %s failed with code: %d (%s)",
           (enable == true) ? "Enable" : "Disable", status.code,
           status.description.c_str());
    }
    onStatusChanged(success);
  };

  if (checkWifiExtHalConnected()) {
    hardware::Return<void> result;
    if (enable) {
      // The transaction ID is inconsequential from CHRE's perspective, and is
      // an unimplemented artifact in the WiFi ext HAL.
      result = mService->enableWifiChreNan(0 /*transaction_id*/, hidlCb);
    } else {
      result = mService->disableWifiChreNan(0 /*transaction_id*/, hidlCb);
    }
    if (!result.isOk()) {
      LOGE("Failed to %s NAN: %s", (enable == true) ? "Enable" : "Disable",
           result.description().c_str());
    }
  }
}

bool WifiExtHalHandler::checkWifiExtHalConnected() {
  bool success = true;
  if (mService == nullptr) {
    mService = IWifiExt::getService();
    if (mService != nullptr) {
      LOGD("Connected to Wifi Ext HAL service");
      mService->linkToDeath(mDeathRecipient, 0 /*cookie*/);
    } else {
      LOGE("Failed to connect to Wifi Ext HAL service");
      success = false;
    }
  }
  return success;
}

void WifiExtHalHandler::onWifiExtHalServiceDeath() {
  LOGI("WiFi Ext HAL service died");
  mService = nullptr;
  // TODO(b/204226580): Figure out if wifi ext HAL is stateful and if it
  // isn't, notify CHRE of a NAN disabled status change to enable nanoapps
  // to not expect NAN data until the service is back up, and expect it to
  // do a re-enable when needed. Or we could store the current status of
  // enablement, and do a re-enable/disable when the service is back up.
}

void WifiExtHalHandler::wifiExtHandlerThreadEntry() {
  while (mThreadRunning) {
    std::unique_lock<std::mutex> lock(mMutex);
    mCondVar.wait(
        lock, [this] { return mEnableConfig.has_value() || !mThreadRunning; });

    if (mThreadRunning) {
      dispatchConfigurationRequest(mEnableConfig.value());
      mEnableConfig.reset();
    }
  }
}

void WifiExtHalHandler::wifiExtHandlerThreadNotifyToExit() {
  std::lock_guard<std::mutex> lock(mMutex);
  mThreadRunning = false;
  mCondVar.notify_one();
}

}  // namespace chre
}  // namespace android

#endif  // WIFI_EXT_V_1_3_HAS_MERGED
