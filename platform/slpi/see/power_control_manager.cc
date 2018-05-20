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

#include "chre/platform/power_control_manager.h"

#include "chre/platform/slpi/power_control_util.h"
#include "chre/platform/system_time.h"
#include "chre/util/lock_guard.h"

namespace chre {

PowerControlManagerBase::PowerControlManagerBase() {
#ifdef CHRE_SLPI_UIMG_ENABLED
  const char kClientName[] = "CHRE";
  mClientHandle = sns_island_aggregator_register_client(kClientName);
  if (mClientHandle == nullptr) {
    FATAL_ERROR("Island aggregator client register failed");
  }
#endif // CHRE_SLPI_UIMG_ENABLED
}

PowerControlManagerBase::~PowerControlManagerBase() {
#ifdef CHRE_SLPI_UIMG_ENABLED
  sns_island_aggregator_deregister_client(mClientHandle);
#endif // CHRE_SLPI_UIMG_ENABLED
}

bool PowerControlManagerBase::voteBigImage(bool bigImage) {
#ifdef CHRE_SLPI_UIMG_ENABLED
  bool success = true;

  {
    LockGuard<Mutex> lock(mMutex);
    mLastBigImageRequest = bigImage;

    bool needBigImage = (bigImage || mBigImageRefCount > 0);
    if (needBigImage != mLastBigImageVote) {
      success = voteSnsPowerMode(needBigImage);
      mLastBigImageVote = needBigImage;
    }
  }
  return success;
#else
  return true;
#endif // CHRE_SLPI_UIMG_ENABLED
}

void PowerControlManagerBase::onHostWakeSuspendEvent(bool awake) {
  if (mHostIsAwake != awake) {
    mHostIsAwake = awake;

    EventLoopManagerSingleton::get()->getEventLoop().postEvent(
        mHostIsAwake ? CHRE_EVENT_HOST_AWAKE : CHRE_EVENT_HOST_ASLEEP,
        nullptr /* eventData */, nullptr /* freeCallback */);
  }
}

#ifdef CHRE_SLPI_UIMG_ENABLED
void PowerControlManagerBase::incrementBigImageRefCount() {
  LockGuard<Mutex> lock(mMutex);

  if (mBigImageRefCount++ == 0) {
    mRefCountStart = Milliseconds(SystemTime::getMonotonicTime());
    LOGW("Big image ref count begins");

    if (!mLastBigImageVote) {
      // Do not call voteBigImage() directly as it will override
      // mLastBigImageRequest.
      voteSnsPowerMode(true /* bigImage */);
      mLastBigImageVote = true;
    }
  }
}

void PowerControlManagerBase::decrementBigImageRefCount() {
  LockGuard<Mutex> lock(mMutex);
  CHRE_ASSERT_LOG(mBigImageRefCount > 0,
                  "Tried to decrement big image ref count when it's 0");

  if (--mBigImageRefCount == 0) {
    uint64_t duration =
        Milliseconds(SystemTime::getMonotonicTime()).getMilliseconds()
        - mRefCountStart.getMilliseconds();
    LOGW("Big image ref count ends: %" PRIu64 " ms", duration);

    // There's no big image activity now, restore the intended uimg power state.
    if (!mLastBigImageRequest) {
      voteSnsPowerMode(false /* bigImage */);
      mLastBigImageVote = false;
    }
  }
}

bool PowerControlManagerBase::voteSnsPowerMode(bool bigImage) {
  sns_rc rc = bigImage
      ? sns_island_block(mClientHandle) : sns_island_unblock(mClientHandle);

  // TODO: (b/74524281) define success = (rc == SNS_RC_SUCCESS).
  bool success = (rc != SNS_RC_FAILED);
  if (!success) {
    // Note that FATAL_ERROR must not be used here, because this can be called
    // from preFatalError() (not that we should use it here regardless)
    LOGE("Failed to vote for bigImage %d with result %d", bigImage, rc);
  }
  return success;
}
#endif  // CHRE_SLPI_UIMG_ENABLED

void PowerControlManager::postEventLoopProcess(size_t numPendingEvents) {
  if (numPendingEvents == 0 && !slpiInUImage()) {
    voteBigImage(false /* bigImage */);
  }
}

bool PowerControlManager::hostIsAwake() {
  return mHostIsAwake;
}

} // namespace chre
