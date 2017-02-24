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

#ifndef CHRE_CORE_HOST_COMMS_MANAGER_H_
#define CHRE_CORE_HOST_COMMS_MANAGER_H_

#include "chre_api/chre/event.h"
#include "chre/platform/host_link.h"
#include "chre/util/dynamic_vector.h"
#include "chre/util/non_copyable.h"
#include "chre/util/synchronized_memory_pool.h"

namespace chre {

// TODO: these special endpoint values should come from the CHRE API

//! Only valid for messages from host to CHRE - indicates that the sender of the
//! message is not specified.
constexpr uint16_t kHostEndpointUnspecified = 0xFFFE;

//! Only valid for messages from CHRE to host - delivers the message to all
//! registered clients of the Context Hub HAL, which is the default behavior.
constexpr uint16_t kHostEndpointBroadcast = 0xFFFF;

/**
 * Data associated with a message either to or from the host.
 */
struct HostMessage : public NonCopyable {
  //! Source/destination nanoapp ID
  uint64_t appId;

  //! Instance identifier associated with the source/destination nanoapp
  // TODO: do we need to keep this around? right now we assume appId is unique
  uint32_t instanceId;

  //! Identifier for the host-side entity that should receive this message, or
  //! that which sent it
  uint16_t hostEndpoint;

  //! Application-specific message ID
  uint32_t messageType;

  //! Application-defined message data
  DynamicVector<uint8_t> message;

  //! Message free callback supplied by the nanoapp. Must only be invoked from
  //! the EventLoop where the nanoapp runs.
  chreMessageFreeFunction *nanoappFreeFunction;
};

typedef HostMessage MessageFromHost;
typedef HostMessage MessageToHost;

/**
 * Manages bi-directional communications with the host. There must only be one
 * instance of this class per CHRE instance, as the HostLink is not multiplexed
 * per-EventLoop.
 */
class HostCommsManager : public NonCopyable {
 public:
  /**
   * Formulates a MessageToHost using the supplied message contents and passes
   * it to the HostLink for transmission to the host processor.
   *
   * @param messageData Pointer to message payload. Can be null if messageSize
   *        is 0
   * @param messageSize Size of the message to send, in bytes
   * @param messageType Application-defined identifier for the message
   * @param hostEndpoint Identifier for the entity on the host that should
   *        receive this message
   * @param freeCallback Optional callback to invoke when the messageData is no
   *        longer needed (the message has been sent or an error occurred)
   *
   * @return true if the message was accepted into the outbound message queue
   *
   * @see chreSendMessageToHost
   */
  bool sendMessageToHostFromCurrentNanoapp(
      void *messageData, uint32_t messageSize, uint32_t messageType,
      uint16_t hostEndpoint, chreMessageFreeFunction *freeCallback);

  /**
   * TODO: implement and document
   *
   * @param nanoappId
   * @param hostEndpoint
   * @param messageType
   * @param messageData
   * @param messageSize
   */
  void onMessageReceivedFromHost(
      uint64_t nanoappId, uint16_t hostEndpoint, uint32_t messageType,
      void *messageData, size_t messageSize);

  /**
   * Invoked by the HostLink platform layer when it is done with a message to
   * the host: either it successfully sent it, or encountered an error.
   *
   * This function is thread-safe.
   *
   * @param message A message pointer previously given to HostLink::sendMessage
   */
  void onMessageToHostComplete(const MessageToHost *msgToHost);

 private:
  //! The maximum number of messages we can have outstanding at any given time
  static constexpr size_t kMaxOutstandingMessages = 32;

  //! Memory pool used to allocate message metadata (but not the contents of the
  //! messages themselves). Must be synchronized as the same HostCommsManager
  //! handles communications for all EventLoops, and also to support freeing
  //! messages directly in onMessageToHostComplete.
  SynchronizedMemoryPool<HostMessage, kMaxOutstandingMessages> mMessagePool;

  //! The platform-specific link to the host that we manage
  HostLink mHostLink;

  /**
   * Releases memory associated with a message to the host, including invoking
   * the Nanoapp's free callback (if given). Must be called from within the
   * context of the EventLoop that contains the sending Nanoapp.
   *
   * @param msgToHost The message to free
   */
  void freeMessageToHost(MessageToHost *msgToHost);

  /**
   * System callback invoked inside the main CHRE event loop to
   *
   * @param type Callback type (informational only)
   * @param data Pointer to the MessageToHost that is complete
   */
  static void onMessageToHostCompleteCallback(uint16_t type, void *data);
};

} // namespace chre

#endif  // CHRE_CORE_HOST_COMMS_MANAGER_H_
