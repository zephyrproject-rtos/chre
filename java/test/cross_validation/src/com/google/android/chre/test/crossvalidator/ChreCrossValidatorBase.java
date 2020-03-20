/*
 * Copyright (C) 2019 The Android Open Source Project
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

package com.google.android.chre.test.crossvalidator;

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
import org.junit.Assume;

import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Base class for CHRE cross validators objects. Handles the basics of data flow from CHRE nanoapp
 * and waiting on data with timeouts without getting into specifics of how to parse data and how to
 * compare it. That responsibility lies with the subclass that should impelment the abstract
 * methods defined at the end of this class.
 */
/*package*/
abstract class ChreCrossValidatorBase {
    protected static final String TAG = "ChreCrossValidator";
    private static final long NANO_APP_ID = 0x476f6f6754000002L;
    private static final long SAMPLING_DURATION_IN_MS = 5000;

    private final ContextHubManager mContextHubManager;
    private final ContextHubClient mContextHubClient;
    private final ContextHubInfo mContextHubInfo;
    protected final NanoAppBinary mNappBinary;

    private final CountDownLatch mAwaitDataLatch = new CountDownLatch(1);

    private final AtomicReference<String> mErrorStr = new AtomicReference<String>();
    protected AtomicBoolean mCollectingData = new AtomicBoolean(false);

    /**
    * Construct ChreCrossValidatorBase object with configuration that will parse messages back from
    * the nanoapp assuming they are datapoints to compare against AP data.
    *
    * @param contextHubManager The manager of the system context hub.
    * @param contextHubInfo The info of the context hub that will be used.
    * @param nanoAppBinary The "chre_cross_validator.napp" nano app binary that will be used
    *      to request chre side data.
    */
    /*package*/
    ChreCrossValidatorBase(ContextHubManager contextHubManager, ContextHubInfo contextHubInfo,
              NanoAppBinary nanoAppBinary) {
        Assume.assumeTrue("Nanoapp given to cross validator is not the designated chre cross"
                + " validation nanoapp.",
                nanoAppBinary.getNanoAppId() == NANO_APP_ID);
        mContextHubManager = contextHubManager;
        mContextHubInfo = contextHubInfo;
        mNappBinary = nanoAppBinary;
        ContextHubClientCallback callback = new ContextHubClientCallback() {
            @Override
            public void onMessageFromNanoApp(ContextHubClient client, NanoAppMessage message) {
                if (mCollectingData.get() && message.getNanoAppId() == mNappBinary.getNanoAppId()) {
                    parseDataFromNanoAppMessage(message);
                }
            }

            @Override
            public void onHubReset(ContextHubClient client) {
                setErrorStr("Context Hub reset occurred");
            }
        };
        mContextHubClient = mContextHubManager.createClient(mContextHubInfo, callback);
    }

    /**
     * Initialize cross validator in preparation for a call to validate method. Subclasses should
     * override this method and add any additional init steps after a call to super.init() if
     * needed. Should be called in @Before methods of tests.
     */
    public void init() throws AssertionError {
        unloadAllNanoApps();
        // Load cross validator nanoapp
        ChreTestUtil.loadNanoAppAssertSuccess(mContextHubManager, mContextHubInfo, mNappBinary);
    }

    /**
     * Validate the data from AP and CHRE according to the parameters passed to this cross
     * validator. Should be called in @Test methods of tests.
     *
     * @param samplingDurationInMs The amount of time in milliseconds to collect samples from AP and
     * CHRE.
     */
    public void validate() throws AssertionError {
        collectDataFromAp();
        collectDataFromChre();
        waitForDataSampling();
        assertApAndChreDataSimilar();
    }

    /**
    * Clean up resources allocated for cross validation. Subclasses should override this method and
    * add any additional deinit steps after a call to super.deinit() if needed. Should be called in
    * @After methods of tests.
    */
    public void deinit() throws AssertionError {
        mCollectingData.set(false);
        // Unload cross validator nanoapp
        ChreTestUtil.unloadNanoAppAssertSuccess(
                mContextHubManager, mContextHubInfo, mNappBinary.getNanoAppId());
        closeContextHubConnection();
        unregisterApDataListener();
    }

    /**
    * Unloads all nanoapps from device. Call before validating data to ensure no inconsistencies
    * with data received.
    */
    private void unloadAllNanoApps() {
        List<NanoAppState> nanoAppStateList =
                ChreTestUtil.queryNanoAppsAssertSuccess(mContextHubManager, mContextHubInfo);

        for (NanoAppState state : nanoAppStateList) {
            ChreTestUtil.unloadNanoAppAssertSuccess(
                    mContextHubManager, mContextHubInfo, state.getNanoAppId());
            Log.d(TAG, String.format("Unloaded napp: 0x%X", state.getNanoAppId()));
        }
    }

    /**
    * Start collecting data from AP
    */
    private void collectDataFromAp() {
        registerApDataListener();
    }

    /**
    * Start collecting data from CHRE
    */
    private void collectDataFromChre() throws AssertionError {
        // The info in the start message will inform the nanoapp of which type of
        // data to collect (accel, gyro, gnss, wifi, etc).
        int result = mContextHubClient.sendMessageToNanoApp(makeStartNanoAppMessage());
        if (result != ContextHubTransaction.RESULT_SUCCESS) {
            Assert.fail("Collect data from CHRE failed with result "
                    + contextHubTransactionResultToString(result)
                    + " while trying to send start message.");
        }
    }

    /**
    * Wait for AP and CHRE data to be fully collected or timeouts occur. collectDataFromAp and
    * collectDataFromChre methods should both be called before this.
    *
    * @param samplingDurationInMs The amount of time to wait for AP and CHRE to collected data in
    * ms.
    */
    private void waitForDataSampling() throws AssertionError {
        mCollectingData.set(true);
        try {
            mAwaitDataLatch.await(SAMPLING_DURATION_IN_MS, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            Assert.fail("await data latch interrupted");
        }
        if (mErrorStr.get() != null) {
            Assert.fail(mErrorStr.get());
        }
        mCollectingData.set(false);
    }

    // Private helpers below

    /**
    * Close the context hub client connection.
    */
    private void closeContextHubConnection() {
        mContextHubClient.close();
    }

    /**
    * Stop data collection by counting down the await data latch and providing an error that will be
    * logged.
    *
    * @param errorStr The string used to describe the error.
    */
    protected void setErrorStr(String errorStr) {
        mErrorStr.set(errorStr);
        mAwaitDataLatch.countDown();
    }

    /**
    * Waits on a CountDownLatch or assert if it timed out or was interrupted.
    *
    * @param latch                       the CountDownLatch
    * @param timeout                     the timeout duration
    * @param unit                        the timeout unit
    * @param timeoutErrorMessage         the message to display on timeout assert
    * @param interruptedExceptionMessage the message to display on InterruptedException assert
    */
    private static void awaitCountDownLatchAssertOnFailure(CountDownLatch latch, long timeout,
            TimeUnit unit, String timeoutErrorMessage, String interruptedExceptionMessage) {
        boolean result = false;
        try {
            result = latch.await(timeout, unit);
        } catch (InterruptedException e) {
            Assert.fail(interruptedExceptionMessage);
        }

        Assert.assertTrue(timeoutErrorMessage, result);
    }

    /**
    * @return the name of the context hub result.
    */
    private static String contextHubTransactionResultToString(int result) {
        switch (result) {
            case ContextHubTransaction.RESULT_SUCCESS:
                return "RESULT_SUCCESS";
            case ContextHubTransaction.RESULT_FAILED_UNKNOWN:
                return "RESULT_FAILED_UNKNOWN";
            case ContextHubTransaction.RESULT_FAILED_BAD_PARAMS:
                return "RESULT_FAILED_BAD_PARAMS";
            case ContextHubTransaction.RESULT_FAILED_UNINITIALIZED:
                return "RESULT_FAILED_UNINITIALIZED";
            case ContextHubTransaction.RESULT_FAILED_BUSY:
                return "RESULT_FAILED_BUSY";
            case ContextHubTransaction.RESULT_FAILED_AT_HUB:
                return "RESULT_FAILED_AT_HUB";
            case ContextHubTransaction.RESULT_FAILED_TIMEOUT:
                return "RESULT_FAILED_TIMEOUT";
            case ContextHubTransaction.RESULT_FAILED_SERVICE_INTERNAL_FAILURE:
                return "RESULT_FAILED_SERVICE_INTERNAL_FAILURE";
            case ContextHubTransaction.RESULT_FAILED_HAL_UNAVAILABLE:
                return "RESULT_FAILED_HAL_UNAVAILABLE";
            default:
                return "UNKNOWN_RESULT";
        }
    }

    // Methods below implemented by concrete subclasses

    /**
    * @return The nanoapp message used to start the data collection in chre
    */
    protected abstract NanoAppMessage makeStartNanoAppMessage();

    /**
    * Extract the specific data that a cross validation test needs from
    * a nanoapp message and save it for future comparison.
    *
    * @param message The message that will be parsed for data
    */
    protected abstract void parseDataFromNanoAppMessage(NanoAppMessage message);

    /**
    * Registers this object to listen for data from the AP using the framework APIs.
    */
    protected abstract void registerApDataListener();

    /**
    * Unregister any AP-side Data Event listeners.
    */
    protected abstract void unregisterApDataListener();

    /**
    * @return true if the data from AP and CHRE are considered close enough to be reliable.
    */
    protected abstract void assertApAndChreDataSimilar() throws AssertionError;
}
