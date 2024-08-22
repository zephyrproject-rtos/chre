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

package com.google.android.chre.test.setting;

import static androidx.test.espresso.intent.matcher.IntentMatchers.hasExtra;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;

import android.app.Instrumentation;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.location.NanoAppBinary;
import android.util.Log;
import androidx.test.platform.app.InstrumentationRegistry;

import com.google.android.chre.nanoapp.proto.ChreSettingsTest;
import com.google.android.utils.chre.SettingsUtil;

import org.hamcrest.Matcher;
import org.hamcrest.core.AllOf;
import org.mockito.hamcrest.MockitoHamcrest;

/** A test to check for behavior when Bluetooth settings are changed. */
public class ContextHubBleSettingsTestExecutor {
    private static final String TAG = "ContextHubBleSettingsTestExecutor";

    private final Context mContext =
            InstrumentationRegistry.getInstrumentation().getTargetContext();

    private final BluetoothAdapter mAdapter;

    private final ContextHubSettingsTestExecutor mExecutor;

    private final SettingsUtil mSettingsUtil;

    private boolean mInitialBluetoothEnabled;
    private boolean mInitialScanningEnabled;
    private boolean mInitialAirplaneMode;

    public ContextHubBleSettingsTestExecutor(NanoAppBinary binary) {
        mAdapter = mContext.getSystemService(BluetoothManager.class).getAdapter();
        mExecutor = new ContextHubSettingsTestExecutor(binary);
        mSettingsUtil = new SettingsUtil(mContext);
    }

    /** Should be called in a @Before method. */
    public void setUp() throws InterruptedException {
        mInitialBluetoothEnabled = mSettingsUtil.isBluetoothEnabled();
        mInitialScanningEnabled = mSettingsUtil.isBluetoothScanningAlwaysEnabled();
        mInitialAirplaneMode = mSettingsUtil.isAirplaneModeOn();
        Log.d(
                TAG,
                ("isBluetoothEnabled=" + mInitialBluetoothEnabled)
                        + (" isBluetoothScanningEnabled=" + mInitialScanningEnabled)
                        + (" isAirplaneModeOn=" + mInitialAirplaneMode));
        mSettingsUtil.setAirplaneMode(false /* enable */);
        mExecutor.init();
    }

    public void runBleScanningTest() throws InterruptedException {
        runBleScanningTest(false /* enableBluetooth */, false /* enableScanning */);
        runBleScanningTest(true /* enableBluetooth */, false /* enableScanning */);
        runBleScanningTest(false /* enableBluetooth */, true /* enableScanning */);
        runBleScanningTest(true /* enableBluetooth */, true /* enableScanning */);
    }

    /** Should be called in an @After method. */
    public void tearDown() throws InterruptedException {
        Log.d(TAG, "tearDown");
        mExecutor.deinit();
        mSettingsUtil.setBluetooth(mInitialBluetoothEnabled);
        mSettingsUtil.setBluetoothScanningSettings(mInitialScanningEnabled);
        mSettingsUtil.setAirplaneMode(mInitialAirplaneMode);
    }

    @SafeVarargs
    private void verifyIntentReceived(BroadcastReceiver receiver, Matcher<Intent>... matchers) {
        verify(receiver, timeout(10_000))
                .onReceive(any(Context.class), MockitoHamcrest.argThat(AllOf.allOf(matchers)));
    }

    private int getBluetoothState() {
        if (mAdapter.getState() == BluetoothAdapter.STATE_ON) {
            return BluetoothAdapter.STATE_ON;
        } else if (mAdapter.isLeEnabled()) {
            return BluetoothAdapter.STATE_BLE_ON;
        } else {
            return BluetoothAdapter.STATE_OFF;
        }
    }

    /** return true if a state change occurred */
    private void setBluetoothMode(int wantedState) {
        BroadcastReceiver receiver = mock(BroadcastReceiver.class);
        mContext.registerReceiver(
                receiver, new IntentFilter(BluetoothAdapter.ACTION_BLE_STATE_CHANGED));
        try {
            if (wantedState == getBluetoothState()) {
                Log.d(TAG, "Bluetooth already in " + BluetoothAdapter.nameForState(wantedState));
                return;
            }

            switch (wantedState) {
                case BluetoothAdapter.STATE_ON -> {
                    mSettingsUtil.setBluetooth(true);
                }
                case BluetoothAdapter.STATE_BLE_ON -> {
                    if (!mAdapter.isBleScanAlwaysAvailable()) {
                        try {
                            // Wait to ensure settings is propagated to Bluetooth
                            Thread.sleep(1000);
                        } catch (InterruptedException e) {
                            assertWithMessage(e.getMessage()).fail();
                        }
                    }
                    // staying in BLE_ON is not possible without the scan setting
                    assertThat(mAdapter.isBleScanAlwaysAvailable()).isTrue();
                    // When Bluetooth is ON, calling enableBLE will not do anything on its own. We
                    // also need to disable the classic Bluetooth
                    assertThat(mAdapter.enableBLE()).isTrue();
                    mSettingsUtil.setBluetooth(false);
                }
                case BluetoothAdapter.STATE_OFF -> {
                    mSettingsUtil.setBluetooth(false);
                }
            }

            verifyIntentReceived(receiver, hasExtra(BluetoothAdapter.EXTRA_STATE, wantedState));
        } finally {
            mContext.unregisterReceiver(receiver);
        }
    }

    void setScanningMode(boolean enableScanning) {
        if (enableScanning == mSettingsUtil.isBluetoothScanningAlwaysEnabled()) {
            Log.d(TAG, "Scanning is already in the expected mode: " + enableScanning);
            return;
        }

        Log.d(TAG, "Setting scanning into: " + enableScanning);
        mSettingsUtil.setBluetoothScanningSettings(enableScanning);
    }

    /**
     * Helper function to run the test
     *
     * @param enableBluetooth if bluetooth is enabled
     * @param enableScanning if bluetooth scanning is always enabled
     */
    private void runBleScanningTest(boolean enableBluetooth, boolean enableScanning)
            throws InterruptedException {
        Log.d(TAG, "runTest: Bluetooth=" + enableBluetooth + " Scanning=" + enableScanning);
        setScanningMode(enableScanning);

        if (enableBluetooth) {
            setBluetoothMode(BluetoothAdapter.STATE_ON);
        } else if (enableScanning) {
            // If scanning just get toggle ON, it may take times to propagate and to allow BLE_ON…
            setBluetoothMode(BluetoothAdapter.STATE_BLE_ON);
        } else {
            setBluetoothMode(BluetoothAdapter.STATE_OFF);
        }

        try {
            // Wait to ensure settings are propagated to CHRE path
            Thread.sleep(2000);
        } catch (InterruptedException e) {
            assertWithMessage(e.getMessage()).fail();
        }

        assertThat(mSettingsUtil.isBluetoothEnabled()).isEqualTo(enableBluetooth);
        assertThat(mSettingsUtil.isBluetoothScanningAlwaysEnabled()).isEqualTo(enableScanning);

        boolean enableFeature = enableBluetooth || enableScanning;
        ChreSettingsTest.TestCommand.State state =
                enableFeature
                        ? ChreSettingsTest.TestCommand.State.ENABLED
                        : ChreSettingsTest.TestCommand.State.DISABLED;
        mExecutor.startTestAssertSuccess(ChreSettingsTest.TestCommand.Feature.BLE_SCANNING, state);
    }
}
