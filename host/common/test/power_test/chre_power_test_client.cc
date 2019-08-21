/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <cutils/sockets.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <utils/StrongPointer.h>

#include <chrono>
#include <cinttypes>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "chre/util/nanoapp/app_id.h"
#include "chre/version.h"
#include "chre_host/host_protocol_host.h"
#include "chre_host/log.h"
#include "chre_host/socket_client.h"

/**
 * @file
 * A test utility that connects to the CHRE daemon and provides commands to take
 * control the power test nanoapp located at system/chre/apps/power_test.
 *
 * Usage:
 *  chre_power_test_client load <optional: tcm>
 *  chre_power_test_client unload <optional: tcm>
 *  chre_power_test_client unloadall
 */

using android::sp;
using android::chre::FragmentedLoadTransaction;
using android::chre::getStringFromByteVector;
using android::chre::HostProtocolHost;
using android::chre::IChreMessageHandlers;
using android::chre::SocketClient;
using flatbuffers::FlatBufferBuilder;

// Aliased for consistency with the way these symbols are referenced in
// CHRE-side code
namespace fbs = ::chre::fbs;

namespace {

constexpr uint32_t kAppVersion = 1;
constexpr uint32_t kApiVersion = CHRE_API_VERSION;
constexpr uint64_t kPowerTestAppId = 0x012345678900000f;
constexpr uint64_t kPowerTestTcmAppId = 0x0123456789000010;

constexpr auto kTimeout = std::chrono::seconds(10);

const char *kPowerTestPath = "/system/lib64/power_test.so";
const char *kPowerTestTcmPath = "/system/lib64/power_test_tcm.so";
std::condition_variable kReadyCond;
std::mutex kReadyMutex;
std::unique_lock<std::mutex> kReadyCondLock(kReadyMutex);

enum class Command : uint32_t { kUnloadAll = 0, kLoad, kUnload };

std::unordered_map<std::string, Command> commandMap{
    {"unloadall", Command::kUnloadAll},
    {"load", Command::kLoad},
    {"unload", Command::kUnload}};

class SocketCallbacks : public SocketClient::ICallbacks,
                        public IChreMessageHandlers {
 public:
  SocketCallbacks(std::condition_variable &readyCond)
      : mConditionVariable(readyCond) {}

  void onMessageReceived(const void *data, size_t length) override {
    if (!HostProtocolHost::decodeMessageFromChre(data, length, *this)) {
      LOGE("Failed to decode message");
    }
  }

  void onConnected() override {
    LOGI("Socket (re)connected");
  }

  void onConnectionAborted() override {
    LOGI("Socket (re)connection aborted");
  }

  void onDisconnected() override {
    LOGI("Socket disconnected");
  }

  void handleNanoappMessage(const fbs::NanoappMessageT &message) override {
    LOGI("Got message from nanoapp 0x%" PRIx64 " to endpoint 0x%" PRIx16
         " with type 0x%" PRIx32 " and length %zu",
         message.app_id, message.host_endpoint, message.message_type,
         message.message.size());
  }

  void handleNanoappListResponse(
      const fbs::NanoappListResponseT &response) override {
    LOGI("Got nanoapp list response with %zu apps:", response.nanoapps.size());
    mAppIdVector.clear();
    for (const auto &nanoapp : response.nanoapps) {
      LOGI("  App ID 0x%016" PRIx64 " version 0x%" PRIx32
           " enabled %d system "
           "%d",
           nanoapp->app_id, nanoapp->version, nanoapp->enabled,
           nanoapp->is_system);
      mAppIdVector.push_back(nanoapp->app_id);
    }
    mConditionVariable.notify_all();
  }

  void handleLoadNanoappResponse(
      const fbs::LoadNanoappResponseT &response) override {
    LOGI("Got load nanoapp response, transaction ID 0x%" PRIx32 " result %d",
         response.transaction_id, response.success);
    mSuccess = response.success;
    mConditionVariable.notify_all();
  }

  void handleUnloadNanoappResponse(
      const fbs::UnloadNanoappResponseT &response) override {
    LOGI("Got unload nanoapp response, transaction ID 0x%" PRIx32 " result %d",
         response.transaction_id, response.success);
    mSuccess = response.success;
    mConditionVariable.notify_all();
  }

  bool actionSucceeded() {
    return mSuccess;
  }

  std::vector<uint64_t> &getAppIdVector() {
    return mAppIdVector;
  }

 private:
  bool mSuccess = false;
  std::condition_variable &mConditionVariable;
  std::vector<uint64_t> mAppIdVector;
};

bool requestNanoappList(SocketClient &client) {
  FlatBufferBuilder builder(64);
  HostProtocolHost::encodeNanoappListRequest(builder);

  LOGI("Sending app list request (%" PRIu32 " bytes)", builder.GetSize());
  if (!client.sendMessage(builder.GetBufferPointer(), builder.GetSize())) {
    LOGE("Failed to send message");
    return false;
  }
  return true;
}

bool sendLoadNanoappRequest(SocketClient &client, const char *filename,
                            uint64_t appId, uint32_t appVersion,
                            uint32_t apiVersion) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file) {
    LOGE("Couldn't open file '%s': %s", filename, strerror(errno));
    return false;
  }
  ssize_t size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> buffer(size);
  if (!file.read(reinterpret_cast<char *>(buffer.data()), size)) {
    LOGE("Couldn't read from file: %s", strerror(errno));
    file.close();
    return false;
  }

  // Perform loading with 1 fragment for simplicity
  FlatBufferBuilder builder(size + 128);
  FragmentedLoadTransaction transaction = FragmentedLoadTransaction(
      1 /* transactionId */, appId, appVersion, apiVersion, buffer,
      buffer.size() /* fragmentSize */);
  HostProtocolHost::encodeFragmentedLoadNanoappRequest(
      builder, transaction.getNextRequest());
  LOGI("Sending load nanoapp request (%" PRIu32
       " bytes total w/ %zu bytes of "
       "payload)",
       builder.GetSize(), buffer.size());
  bool success =
      client.sendMessage(builder.GetBufferPointer(), builder.GetSize());
  if (!success) {
    LOGE("Failed to send message");
  }
  file.close();
  return success;
}

bool loadNanoapp(SocketClient &client, sp<SocketCallbacks> callbacks,
                 const char *filename, uint64_t appId, uint32_t appVersion,
                 uint32_t apiVersion) {
  if (!sendLoadNanoappRequest(client, filename, appId, appVersion,
                              apiVersion)) {
    return false;
  }
  auto status = kReadyCond.wait_for(kReadyCondLock, kTimeout);
  bool success =
      (status == std::cv_status::no_timeout && callbacks->actionSucceeded());
  LOGI("Loading the nanoapp with appId: %" PRIx64 " success: %d", appId,
       success);
  return success;
}

bool sendUnloadNanoappRequest(SocketClient &client, uint64_t appId) {
  FlatBufferBuilder builder(64);
  constexpr uint32_t kTransactionId = 4321;
  HostProtocolHost::encodeUnloadNanoappRequest(
      builder, kTransactionId, appId, true /* allowSystemNanoappUnload */);

  LOGI("Sending unload request for nanoapp 0x%016" PRIx64 " (size %" PRIu32 ")",
       appId, builder.GetSize());
  if (!client.sendMessage(builder.GetBufferPointer(), builder.GetSize())) {
    LOGE("Failed to send message");
    return false;
  }
  return true;
}

bool unloadNanoapp(SocketClient &client, sp<SocketCallbacks> callbacks,
                   uint64_t appId) {
  if (!sendUnloadNanoappRequest(client, appId)) {
    return false;
  }
  auto status = kReadyCond.wait_for(kReadyCondLock, kTimeout);
  bool success =
      (status == std::cv_status::no_timeout && callbacks->actionSucceeded());
  LOGI("Unloading the nanoapp with appId: %" PRIx64 " success: %d", appId,
       success);
  return success;
}

bool listNanoapps(SocketClient &client) {
  if (!requestNanoappList(client)) {
    LOGE("Failed in listing nanoapps");
    return false;
  }
  auto status = kReadyCond.wait_for(kReadyCondLock, kTimeout);
  bool success = (status == std::cv_status::no_timeout);
  LOGI("Listed nanoapps success %d", success);
  return success;
}

bool unloadAllNanoapps(SocketClient &client, sp<SocketCallbacks> callbacks) {
  std::vector<uint64_t> appIds;
  if (!listNanoapps(client)) {
    return false;
  }
  for (auto appId : callbacks->getAppIdVector()) {
    if (!unloadNanoapp(client, callbacks, appId)) {
      LOGE("Failed in unloading nanoapps, unloading aborted");
      return false;
    }
  }
  LOGI("Unloading all nanoapps succeeded");
  return true;
}

uint64_t getId(std::vector<std::string> args) {
  if (!args.empty() && args[0] == "tcm") {
    return kPowerTestTcmAppId;
  }
  return kPowerTestAppId;
}

const char *getPath(std::vector<std::string> args) {
  if (!args.empty() && args[0] == "tcm") {
    return kPowerTestTcmPath;
  }
  return kPowerTestPath;
}

static void usage() {
  const char *name = "chre_power_test_client";
  LOGI(
      "\n"
      "Usage:\n"
      " %s load <optional: tcm>\n"
      " %s unload <optional: tcm>\n"
      " %s unloadall\n",
      name, name, name);
}

}  // anonymous namespace

int main(int argc, char *argv[]) {
  int argi = 0;
  const std::string name{argv[argi++]};
  const std::string cmd{argi < argc ? argv[argi++] : ""};

  if (commandMap.find(cmd) == commandMap.end()) {
    usage();
    LOGE("No command");
    return -1;
  }

  std::vector<std::string> args;
  while (argi < argc) {
    args.push_back(std::string(argv[argi++]));
  }

  SocketClient client;
  sp<SocketCallbacks> callbacks = new SocketCallbacks(kReadyCond);

  if (!client.connect("chre", callbacks)) {
    LOGE("Couldn't connect to socket");
    return -1;
  }

  Command commandEnum = commandMap[cmd];
  bool success = false;
  switch (commandEnum) {
    case Command::kUnloadAll: {
      success = unloadAllNanoapps(client, callbacks);
      break;
    }
    case Command::kUnload: {
      success = unloadNanoapp(client, callbacks, getId(args));
      break;
    }
    case Command::kLoad: {
      success = loadNanoapp(client, callbacks, getPath(args), getId(args),
                            kAppVersion, kApiVersion);
      break;
    }
    default: {
      usage();
      LOGE("Unknown command");
    }
  }

  client.disconnect();
  return success ? 0 : -1;
}