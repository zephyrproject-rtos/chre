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
package com.google.android.chre.test.setting;

import android.app.Instrumentation;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.location.NanoAppBinary;

import androidx.test.InstrumentationRegistry;

import com.google.android.chre.nanoapp.proto.ChreSettingsTest;
import com.google.android.utils.chre.ChreTestUtil;

import org.junit.Assert;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * A test to check for behavior when WWAN settings are changed.
 */
public class ContextHubWwanSettingsTestExecutor {
    private final ContextHubSettingsTestExecutor mExecutor;

    private boolean mInitialAirplaneMode;

    private final Instrumentation mInstrumentation = InstrumentationRegistry.getInstrumentation();

    private CountDownLatch mCountDownLatch = null;

    public ContextHubWwanSettingsTestExecutor(NanoAppBinary binary) {
        mExecutor = new ContextHubSettingsTestExecutor(binary);

        Context context = InstrumentationRegistry.getTargetContext();
        IntentFilter intentFilter = new
                IntentFilter("android.intent.action.AIRPLANE_MODE_CHANGED");

        BroadcastReceiver receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (mCountDownLatch != null) {
                    mCountDownLatch.countDown();
                }
            }
        };

        context.registerReceiver(receiver, intentFilter);
    }

    /**
     * Should be called in a @Before method.
     */
    public void setUp() {
        mInitialAirplaneMode = isAirplaneModeOn();
        mExecutor.init();
    }

    public void runWwanTest() {
        runTest(false /* enableFeature */);
        runTest(true /* enableFeature */);
    }

    /**
     * Should be called in an @After method.
     */
    public void tearDown() {
        mExecutor.deinit();
        setAirplaneMode(mInitialAirplaneMode);
    }

    /**
     * Sets the airplane mode on the device.
     * @param enable True to enable airplane mode, false to disable it.
     */
    private void setAirplaneMode(boolean enable) {
        if (enable) {
            ChreTestUtil.executeShellCommand(
                    mInstrumentation, "cmd connectivity airplane-mode enable");
        } else {
            ChreTestUtil.executeShellCommand(
                    mInstrumentation, "cmd connectivity airplane-mode disable");
        }
    }

    /**
     * Helper function to run the test.
     * @param enableFeature True for enable.
     */
    private void runTest(boolean enableFeature) {
        mCountDownLatch = new CountDownLatch(1);
        boolean airplaneModeExpected = !enableFeature;
        setAirplaneMode(airplaneModeExpected);

        if (isAirplaneModeOn() != airplaneModeExpected) {
            try {
                mCountDownLatch.await(5, TimeUnit.SECONDS);
            } catch (InterruptedException e) {
                Assert.fail(e.getMessage());
            }
        }
        Assert.assertTrue(isAirplaneModeOn() == airplaneModeExpected);

        try {
            Thread.sleep(1000);  // wait for setting to propagate
        } catch (InterruptedException e) {
            Assert.fail(e.getMessage());
        }

        ChreSettingsTest.TestCommand.State state = enableFeature
                ? ChreSettingsTest.TestCommand.State.ENABLED
                : ChreSettingsTest.TestCommand.State.DISABLED;
        mExecutor.startTestAssertSuccess(
                ChreSettingsTest.TestCommand.Feature.WWAN_CELL_INFO, state);
    }

    /**
     * @return true if the airplane mode is currently enabled.
     */
    private boolean isAirplaneModeOn() {
        String out = ChreTestUtil.executeShellCommand(
                mInstrumentation, "settings get global airplane_mode_on");
        return ChreTestUtil.convertToIntegerOrFail(out) > 0;
    }
}
