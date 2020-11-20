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

#include "chre/platform/shared/platform_log.h"
#include "chre/core/event_loop_manager.h"

// TODO(b/174676445): Move functionality that is shared between platforms that
// use buffered logging into the shared/ directory in its own implementation
// file.

void chrePlatformSlpiLogToBuffer(chreLogLevel chreLogLevel, const char *format,
                                 ...) {
  va_list args;
  va_start(args, format);
  if (chre::PlatformLogSingleton::isInitialized()) {
    chre::PlatformLogSingleton::get()->logVa(chreLogLevel, format, args);
  }
  va_end(args);
}

namespace chre {

void sendBufferedLogMessageCallback(uint16_t eventType, void *data,
                                    void * /* extraData */) {
  if (EventLoopManagerSingleton::get()
          ->getEventLoop()
          .getPowerControlManager()
          .hostIsAwake()) {
    PlatformLog *platformLog = PlatformLogSingleton::get();
    LogBuffer *logBuffer = platformLog->getLogBuffer();
    uint8_t *tempLogBufferData =
        reinterpret_cast<uint8_t *>(platformLog->getTempLogBufferData());
    size_t bytesCopied =
        logBuffer->copyLogs(tempLogBufferData, CHRE_MESSAGE_TO_HOST_MAX_SIZE);
    auto &hostCommsMgr =
        EventLoopManagerSingleton::get()->getHostCommsManager();
    hostCommsMgr.sendLogMessageV2(tempLogBufferData, bytesCopied);
  }
}

PlatformLogBase::PlatformLogBase()
    : mLogBuffer(this, mLogBufferData, CHRE_MESSAGE_TO_HOST_MAX_SIZE) {}

void PlatformLogBase::onLogsReady(LogBuffer *logBuffer) {
  // TODO(b/174676964): Have the PlatformLog class also send logs to host if the
  // AP just awoke.
  if (EventLoopManagerSingleton::isInitialized() &&
      EventLoopManagerSingleton::get()
          ->getEventLoop()
          .getPowerControlManager()
          .hostIsAwake()) {
    // Post a deffered callback so that any errors that occur will not be logged
    // causing a never ending recursive loop.
    EventLoopManagerSingleton::get()->deferCallback(
        SystemCallbackType::SendBufferedLogMessage, nullptr,
        sendBufferedLogMessageCallback);
  }
}

PlatformLog::PlatformLog() {}

PlatformLog::~PlatformLog() {}

void PlatformLog::logVa(chreLogLevel logLevel, const char *formatStr,
                        va_list args) {
  LogBufferLogLevel logBufLogLevel = chreToLogBufferLogLevel(logLevel);
  uint64_t timeNs = SystemTime::getMonotonicTime().toRawNanoseconds();
  uint32_t timeMs =
      static_cast<uint32_t>(timeNs / kOneMillisecondInNanoseconds);
  mLogBuffer.handleLogVa(logBufLogLevel, timeMs, formatStr, args);
}

LogBufferLogLevel PlatformLogBase::chreToLogBufferLogLevel(
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

}  // namespace chre
