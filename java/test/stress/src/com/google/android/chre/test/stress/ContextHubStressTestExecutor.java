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
package com.google.android.chre.test.stress;

import android.hardware.location.ContextHubClient;
import android.hardware.location.ContextHubClientCallback;
import android.hardware.location.ContextHubInfo;
import android.hardware.location.ContextHubManager;
import android.hardware.location.ContextHubTransaction;
import android.hardware.location.NanoAppBinary;
import android.hardware.location.NanoAppMessage;
import android.util.Log;

import com.google.android.chre.nanoapp.proto.ChreStressTest;
import com.google.android.chre.nanoapp.proto.ChreTestCommon;
import com.google.android.utils.chre.ChreTestUtil;
import com.google.protobuf.InvalidProtocolBufferException;

import org.junit.Assert;

import java.nio.charset.StandardCharsets;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

/**
 * A class that can execute the CHRE Stress test.
 */
public class ContextHubStressTestExecutor extends ContextHubClientCallback {
    private static final String TAG = "ContextHubStressTestExecutor";

    private final NanoAppBinary mNanoAppBinary;

    private final long mNanoAppId;

    private ContextHubClient mContextHubClient;

    private final AtomicReference<ChreTestCommon.TestResult> mTestResult =
            new AtomicReference<>();

    private final AtomicBoolean mChreReset = new AtomicBoolean(false);

    private final ContextHubManager mContextHubManager;

    private final ContextHubInfo mContextHubInfo;

    private CountDownLatch mCountDownLatch;

    public ContextHubStressTestExecutor(ContextHubManager manager, ContextHubInfo info,
            NanoAppBinary binary) {
        mNanoAppBinary = binary;
        mNanoAppId = mNanoAppBinary.getNanoAppId();
        mContextHubManager = manager;
        mContextHubInfo = info;
    }

    @Override
    public void onMessageFromNanoApp(ContextHubClient client, NanoAppMessage message) {
        if (message.getNanoAppId() == mNanoAppId) {
            boolean valid = false;
            switch (message.getMessageType()) {
                case ChreStressTest.MessageType.TEST_RESULT_VALUE: {
                    try {
                        mTestResult.set(
                                ChreTestCommon.TestResult.parseFrom(message.getMessageBody()));
                        valid = true;
                    } catch (InvalidProtocolBufferException e) {
                        Log.e(TAG, "Failed to parse message: " + e.getMessage());
                    }
                    break;
                }
                default: {
                    Log.e(TAG, "Unknown message type " + message.getMessageType());
                }
            }

            if (valid && mCountDownLatch != null) {
                mCountDownLatch.countDown();
            }
        }
    }

    @Override
    public void onHubReset(ContextHubClient client) {
        mChreReset.set(true);
        if (mCountDownLatch != null) {
            mCountDownLatch.countDown();
        }
    }

    /**
     * Should be invoked before run() is invoked to set up the test, e.g. in a @Before method.
     */
    public void init() {
        ChreTestUtil.loadNanoAppAssertSuccess(mContextHubManager, mContextHubInfo, mNanoAppBinary);
        mContextHubClient = mContextHubManager.createClient(mContextHubInfo, this);
        Assert.assertTrue(mContextHubClient != null);
    }

    /**
     * @param timeout The amount of time to run the stress test.
     * @param unit The unit for timeout.
     */
    public void runStressTest(long timeout, TimeUnit unit) {
        mTestResult.set(null);
        mCountDownLatch = new CountDownLatch(1);

        // TODO(b/186868033): Add other features
        sendTestMessage(ChreStressTest.TestCommand.Feature.WIFI, true /* start */);
        sendTestMessage(ChreStressTest.TestCommand.Feature.GNSS_LOCATION, true /* start */);

        try {
            mCountDownLatch.await(timeout, unit);
        } catch (InterruptedException e) {
            Assert.fail(e.getMessage());
        }

        if (mTestResult.get() != null
                && mTestResult.get().getCode() == ChreTestCommon.TestResult.Code.FAILED) {
            if (mTestResult.get().hasErrorMessage()) {
                Assert.fail(new String(mTestResult.get().getErrorMessage().toByteArray(),
                        StandardCharsets.UTF_8));
            } else {
                Assert.fail("Stress test failed");
            }
        }

        sendTestMessage(ChreStressTest.TestCommand.Feature.WIFI, false /* start */);
        sendTestMessage(ChreStressTest.TestCommand.Feature.GNSS_LOCATION, false /* start */);

        try {
            // Add a short delay to make sure the stop command did not cause issues.
            Thread.sleep(10000);
        } catch (InterruptedException e) {
            Assert.fail(e.getMessage());
        }
    }

    /**
     * Cleans up the test, should be invoked in e.g. @After method.
     */
    public void deinit() {
        ChreTestUtil.unloadNanoApp(mContextHubManager, mContextHubInfo, mNanoAppId);
        if (mContextHubClient != null) {
            mContextHubClient.close();
        }

        if (mChreReset.get()) {
            Assert.fail("CHRE reset during the test");
        }
    }

    /**
     * @param feature The feature to start testing for.
     * @param start true to start the test, false to stop.
     */
    private void sendTestMessage(ChreStressTest.TestCommand.Feature feature, boolean start) {
        ChreStressTest.TestCommand testCommand = ChreStressTest.TestCommand.newBuilder()
                .setFeature(feature).setStart(start).build();

        NanoAppMessage message = NanoAppMessage.createMessageToNanoApp(
                mNanoAppId, ChreStressTest.MessageType.TEST_COMMAND_VALUE,
                testCommand.toByteArray());
        int result = mContextHubClient.sendMessageToNanoApp(message);
        if (result != ContextHubTransaction.RESULT_SUCCESS) {
            Assert.fail("Failed to send message: result = " + result);
        }
    }
}
