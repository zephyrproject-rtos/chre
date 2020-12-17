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

#include "chre/platform/shared/log_buffer_manager.h"

#include "chre/core/event_loop_manager.h"
#include "chre/util/lock_guard.h"

void chrePlatformLogToBuffer(chreLogLevel chreLogLevel, const char *format,
                             ...) {
  va_list args;
  va_start(args, format);
  if (chre::LogBufferManagerSingleton::isInitialized()) {
    chre::LogBufferManagerSingleton::get()->logVa(chreLogLevel, format, args);
  }
  va_end(args);
}

namespace chre {

void sendBufferedLogMessageCallback(uint16_t /* eventType */, void * /* data */,
                                    void * /* extraData */) {
  LogBufferManagerSingleton::get()->sendLogsToHost();
}

void LogBufferManager::onLogsReady(LogBuffer *logBuffer) {
  // TODO(b/174676964): Have the LogBufferManager class also send logs to host
  // if the AP just awoke.
  LockGuard<Mutex> lockGuard(mFlushLogsMutex);
  if (!mLogFlushToHostPending) {
    if (EventLoopManagerSingleton::isInitialized() &&
        EventLoopManagerSingleton::get()
            ->getEventLoop()
            .getPowerControlManager()
            .hostIsAwake()) {
      EventLoopManagerSingleton::get()->deferCallback(
          SystemCallbackType::SendBufferedLogMessage, nullptr,
          sendBufferedLogMessageCallback);
      mLogFlushToHostPending = true;
    }
  } else {
    mLogsBecameReadyWhileFlushPending = true;
  }
}

void LogBufferManager::onLogsSentToHost() {
  bool shouldPostCallback = false;
  {
    LockGuard<Mutex> lockGuard(mFlushLogsMutex);
    shouldPostCallback = mLogsBecameReadyWhileFlushPending;
    mLogsBecameReadyWhileFlushPending = false;
    mLogFlushToHostPending = shouldPostCallback;
  }
  if (shouldPostCallback) {
    Nanoseconds delay(Milliseconds(10).toRawNanoseconds());
    EventLoopManagerSingleton::get()->setDelayedCallback(
        SystemCallbackType::SendBufferedLogMessage, nullptr,
        sendBufferedLogMessageCallback, delay);
  }
}

void LogBufferManager::sendLogsToHost() {
  if (EventLoopManagerSingleton::get()
          ->getEventLoop()
          .getPowerControlManager()
          .hostIsAwake()) {
    LogBufferManager *platformLog = LogBufferManagerSingleton::get();
    LogBuffer *logBuffer = platformLog->getLogBuffer();
    uint8_t *tempLogBufferData =
        reinterpret_cast<uint8_t *>(platformLog->getTempLogBufferData());
    size_t bytesCopied =
        logBuffer->copyLogs(tempLogBufferData, sizeof(mLogBufferData));
    auto &hostCommsMgr =
        EventLoopManagerSingleton::get()->getHostCommsManager();
    hostCommsMgr.sendLogMessageV2(tempLogBufferData, bytesCopied);
  }
}

void LogBufferManager::log(chreLogLevel logLevel, const char *formatStr, ...) {
  va_list args;
  va_start(args, formatStr);
  logVa(logLevel, formatStr, args);
  va_end(args);
}

void LogBufferManager::logVa(chreLogLevel logLevel, const char *formatStr,
                             va_list args) {
  LogBufferLogLevel logBufLogLevel = chreToLogBufferLogLevel(logLevel);
  uint64_t timeNs = SystemTime::getMonotonicTime().toRawNanoseconds();
  uint32_t timeMs =
      static_cast<uint32_t>(timeNs / kOneMillisecondInNanoseconds);
  mLogBuffer.handleLogVa(logBufLogLevel, timeMs, formatStr, args);
}

LogBuffer *LogBufferManager::getLogBuffer() {
  return &mLogBuffer;
}

uint8_t *LogBufferManager::getTempLogBufferData() {
  return mTempLogBufferData;
}

LogBufferLogLevel LogBufferManager::chreToLogBufferLogLevel(
    chreLogLevel chreLogLevel) {
  switch (chreLogLevel) {
    case CHRE_LOG_ERROR:
      return LogBufferLogLevel::ERROR;
    case CHRE_LOG_WARN:
      return LogBufferLogLevel::WARN;
    case CHRE_LOG_INFO:
      return LogBufferLogLevel::INFO;
    default:  // CHRE_LOG_DEBUG
      return LogBufferLogLevel::DEBUG;
  }
}

//! Explicitly instantiate the EventLoopManagerSingleton to reduce codesize.
template class Singleton<LogBufferManager>;

}  // namespace chre
