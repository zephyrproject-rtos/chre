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

#ifndef CHRE_PLATFORM_LOG_BUFFER_MANAGER_BUFFER_H_
#define CHRE_PLATFORM_LOG_BUFFER_MANAGER_BUFFER_H_

#include "chre/platform/mutex.h"
#include "chre/platform/shared/log_buffer.h"
#include "chre/util/singleton.h"
#include "chre_api/chre/re.h"

namespace chre {

/**
 * A log buffer manager that platform code can use to buffer logs when the host
 * is not available and then send them off when the host becomes available. Uses
 * the LogBuffer API to buffer the logs in memory.
 *
 * When implementing this class in platform code. Use the singleton defined
 * after this class and pass logs to the log or logVa methods. Initialize the
 * singleton before using it. Call the onLogsSentToHost callback immediately
 * after sending logs to the host.
 */
class LogBufferManager : public LogBufferCallbackInterface {
 public:
  LogBufferManager()
      : mLogBuffer(this, mLogBufferData, sizeof(mLogBufferData)) {}

  ~LogBufferManager() = default;

  /**
   * Logs message with printf-style arguments. No trailing newline is required
   * for this method.
   */
  void log(chreLogLevel logLevel, const char *formatStr, ...);

  /**
   * Logs message with printf-style arguments. No trailing newline is required
   * for this method. Uses va_list parameter instead of ...
   */
  void logVa(chreLogLevel logLevel, const char *formatStr, va_list args);

  /**
   * Overrides required method from LogBufferCallbackInterface.
   */
  void onLogsReady(LogBuffer *logBuffer) final;

  /**
   * The platform code should call this method after the logs have been sent to
   * the host to signal that more logs can be sent to the host when ready.
   */
  void onLogsSentToHost();

  /**
   * Send logs to this host. This is called inside the deferred callback past to
   * the event loop manaager.
   */
  void sendLogsToHost();

 private:
  LogBuffer *getLogBuffer();

  uint8_t *getTempLogBufferData();

  /*
   * @return The LogBuffer log level for the given CHRE log level.
   */
  LogBufferLogLevel chreToLogBufferLogLevel(chreLogLevel chreLogLevel);

  LogBuffer mLogBuffer;
  uint8_t mLogBufferData[CHRE_MESSAGE_TO_HOST_MAX_SIZE];
  uint8_t mTempLogBufferData[sizeof(mLogBufferData)];

  bool mLogFlushToHostPending = false;
  bool mLogsBecameReadyWhileFlushPending = false;
  Mutex mFlushLogsMutex;
};

//! Provides an alias to the LogBufferManager singleton.
typedef Singleton<LogBufferManager> LogBufferManagerSingleton;

//! Extern the explicit LogBufferManagerSingleton to force non-inline calls.
//! This reduces codesize considerably.
extern template class Singleton<LogBufferManager>;

}  // namespace chre

#endif  // CHRE_PLATFORM_LOG_BUFFER_MANAGER_H_
