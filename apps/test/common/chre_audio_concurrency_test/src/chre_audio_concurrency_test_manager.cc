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

#include "chre_audio_concurrency_test_manager.h"

#include <pb_decode.h>

#include "chre/util/nanoapp/log.h"
#include "chre_audio_concurrency_test.nanopb.h"
#include "send_message.h"

#define LOG_TAG "[ChreAudioConcurrencyTest]"

namespace chre {

using test_shared::sendTestResultToHost;

namespace audio_concurrency_test {

namespace {

//! The message type to use with sendTestResultToHost()
constexpr uint32_t kTestResultMessageType =
    chre_audio_concurrency_test_MessageType_TEST_RESULT;

bool isTestSupported() {
  // CHRE audio was supported in CHRE v1.2
  return chreGetVersion() >= CHRE_API_VERSION_1_2;
}

bool getTestStep(const chre_audio_concurrency_test_TestCommand &command,
                 Manager::TestStep *step) {
  bool success = true;
  switch (command.step) {
    case chre_audio_concurrency_test_TestCommand_Step_ENABLE_AUDIO:
      *step = Manager::TestStep::ENABLE_AUDIO;
      break;
    case chre_audio_concurrency_test_TestCommand_Step_VERIFY_AUDIO_RESUME:
      *step = Manager::TestStep::VERIFY_AUDIO_RESUME;
      break;
    default:
      LOGE("Unknown test step %d", command.step);
      success = false;
  }

  return success;
}

}  // anonymous namespace

bool Manager::handleTestCommandMessage(uint16_t hostEndpointId, TestStep step) {
  bool success = true;

  // Treat as success if CHRE audio is unsupported
  if (!isTestSupported()) {
    sendTestResultToHost(hostEndpointId, kTestResultMessageType,
                         true /* success */);
  } else {
    if (step == TestStep::ENABLE_AUDIO) {
      // TODO: Enable audio
    } else if (step == TestStep::VERIFY_AUDIO_RESUME) {
      // TODO: Verify audio resumes
    }

    // TODO: Save test state
  }

  return success;
}

void Manager::handleMessageFromHost(uint32_t senderInstanceId,
                                    const chreMessageFromHostData *hostData) {
  bool success = false;
  uint32_t messageType = hostData->messageType;
  if (senderInstanceId != CHRE_INSTANCE_ID) {
    LOGE("Incorrect sender instance id: %" PRIu32, senderInstanceId);
  } else if (messageType !=
             chre_audio_concurrency_test_MessageType_TEST_COMMAND) {
    LOGE("Invalid message type %" PRIu32, messageType);
  } else {
    pb_istream_t istream = pb_istream_from_buffer(
        static_cast<const pb_byte_t *>(hostData->message),
        hostData->messageSize);
    chre_audio_concurrency_test_TestCommand testCommand =
        chre_audio_concurrency_test_TestCommand_init_default;

    TestStep step;
    if (!pb_decode(&istream, chre_audio_concurrency_test_TestCommand_fields,
                   &testCommand)) {
      LOGE("Failed to decode start command error %s", PB_GET_ERROR(&istream));
    } else if (getTestStep(testCommand, &step)) {
      success = handleTestCommandMessage(hostData->hostEndpoint, step);
    }
  }

  if (!success) {
    sendTestResultToHost(hostData->hostEndpoint, kTestResultMessageType,
                         false /* success */);
  }
}

void Manager::handleEvent(uint32_t senderInstanceId, uint16_t eventType,
                          const void *eventData) {
  if (eventType == CHRE_EVENT_MESSAGE_FROM_HOST) {
    handleMessageFromHost(
        senderInstanceId,
        static_cast<const chreMessageFromHostData *>(eventData));
  } else {
    LOGW("Got unknown event type from senderInstanceId %" PRIu32
         " and with eventType %" PRIu16,
         senderInstanceId, eventType);
  }
}

}  // namespace audio_concurrency_test

}  // namespace chre
