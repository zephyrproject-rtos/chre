/*
 * Copyright (C) 2022 The Android Open Source Project
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
package com.google.android.chre.utils.pigweed;

import android.hardware.location.ContextHubClient;
import android.hardware.location.ContextHubTransaction;
import android.hardware.location.NanoAppMessage;

import dev.pigweed.pw_rpc.Channel;
import dev.pigweed.pw_rpc.ChannelOutputException;

/**
 * Implements the Channel.Output interface of Pigweed RPC and provides a layer of abstraction on top
 * of Pigweed RPC to make it more friendly to use with CHRE APIs.
 */
public class ChreChannelOutput implements Channel.Output {
    /**
    * Random value chosen not too close to max value to try to avoid conflicts with other messages
    * in case pw rpc isn't the only way the client chooses to communicate.
    */
    public static final int PW_RPC_CHRE_MESSAGE_TYPE = Integer.MAX_VALUE - 10;

    private final ContextHubClient mClient;
    private final long mNanoappId;

    ChreChannelOutput(ContextHubClient client, long nanoappId) {
        mClient = client;
        mNanoappId = nanoappId;
    }

    @Override
    public void send(byte[] packet) throws ChannelOutputException {
        NanoAppMessage message = NanoAppMessage.createMessageToNanoApp(
                mNanoappId, PW_RPC_CHRE_MESSAGE_TYPE, packet);
        if (ContextHubTransaction.RESULT_SUCCESS != mClient.sendMessageToNanoApp(message)) {
            throw new ChannelOutputException();
        }
    }
}
