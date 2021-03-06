/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef CHRE_PLATFORM_LINUX_PLATFORM_LOG_H_
#define CHRE_PLATFORM_LINUX_PLATFORM_LOG_H_

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "chre/util/singleton.h"
#include "chre_api/chre/re.h"

namespace chre {

/**
 * Storage for the Linux implementation of the PlatformLog class.
 */
class PlatformLog {
 public:
  PlatformLog();

  ~PlatformLog();

  /**
   * Logs message with printf-style arguments. No trailing newline is required
   * for this method.
   */
  void log(chreLogLevel logLevel, const char *formatStr, ...) {
    va_list args;
    va_start(args, formatStr);
    logVa(logLevel, formatStr, args);
    va_end(args);
  }

  /**
   * Logs message with printf-style arguments. No trailing newline is required
   * for this method. Uses va_list parameter instead of ...
   */
  void logVa(chreLogLevel logLevel, const char *formatStr, va_list args);

 private:
  /**
   * A looper method that idles on a condition variable on logs becoming
   * available. When logs are available, they are output via std::cout.
   */
  void logLooper();

  //! The thread that waits on incoming log messages and sends them out to
  //! std::cout.
  std::thread mLoggerThread;

  //! A mutex to guard the shared queue and exit condition of this class.
  std::mutex mMutex;

  //! The condition variable to signal that the log looper has messages
  //! available to output.
  std::condition_variable mConditionVariable;

  //! A queue of incoming log messages.
  std::queue<char *> mLogQueue;

  //! A flag to indicate that the logger should shut down.
  bool mStopLogger = false;
};

typedef Singleton<PlatformLog> PlatformLogSingleton;

}  // namespace chre

#endif  // CHRE_PLATFORM_LINUX_PLATFORM_LOG_H_
