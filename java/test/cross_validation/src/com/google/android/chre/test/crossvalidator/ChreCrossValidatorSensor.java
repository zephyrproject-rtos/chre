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
package com.google.android.chre.test.crossvalidator;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.hardware.location.ContextHubInfo;
import android.hardware.location.ContextHubManager;
import android.hardware.location.NanoAppBinary;
import android.hardware.location.NanoAppMessage;

import androidx.test.InstrumentationRegistry;

import com.google.android.chre.nanoapp.proto.ChreCrossValidation;
import com.google.android.utils.chre.ChreTestUtil;
import com.google.protobuf.InvalidProtocolBufferException;

import org.junit.Assert;
import org.junit.Assume;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;


public class ChreCrossValidatorSensor
        extends ChreCrossValidatorBase implements SensorEventListener {
    /**
    * Contains the information required for each sensor type to validate data.
    */
    private static class SensorTypeInfo {
        public final int sensorType;
        // The length of the data samples in floats that is expected for a sensor type
        public final int expectedValuesLength;
        // The amount that each value in the values array of a certain datapoint can differ between
        // AP and CHRE
        public final float errorMargin;

        SensorTypeInfo(int sensorType, int expectedValuesLength, float errorMargin) {
            this.sensorType = sensorType;
            this.expectedValuesLength = expectedValuesLength;
            this.errorMargin = errorMargin;
        }
    }

    // The portion of datapoints that can be thrown away before while still searching for first
    // datapoints that are similar between CHRE and AP
    private static final double ALIGNMENT_JUNK_FACTOR = 2.0 / 3;

    // TODO(b/146052784): May need to account for differences in sampling rate and latency from
    // AP side vs CHRE side
    private static final long SAMPLING_INTERVAL_IN_MS = 20;
    private static final long SAMPLING_LATENCY_IN_MS = 0;

    private List<SensorDatapoint> mApDatapoints;
    private List<SensorDatapoint> mChreDatapoints;

    private SensorManager mSensorManager;
    private Sensor mSensor;

    private SensorTypeInfo mSensorTypeInfo;

    private static final Map<Integer, SensorTypeInfo> SENSOR_TYPE_TO_INFO =
            makeSensorTypeToInfoMap();

    /*
    * @param contextHubManager The context hub manager that will be passed to super ctor.
    * @param contextHubInfo The context hub info that will be passed to super ctor.
    * @param nappAppBinary The nanoapp binary that will be passed to super ctor.
    * @param sensorType The sensor type that this sensor validator will validate against. This must
    *     be one of the int constants starting with TYPE_ defined in android.hardware.Sensor class.
    */
    public ChreCrossValidatorSensor(ContextHubManager contextHubManager,
            ContextHubInfo contextHubInfo, NanoAppBinary nanoAppBinary, int sensorType)
            throws AssertionError {
        super(contextHubManager, contextHubInfo, nanoAppBinary);
        mApDatapoints = new ArrayList<SensorDatapoint>();
        mChreDatapoints = new ArrayList<SensorDatapoint>();
        Assert.assertTrue(String.format("Sensor type %d is not recognized", sensorType),
                isSensorTypeValid(sensorType));
        mSensorTypeInfo = SENSOR_TYPE_TO_INFO.get(sensorType);
    }

    @Override
    protected NanoAppMessage makeStartNanoAppMessage() {
        int messageType = ChreCrossValidation.MessageType.CHRE_CROSS_VALIDATION_START_VALUE;
        ChreCrossValidation.StartSensorCommand startSensor =
                ChreCrossValidation.StartSensorCommand.newBuilder()
                .setSamplingIntervalInNs(TimeUnit.MILLISECONDS.toNanos(SAMPLING_INTERVAL_IN_MS))
                .setSamplingMaxLatencyInNs(TimeUnit.MILLISECONDS.toNanos(SAMPLING_LATENCY_IN_MS))
                .setSensorType(ChreCrossValidation.SensorType.forNumber(mSensorTypeInfo.sensorType))
                .build();
        ChreCrossValidation.StartCommand startCommand =
                ChreCrossValidation.StartCommand.newBuilder().setStartSensorCommand(startSensor)
                .build();
        return NanoAppMessage.createMessageToNanoApp(
                mNappBinary.getNanoAppId(), messageType, startCommand.toByteArray());
    }

    @Override
    protected void parseDataFromNanoAppMessage(NanoAppMessage message) {
        final String kParseDataErrorPrefix = "While parsing data from nanoapp: ";
        ChreCrossValidation.Data dataProto;
        try {
            dataProto = ChreCrossValidation.Data.parseFrom(message.getMessageBody());
        } catch (InvalidProtocolBufferException e) {
            setErrorStr("Error parsing protobuff: " + e);
            return;
        }
        if (!dataProto.hasSensorData()) {
            setErrorStr(kParseDataErrorPrefix + "found non sensor type data");
        } else {
            ChreCrossValidation.SensorData sensorData = dataProto.getSensorData();
            int sensorType = sensorData.getSensorType().getNumber();
            if (sensorType != mSensorTypeInfo.sensorType) {
                setErrorStr(
                        String.format(kParseDataErrorPrefix
                        + "incorrect sensor type %d when expecting %d",
                        sensorType, mSensorTypeInfo.sensorType));
            } else {
                for (ChreCrossValidation.SensorDatapoint datapoint :
                        sensorData.getDatapointsList()) {
                    int valuesLength = datapoint.getValuesList().size();
                    if (valuesLength != mSensorTypeInfo.expectedValuesLength) {
                        setErrorStr(String.format(kParseDataErrorPrefix
                                + "incorrect sensor datapoints values length %d when expecing %d",
                                sensorType, valuesLength, mSensorTypeInfo.expectedValuesLength));
                        break;
                    }
                    SensorDatapoint newDatapoint = new SensorDatapoint(datapoint, sensorType);
                    mChreDatapoints.add(newDatapoint);
                }
            }
        }
    }

    @Override
    protected void registerApDataListener() {
        mSensorManager =
                (SensorManager) InstrumentationRegistry.getInstrumentation().getContext()
                .getSystemService(Context.SENSOR_SERVICE);
        Assert.assertNotNull("Sensor manager could not be instantiated.", mSensorManager);
        mSensor = mSensorManager.getDefaultSensor(mSensorTypeInfo.sensorType);
        Assume.assumeNotNull(String.format("Sensor could not be instantiated for sensor type %d.",
                mSensorTypeInfo.sensorType),
                mSensor);
        Assert.assertTrue(mSensorManager.registerListener(
                this, mSensor, (int) TimeUnit.MILLISECONDS.toMicros(SAMPLING_INTERVAL_IN_MS)));
    }

    @Override
    protected void unregisterApDataListener() {
        mSensorManager.unregisterListener(this);
    }

    @Override
    protected void assertApAndChreDataSimilar() throws AssertionError {
        Assert.assertTrue("Did not find any CHRE datapoints", !mChreDatapoints.isEmpty());
        Assert.assertTrue("Did not find any AP datapoints", !mApDatapoints.isEmpty());
        alignApAndChreDatapoints();
        // AP and CHRE datapoints will be same size
        // TODO(b/146052784): Ensure that CHRE data is the same sampling rate as AP data for
        // comparison
        for (int i = 0; i < mApDatapoints.size(); i++) {
            SensorDatapoint apDp = mApDatapoints.get(i);
            SensorDatapoint chreDp = mChreDatapoints.get(i);
            String datapointsAssertMsg =
                    String.format("AP and CHRE three axis datapoint values differ on index %d", i)
                    + "\nAP data -> " + apDp + "\nCHRE data -> "
                    + chreDp;
            String timestampsAssertMsg =
                    String.format("AP and CHRE three axis timestamp values differ on index %d", i)
                    + "\nAP data -> " + apDp + "\nCHRE data -> "
                    + chreDp;

            // TODO(b/146052784): Log full list of datapoints to file on disk on assertion failure
            // so that there is more insight into the problem then just logging the one pair of
            // datapoints
            Assert.assertTrue(datapointsAssertMsg,
                    SensorDatapoint.datapointsAreSimilar(
                    apDp, chreDp, mSensorTypeInfo.errorMargin));
            Assert.assertTrue(timestampsAssertMsg,
                    SensorDatapoint.timestampsAreSimilar(apDp, chreDp));
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {}

    @Override
    public void onSensorChanged(SensorEvent event) {
        if (mCollectingData.get()) {
            mApDatapoints.add(new SensorDatapoint(event));
        }
    }

    @Override
    public void init() throws AssertionError {
        super.init();
        restrictSensors();
    }

    @Override
    public void deinit() throws AssertionError {
        super.deinit();
        unrestrictSensors();
    }

    /*
    * @param sensorType The sensor type that was passed to the ctor that will be validated.
    * @return true if sensor type is recognized.
    */
    private static boolean isSensorTypeValid(int sensorType) {
        return SENSOR_TYPE_TO_INFO.containsKey(sensorType);
    }

    /**
    * Make the sensor type info objects for each sensor type and map from sensor type to those
    * objects.
    *
    * @return The map from sensor type to info for that type.
    */
    private static Map<Integer, SensorTypeInfo> makeSensorTypeToInfoMap() {
        Map<Integer, SensorTypeInfo> map = new HashMap<Integer, SensorTypeInfo>();
        map.put(Sensor.TYPE_ACCELEROMETER, new SensorTypeInfo(Sensor.TYPE_ACCELEROMETER, 3, 0.01f));
        return map;
    }

    /*
    * Align the AP and CHRE datapoints by finding the first pair that are similar comparing
    * linearly from there. Also truncate the end if one list has more datapoints than the other
    * after this. This is needed because AP and CHRE can start sending data and varying times to
    * this validator and can also stop sending at various times.
    */
    private void alignApAndChreDatapoints() throws AssertionError {
        int matchAp = 0, matchChre = 0;
        int apIndex = 0, chreIndex = 0;
        boolean foundMatch = false;
        int discardableSize = (int) (mApDatapoints.size() * ALIGNMENT_JUNK_FACTOR);
        // if the start point of alignment exceeds halfway down either list then this is considered
        // not enough alignment for datapoints to be valid
        while (apIndex < discardableSize && chreIndex < discardableSize) {
            SensorDatapoint apDp = mApDatapoints.get(apIndex);
            SensorDatapoint chreDp = mChreDatapoints.get(chreIndex);
            if (SensorDatapoint.timestampsAreSimilar(apDp, chreDp)) {
                matchAp = apIndex;
                matchChre = chreIndex;
                foundMatch = true;
                break;
            }
            if ((apDp.getTimestamp() < chreDp.getTimestamp()) && (apIndex < discardableSize - 1)) {
                apIndex++;
            } else {
                chreIndex++;
            }
        }
        Assert.assertTrue("Did not find matching timestamps to align AP and CHRE datapoints.",
                foundMatch);
        // Remove extraneous datapoints before matching datapoints
        mApDatapoints.removeAll(mApDatapoints.subList(0, matchAp));
        mChreDatapoints.removeAll(mChreDatapoints.subList(0, matchChre));
        // Remove extraneous datapoints from of end of larger list
        if (mApDatapoints.size() < mChreDatapoints.size()) {
            mChreDatapoints.removeAll(mChreDatapoints.subList(mApDatapoints.size(),
                    mChreDatapoints.size() + 1));
        } else if (mApDatapoints.size() > mChreDatapoints.size()) {
            mApDatapoints.removeAll(mApDatapoints.subList(mChreDatapoints.size(),
                    mApDatapoints.size() + 1));
        }
    }

    /**
    * Restrict other applications from accessing sensors. Should be called before validating data.
    */
    private void restrictSensors() {
        ChreTestUtil.executeShellCommand(InstrumentationRegistry.getInstrumentation(),
                "dumpsys sensorservice restrict ChreCrossValidatorSensor");
    }

    /**
    * Unrestrict other applications from accessing sensors. Should be called after validating data.
    */
    private void unrestrictSensors() {
        ChreTestUtil.executeShellCommand(
                InstrumentationRegistry.getInstrumentation(), "dumpsys sensorservice enable");
    }
}
