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
package com.google.android.chre.test.chqts;

import android.hardware.location.ContextHubClient;
import android.hardware.location.ContextHubClientCallback;
import android.hardware.location.ContextHubInfo;
import android.hardware.location.ContextHubManager;
import android.hardware.location.ContextHubTransaction;
import android.hardware.location.NanoAppBinary;
import android.hardware.location.NanoAppMessage;
import android.hardware.location.NanoAppState;
import android.util.Log;

import com.google.android.utils.chre.ChreTestUtil;

import org.junit.Assert;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.Charset;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

/**
 * A class that can execute the CHQTS "general" tests. Nanoapps using this "general" test framework
 * have the name "general_test".
 *
 * A test successfully passes in one of two ways:
 * - MessageType.SUCCESS received from the Nanoapp by this infrastructure.
 * - A call to ContextHubGeneralTestExecutor.pass() by the test code.
 *
 * TODO: Refactor this class to be able to be invoked for < P builds.
 */
public class ContextHubGeneralTestExecutor extends ContextHubClientCallback {
    public static final String TAG = "ContextHubGeneralTestExecutor";

    private final NanoAppBinary mNanoAppBinary;

    private final long mNanoAppId;

    private ContextHubClient mContextHubClient;

    private final ContextHubManager mContextHubManager;

    private final ContextHubInfo mContextHubInfo;

    private CountDownLatch mCountDownLatch;

    private boolean mInitialized = false;

    private AtomicReference<String> mErrorString = new AtomicReference<>(null);

    public ContextHubGeneralTestExecutor(ContextHubManager manager, ContextHubInfo info,
            NanoAppBinary binary) {
        mContextHubManager = manager;
        mContextHubInfo = info;
        mNanoAppBinary = binary;
        mNanoAppId = mNanoAppBinary.getNanoAppId();
    }

    @Override
    public void onMessageFromNanoApp(ContextHubClient client, NanoAppMessage message) {
        if (message.getNanoAppId() == mNanoAppId) {
            NanoAppMessage realMessage = hackMessageFromNanoApp(message);

            int messageType = message.getMessageType();
            ContextHubTestConstants.MessageType messageEnum =
                    ContextHubTestConstants.MessageType.fromInt(messageType, "");
            byte[] data = message.getMessageBody();

            switch (messageEnum) {
                case INVALID_MESSAGE_TYPE:  // fall-through
                case FAILURE:  // fall-through
                case INTERNAL_FAILURE:
                    // These are univeral failure conditions for all tests.
                    // If they have data, it's expected to be an ASCII string.
                    String errorString = new String(data, Charset.forName("US-ASCII"));
                    mErrorString.set(errorString);
                    mCountDownLatch.countDown();
                    break;

                case SKIPPED:
                    // TODO: Use junit Assume
                    String reason = new String(data, Charset.forName("US-ASCII"));
                    Log.w(TAG, "SKIPPED " + ":" + reason);
                    pass();
                    break;

                case SUCCESS:
                    // This is a universal success for the test.  We ignore
                    // 'data'.
                    pass();
                    break;

                default:
                    // TODO: Handle specific message
            }
        }
    }

    /**
     * Should be invoked before run() is invoked to set up the test, e.g. in a @Before method.
     */
    public void init() {
        Assert.assertFalse("init() must not be invoked when already initialized", mInitialized);

        mContextHubClient = mContextHubManager.createClient(mContextHubInfo, this);
        Assert.assertTrue(mContextHubClient != null);

        unloadAllNanoApps();
        ChreTestUtil.loadNanoAppAssertSuccess(mContextHubManager, mContextHubInfo, mNanoAppBinary);

        mInitialized = true;
    }

    /**
     * Run the test.
     */
    public void run(ContextHubTestConstants.TestNames testName, long timeoutSeconds) {
        mCountDownLatch = new CountDownLatch(1);
        NanoAppMessage message = NanoAppMessage.createMessageToNanoApp(
                mNanoAppId, testName.asInt(), new byte[0]);

        int result = mContextHubClient.sendMessageToNanoApp(hackMessageToNanoApp(message));
        if (result != ContextHubTransaction.RESULT_SUCCESS) {
            Assert.fail("Failed to send message: result = " + result);
        }

        boolean success = false;
        try {
            success = mCountDownLatch.await(timeoutSeconds, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            Assert.fail(e.getMessage());
        }

        Assert.assertTrue("Test timed out", success);
        if (mErrorString.get() != null) {
            Assert.fail(mErrorString.get());
        }
    }

    /**
     * Invoke to indicate that the test has passed.
     */
    public void pass() {
        mCountDownLatch.countDown();
    }

    /**
     * Cleans up the test, should be invoked in e.g. @After method.
     */
    public void deinit() {
        Assert.assertTrue("deinit() must be invoked after init()", mInitialized);

        // TODO: If the nanoapp aborted (i.e. test failed), wait for CHRE reset or nanoapp abort
        // callback, and otherwise assert unload success.
        mContextHubManager.unloadNanoApp(mContextHubInfo, mNanoAppId);
        mContextHubClient.close();
        mContextHubClient = null;

        mInitialized = false;
    }

    private void unloadAllNanoApps() {
        List<NanoAppState> stateList =
                ChreTestUtil.queryNanoAppsAssertSuccess(mContextHubManager, mContextHubInfo);
        for (NanoAppState state : stateList) {
            ChreTestUtil.unloadNanoAppAssertSuccess(
                    mContextHubManager, mContextHubInfo, state.getNanoAppId());
        }
    }

    // TODO: Remove this hack
    private NanoAppMessage hackMessageToNanoApp(NanoAppMessage message) {
        // For NYC, we are not able to assume that the messageType correctly
        // makes it to the nanoapp.  So we put it, in little endian, as the
        // first four bytes of the message.
        byte[] origData = message.getMessageBody();
        ByteBuffer newData = ByteBuffer.allocate(4 + origData.length);
        newData.order(ByteOrder.LITTLE_ENDIAN);
        newData.putInt(message.getMessageType());
        newData.put(origData);
        return NanoAppMessage.createMessageToNanoApp(
                message.getNanoAppId(), message.getMessageType(), newData.array());
    }

    // TODO: Remove this hack
    private NanoAppMessage hackMessageFromNanoApp(NanoAppMessage message) {
        // For now, our nanohub HAL and JNI code end up not sending across the
        // message type of the user correctly.  So our testing protocol hacks
        // around this by putting the message type in the first four bytes of
        // the data payload, in little endian.
        ByteBuffer origData = ByteBuffer.wrap(message.getMessageBody());
        origData.order(ByteOrder.LITTLE_ENDIAN);
        int newMessageType = origData.getInt();
        // The new data is the remainder of this array (which could be empty).
        byte[] newData = new byte[origData.remaining()];
        origData.get(newData);
        return NanoAppMessage.createMessageFromNanoApp(
                message.getNanoAppId(), newMessageType, newData,
                message.isBroadcastMessage());
    }
}
