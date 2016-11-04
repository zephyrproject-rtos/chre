#include "chre/platform/system_timer.h"

#include "chre/platform/log.h"

#include <errno.h>
#include <string.h>

namespace chre {

namespace {

constexpr uint64_t kOneSecondInNanoseconds = 1000000000;

void NanosecondsToTimespec(uint64_t ns, struct timespec *ts) {
  ts->tv_sec  = ns / kOneSecondInNanoseconds;
  ts->tv_nsec = ns % kOneSecondInNanoseconds;
}

}  // anonymous namespace

void SystemTimerBase::systemTimerNotifyCallback(union sigval cookie) {
  SystemTimer *sysTimer = static_cast<SystemTimer*>(cookie.sival_ptr);

  sysTimer->mCallback(sysTimer->mData);
}

SystemTimer::SystemTimer(SystemTimerCallback *callback, void *data)
    : mCallback(callback), mData(data) {}

SystemTimer::~SystemTimer() {
  if (mInitialized) {
    int ret = timer_delete(mTimerId);
    if (ret != 0) {
      LOGE("Couldn't delete timer: %s", strerror(errno));
    }
    mInitialized = false;
  }
}

bool SystemTimer::init() {
  if (mInitialized) {
    LOGW("Tried re-initializing timer");
  } else {
    struct sigevent sigevt = {};

    sigevt.sigev_notify = SIGEV_THREAD;
    sigevt.sigev_value.sival_ptr = this;
    sigevt.sigev_notify_function = systemTimerNotifyCallback;
    sigevt.sigev_notify_attributes = nullptr;

    int ret = timer_create(CLOCK_MONOTONIC, &sigevt, &mTimerId);
    if (ret != 0) {
      LOGE("Couldn't create timer: %s", strerror(errno));
    } else {
      mInitialized = true;
    }
  }

  return mInitialized;
}

bool SystemTimer::set(uint64_t delayNs, uint64_t intervalNs) {
  // 0 has a special meaning in POSIX, i.e. cancel the timer. In our API, a
  // value of 0 just means fire right away.
  if (delayNs == 0) {
    delayNs = 1;
  }
  return (mInitialized) ? setInternal(delayNs, intervalNs) : false;
}

bool SystemTimer::cancel() {
  // Setting delay to 0 disarms the timer
  return (mInitialized) ? setInternal(0, 0) : false;
}

bool SystemTimerBase::setInternal(uint64_t delayNs, uint64_t intervalNs) {
  constexpr int kFlags = 0;
  struct itimerspec spec = {};
  bool success = false;

  NanosecondsToTimespec(delayNs, &spec.it_value);
  NanosecondsToTimespec(intervalNs, &spec.it_interval);

  int ret = timer_settime(mTimerId, kFlags, &spec, nullptr);
  if (ret != 0) {
    LOGE("Couldn't set timer: %s", strerror(errno));
  } else {
    LOGD("Set timer to expire in %.f ms with interval %.f ms",
         (delayNs / 1000000.0), (intervalNs / 1000000.0));
    success = true;
  }

  return success;
}

}  // namespace chre
