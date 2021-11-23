/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "chre/core/host_notifications.h"

#include "chre/core/event_loop_manager.h"
#include "chre/util/dynamic_vector.h"
#include "chre/util/nested_data_ptr.h"

namespace chre {

namespace {

void hostNotificationCallback(uint16_t type, void *data,
                              void * /* extraData */) {
  // TODO(b/194287786): Store host metadata using HostEndpointConnected event.
  uint16_t hostEndpointId = NestedDataPtr<uint16_t>(data);

  SystemCallbackType callbackType = static_cast<SystemCallbackType>(type);
  if (callbackType == SystemCallbackType::HostEndpointDisconnected) {
    uint16_t eventType = CHRE_EVENT_HOST_ENDPOINT_NOTIFICATION;
    auto *eventData = memoryAlloc<struct chreHostEndpointNotification>();
    eventData->hostEndpointId = hostEndpointId;
    eventData->notificationType = HOST_ENDPOINT_NOTIFICATION_TYPE_DISCONNECT;
    eventData->reserved = 0;

    if (eventData == nullptr) {
      LOG_OOM();
    } else {
      EventLoopManagerSingleton::get()->getEventLoop().postEventOrDie(
          eventType, eventData, freeEventDataCallback, kBroadcastInstanceId);
    }
  }
}

}  // anonymous namespace

void postHostEndpointConnected(uint16_t hostEndpointId) {
  EventLoopManagerSingleton::get()->deferCallback(
      SystemCallbackType::HostEndpointConnected,
      NestedDataPtr<uint16_t>(hostEndpointId), hostNotificationCallback,
      nullptr);
}

void postHostEndpointDisconnected(uint16_t hostEndpointId) {
  EventLoopManagerSingleton::get()->deferCallback(
      SystemCallbackType::HostEndpointDisconnected,
      NestedDataPtr<uint16_t>(hostEndpointId), hostNotificationCallback,
      nullptr);
}

}  // namespace chre
