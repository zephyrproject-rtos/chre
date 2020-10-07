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

#include <gtest/gtest.h>
#include <string>

#include "chre/platform/atomic.h"
#include "chre/platform/condition_variable.h"
#include "chre/platform/mutex.h"
#include "chre/platform/shared/log_buffer.h"

namespace chre {

// TODO(b/146164384): Test that the onLogsReady callback is called
//                    asynchronously

class TestLogBufferCallback : public LogBufferCallbackInterface {
 public:
  void onLogsReady(LogBuffer *logBuffer) {
    // Do nothing
  }
};

static constexpr size_t kDefaultBufferSize = 1024;
static constexpr size_t kBytesBeforeLogData = 6;

TEST(LogBuffer, HandleOneLogAndCopy) {
  char buffer[kDefaultBufferSize];
  constexpr size_t kOutBufferSize = 20;
  char outBuffer[kOutBufferSize];
  const char *testLogStr = "test";
  char testedBuffer[kOutBufferSize];
  TestLogBufferCallback callback;

  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);
  logBuffer.handleLog(LogBufferLogLevel::INFO, 0, testLogStr);
  size_t bytesCopied = logBuffer.copyLogs(outBuffer, kOutBufferSize);

  EXPECT_EQ(bytesCopied,
            strlen(testLogStr) +
                kBytesBeforeLogData /*loglevel, timestamp, and loglen*/);
  memcpy(testedBuffer, outBuffer + kBytesBeforeLogData, strlen(testLogStr));
  testedBuffer[strlen(testLogStr)] = '\0';
  EXPECT_TRUE(strcmp(testedBuffer, testLogStr) == 0);
}

TEST(LogBuffer, FailOnMoreCopyThanHandle) {
  char buffer[kDefaultBufferSize];
  constexpr size_t kOutBufferSize = 20;
  char outBuffer[kOutBufferSize];
  constexpr size_t kEmptyBufferSize = 10;
  char emptyBuffer[kEmptyBufferSize];
  emptyBuffer[0] = '\0';
  const char *testLogStr = "test";
  TestLogBufferCallback callback;
  char testedBuffer[kOutBufferSize];

  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);
  logBuffer.handleLog(LogBufferLogLevel::INFO, 0, testLogStr);
  logBuffer.copyLogs(outBuffer, kOutBufferSize);
  memcpy(testedBuffer, outBuffer + kBytesBeforeLogData, strlen(testLogStr));
  testedBuffer[strlen(testLogStr)] = '\0';
  size_t bytesCopied = logBuffer.copyLogs(outBuffer, kOutBufferSize);

  EXPECT_EQ(bytesCopied, 0);
  EXPECT_EQ(strlen(emptyBuffer), 0);
}

TEST(LogBuffer, FailOnHandleLargerLogThanBufferSize) {
  char buffer[kDefaultBufferSize];
  constexpr size_t kOutBufferSize = 20;
  char outBuffer[kOutBufferSize];
  // Note the size of this log is too big to fit in the buffer that we are
  // using for the LogBuffer object
  std::string testLogStrStr(1025, 'a');
  TestLogBufferCallback callback;

  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);
  logBuffer.handleLog(LogBufferLogLevel::INFO, 0, testLogStrStr.c_str());
  size_t bytesCopied = logBuffer.copyLogs(outBuffer, kOutBufferSize);

  // Should not be able to read this log out because there should be no log in
  // the first place
  EXPECT_EQ(bytesCopied, 0);
}

TEST(LogBuffer, LogOverwritten) {
  char buffer[kDefaultBufferSize];
  constexpr size_t kOutBufferSize = 200;
  char outBuffer[kOutBufferSize];
  char testedBuffer[kOutBufferSize];
  TestLogBufferCallback callback;
  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);
  // This for loop adds 1060 bytes of data through the buffer which is > than
  // 1024
  for (size_t i = 0; i < 10; i++) {
    std::string testLogStrStr(100, 'a' + i);
    const char *testLogStr = testLogStrStr.c_str();
    logBuffer.handleLog(LogBufferLogLevel::INFO, 0, testLogStr);
  }
  size_t bytesCopied = logBuffer.copyLogs(outBuffer, kBytesBeforeLogData + 100);
  memcpy(testedBuffer, outBuffer + kBytesBeforeLogData, 100);
  testedBuffer[100] = '\0';

  // Should have read out the second from front test log string which is 'a' + 1
  // = 'b'
  EXPECT_TRUE(strcmp(testedBuffer, std::string(100, 'b').c_str()) == 0);
  EXPECT_EQ(bytesCopied, kBytesBeforeLogData + 100);
}

TEST(LogBuffer, CopyIntoEmptyBuffer) {
  char buffer[kDefaultBufferSize];
  constexpr size_t kOutBufferSize = 0;
  char outBuffer[kOutBufferSize];
  TestLogBufferCallback callback;
  LogBuffer logBuffer(&callback, buffer, kDefaultBufferSize);

  logBuffer.handleLog(LogBufferLogLevel::INFO, 0, "test");
  size_t bytesCopied = logBuffer.copyLogs(outBuffer, kOutBufferSize);

  EXPECT_EQ(bytesCopied, 0);
}

// TODO(srok): Add multithreaded tests

}  // namespace chre
