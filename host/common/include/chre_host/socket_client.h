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

#ifndef CHRE_HOST_SOCKET_CLIENT_H_
#define CHRE_HOST_SOCKET_CLIENT_H_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <cutils/sockets.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

namespace android {
namespace chre {

class SocketClient {
 public:
  SocketClient();
  ~SocketClient();

  /**
   * Represents the callback interface used for handling events that occur on
   * the receive thread. Note that it is *not* safe to call connect() or
   * disconnect() from the context of these callbacks.
   */
  class ICallbacks : public VirtualLightRefBase {
   public:
    /**
     * Invoked from within the context of the read thread when a message is
     * received on the socket.
     *
     * @param data Buffer containing received message data
     * @param length Size of the message in bytes
     */
    virtual void onMessageReceived(const void *data, size_t length) = 0;

    /**
     * Invoked when the remote side disconnects the socket. It is not safe to
     * call connect() or disconnect() from the context of this callback.
     */
    virtual void onSocketDisconnectedByRemote() {};

    /**
     * Invoked if reconnectAutomatically was true in connect() and we've
     * successfully reconnected the socket.
     */
    virtual void onSocketReconnected() {};

    /**
     * Invoked if reconnectAutomatically was true in connect(), and we've tried
     * to reconnect the socket too many times and are giving up. After this, the
     */
    virtual void onReconnectAborted() {};
  };

  /**
   * Connects to the Android reserved namespace socket with the given name,
   * and starts a receive thread to handle messages received on the socket.
   * Returns
   *
   * @param socketName Name of the Android domain socket to connect to
   * @param reconnectAutomatically If true, automatically attempt to re-connect
   *        to the socket if disconnected by the remote end. This does not
   *        influence the initial connection attempt, which happens
   *        synchronously within this function call.
   * @param callbacks
   *
   * @return true if the connection was successful
   */
  bool connect(const char *socketName, bool reconnectAutomatically,
               const ::android::sp<ICallbacks>& callbacks);

  /**
   * Performs graceful teardown of the socket. After this function returns, this
   * object will no longer invoke any callbacks or hold a reference to the
   * callbacks object provided to connect().
   */
  void disconnect();

  /**
   * Send a message on the connected socket. Safe to call from any thread.
   *
   * @param data Buffer containing message data
   * @param length Size of the message to send in bytes
   *
   * @return true if the message was successfully sent
   */
  bool sendMessage(const void *data, size_t length);

 private:
  static constexpr size_t kMaxSocketNameLen = 64;
  char mSocketName[kMaxSocketNameLen];
  bool mReconnectAutomatically;
  sp<ICallbacks> mCallbacks;

  std::atomic<int> mSockFd;
  std::thread mRxThread;

  //! Set to true when we initiate the graceful socket shutdown procedure, so we
  //! know not to invoke onSocketDisconnectedByRemote()
  std::atomic<bool> mGracefulShutdown;

  //! Condition variable used as the method to wake the RX thread when we want
  //! to disconnect, but it's trying to reconnect automatically
  std::condition_variable mShutdownCond;
  std::mutex mShutdownMutex;

  bool inReceiveThread() const;
  void receiveThread();
  bool receiveThreadRunning() const;
  bool reconnect();
  bool tryConnect();
};

}  // namespace chre
}  // namespace android

#endif  // CHRE_HOST_SOCKET_CLIENT_H_
