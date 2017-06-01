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

#include "chre/platform/platform_nanoapp.h"

#include "chre/platform/assert.h"
#include "chre/platform/log.h"
#include "chre/platform/memory.h"
#include "chre/platform/shared/nanoapp_support_lib_dso.h"
#include "chre_api/chre/version.h"

#include "dlfcn.h"

#include <inttypes.h>
#include <string.h>

namespace chre {

namespace {

/**
 * Performs sanity checks on the app info structure included in a dynamically
 * loaded nanoapp.
 *
 * @param expectedAppId The app ID passed alongside the binary
 * @param expectedAppVersion The app version number passed alongside the binary
 * @param appInfo App info structure included in the nanoapp binary
 * @param skipVersionValidation if true, ignore the expectedAppVersion parameter
 *
 * @return true if validation was successful
 */
bool validateAppInfo(uint64_t expectedAppId, uint32_t expectedAppVersion,
                     const struct chreNslNanoappInfo *appInfo,
                     bool skipVersionValidation = false) {
  uint32_t ourApiMajorVersion = CHRE_EXTRACT_MAJOR_VERSION(chreGetApiVersion());
  uint32_t targetApiMajorVersion = CHRE_EXTRACT_MAJOR_VERSION(
      appInfo->targetApiVersion);

  bool success = false;
  if (appInfo->magic != CHRE_NSL_NANOAPP_INFO_MAGIC) {
    LOGE("Invalid app info magic: got 0x%08" PRIx32 " expected 0x%08" PRIx32,
         appInfo->magic, static_cast<uint32_t>(CHRE_NSL_NANOAPP_INFO_MAGIC));
  } else if (appInfo->appId == 0) {
    LOGE("Rejecting invalid app ID 0");
  } else if (expectedAppId != appInfo->appId) {
    LOGE("Expected app ID (0x%016" PRIx64 ") doesn't match internal one (0x%016"
         PRIx64 ")", expectedAppId, appInfo->appId);
  } else if (!skipVersionValidation
      && expectedAppVersion != appInfo->appVersion) {
    LOGE("Expected app version (0x%" PRIx32 ") doesn't match internal one (0x%"
         PRIx32 ")", expectedAppVersion, appInfo->appVersion);
  } else if (targetApiMajorVersion != ourApiMajorVersion) {
    LOGE("App targets a different major API version (%" PRIu32 ") than what we "
         "provide (%" PRIu32 ")", targetApiMajorVersion, ourApiMajorVersion);
  } else if (strlen(appInfo->name) > CHRE_NSL_DSO_NANOAPP_STRING_MAX_LEN) {
    LOGE("App name is too long");
  } else if (strlen(appInfo->vendor) > CHRE_NSL_DSO_NANOAPP_STRING_MAX_LEN) {
    LOGE("App vendor is too long");
  } else {
    success = true;
  }

  return success;
}

}  // anonymous namespace

PlatformNanoapp::~PlatformNanoapp() {
  closeNanoapp();
  if (mAppBinary != nullptr) {
    memoryFree(mAppBinary);
  }
}

bool PlatformNanoapp::start() {
  // Invoke the start entry point after successfully opening the app
  return openNanoapp() ? mAppInfo->entryPoints.start() : false;
}

void PlatformNanoapp::handleEvent(uint32_t senderInstanceId,
                                  uint16_t eventType,
                                  const void *eventData) {
  mAppInfo->entryPoints.handleEvent(senderInstanceId, eventType, eventData);
}

void PlatformNanoapp::end() {
  mAppInfo->entryPoints.end();
  closeNanoapp();
}

bool PlatformNanoappBase::loadFromBuffer(uint64_t appId, uint32_t appVersion,
                                         const void *appBinary,
                                         size_t appBinaryLen) {
  CHRE_ASSERT(!isLoaded());
  bool success = false;
  constexpr size_t kMaxAppSize = 2 * 1024 * 1024;  // 2 MiB

  if (appBinaryLen > kMaxAppSize) {
    LOGE("Rejecting app size %zu above limit %zu", appBinaryLen, kMaxAppSize);
  } else {
    mAppBinary = memoryAlloc(appBinaryLen);
    if (mAppBinary == nullptr) {
      LOGE("Couldn't allocate %zu byte buffer for nanoapp 0x%016" PRIx64,
           appBinaryLen, appId);
    } else {
      mExpectedAppId = appId;
      mExpectedAppVersion = appVersion;
      mAppBinaryLen = appBinaryLen;
      memcpy(mAppBinary, appBinary, appBinaryLen);
      success = true;
    }
  }

  return success;
}

void PlatformNanoappBase::loadFromFile(uint64_t appId, const char *filename) {
  CHRE_ASSERT(!isLoaded());
  mExpectedAppId = appId;
  mFilename = filename;
}

void PlatformNanoappBase::loadStatic(const struct chreNslNanoappInfo *appInfo) {
  CHRE_ASSERT(!isLoaded());
  mIsStatic = true;
  mAppInfo = appInfo;
}

bool PlatformNanoappBase::isLoaded() const {
  return (mIsStatic || mAppBinary != nullptr);
}

void PlatformNanoappBase::closeNanoapp() {
  if (mDsoHandle != nullptr) {
    mAppInfo = nullptr;
    if (dlclose(mDsoHandle) != 0) {
      const char *name = (mAppInfo != nullptr) ? mAppInfo->name : "unknown";
      LOGE("dlclose of %s failed: %s", name, dlerror());
    }
    mDsoHandle = nullptr;
  }
}

bool PlatformNanoappBase::openNanoapp() {
  bool success = false;

  if (mIsStatic) {
    success = true;
  } else if (mFilename != nullptr) {
    success = openNanoappFromFile();
  } else if (mAppBinary != nullptr) {
    success = openNanoappFromBuffer();
  } else {
    CHRE_ASSERT(false);
  }

  return success;
}

bool PlatformNanoappBase::openNanoappFromBuffer() {
  CHRE_ASSERT(mAppBinary != nullptr);
  CHRE_ASSERT_LOG(mDsoHandle == nullptr, "Re-opening nanoapp");
  bool success = false;

  // Populate a filename string (just a requirement of the dlopenbuf API)
  constexpr size_t kMaxFilenameLen = 17;
  char filename[kMaxFilenameLen];
  snprintf(filename, sizeof(filename), "%016" PRIx64, mExpectedAppId);

  mDsoHandle = dlopenbuf(
      filename, static_cast<const char *>(mAppBinary),
      static_cast<int>(mAppBinaryLen), RTLD_NOW);
  if (mDsoHandle == nullptr) {
    LOGE("Failed to load nanoapp: %s", dlerror());
  } else {
    mAppInfo = static_cast<const struct chreNslNanoappInfo *>(
        dlsym(mDsoHandle, CHRE_NSL_DSO_NANOAPP_INFO_SYMBOL_NAME));
    if (mAppInfo == nullptr) {
      LOGE("Failed to find app info symbol: %s", dlerror());
    } else {
      success = validateAppInfo(mExpectedAppId, mExpectedAppVersion, mAppInfo);
      if (!success) {
        mAppInfo = nullptr;
      } else {
        LOGI("Successfully loaded nanoapp: %s (0x%016" PRIx64 ") version 0x%"
             PRIx32, mAppInfo->name, mAppInfo->appId, mAppInfo->appVersion);
      }
    }
  }

  return success;
}

bool PlatformNanoappBase::openNanoappFromFile() {
  CHRE_ASSERT(mFilename != nullptr);
  CHRE_ASSERT_LOG(mDsoHandle == nullptr, "Re-opening nanoapp");
  bool success = false;

  mDsoHandle = dlopen(mFilename, RTLD_NOW);
  if (mDsoHandle == nullptr) {
    LOGE("Failed to load nanoapp from file %s: %s", mFilename, dlerror());
  } else {
    mAppInfo = static_cast<const struct chreNslNanoappInfo *>(
        dlsym(mDsoHandle, CHRE_NSL_DSO_NANOAPP_INFO_SYMBOL_NAME));
    if (mAppInfo == nullptr) {
      LOGE("Failed to find app info symbol in %s: %s", mFilename, dlerror());
    } else {
      success = validateAppInfo(mExpectedAppId, 0, mAppInfo,
                                true /* ignoreAppVersion */);
      if (!success) {
        mAppInfo = nullptr;
      } else {
        LOGI("Successfully loaded nanoapp %s (0x%016" PRIx64 ") version 0x%"
             PRIx32 " from file %s", mAppInfo->name, mAppInfo->appId,
             mAppInfo->appVersion, mFilename);
        // Save the app version field in case this app gets disabled and we
        // still get a query request for the version later on. We are OK not
        // knowing the version prior to the first load because we assume that
        // nanoapps loaded via file are done at CHRE initialization time.
        mExpectedAppVersion = mAppInfo->appVersion;
      }
    }
  }

  return success;
}

uint64_t PlatformNanoapp::getAppId() const {
  return (mAppInfo != nullptr) ? mAppInfo->appId : mExpectedAppId;
}

uint32_t PlatformNanoapp::getAppVersion() const {
  return (mAppInfo != nullptr) ? mAppInfo->appVersion : mExpectedAppVersion;
}

uint32_t PlatformNanoapp::getTargetApiVersion() const {
  return (mAppInfo != nullptr) ? mAppInfo->targetApiVersion : 0;
}

bool PlatformNanoapp::isSystemNanoapp() const {
  // Right now, we assume that system nanoapps are always static nanoapps. Since
  // mAppInfo can only be null either prior to loading the app (in which case
  // this function is not expected to return a valid value anyway), or when a
  // dynamic nanoapp is not running, "false" is the correct return value in that
  // case.
  return (mAppInfo != nullptr) ? mAppInfo->isSystemNanoapp : false;
}

}  // namespace chre
