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

import android.hardware.Sensor;
import android.hardware.SensorEvent;

import com.google.android.chre.nanoapp.proto.ChreCrossValidation;
import com.google.common.primitives.Floats;

import org.junit.Assert;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/*
 * Class that all types of sensor datapoints inherit from which supports comparison to another
 * datapoint via a static method. Objects of this type are used in the sensor CHRE cross validator.
 */
/*package*/
class SensorDatapoint {
    // The chreGetTimeOffset() function promises +/-10ms accuracy to actual AP time so allow this
    // much leeway for datapoint comparison.
    private static final long MAX_TIMESTAMP_DIFF_NS = 10000000L;
    private static final Integer[] VALID_SENSOR_TYPES_ARR = {Sensor.TYPE_ACCELEROMETER};
    private static final Set<Integer> VALID_SENSOR_TYPES =
            new HashSet<Integer>(Arrays.asList(VALID_SENSOR_TYPES_ARR));

    private long mTimestamp;
    private float[] mValues;
    private int mSensorType;

    /*
    * This is the CHRE datapoint ctor. Construct datapoint using a timestamp and float values
    * that were collected by CHRE.
    *
    * @param timestamp The timestamp for datapoint.
    * @param values The array of float values for datapoint.
    */
    /*package*/
    SensorDatapoint(ChreCrossValidation.SensorDatapoint datapoint, int sensorType) {
        mTimestamp = datapoint.getTimestampInNs();
        mValues = Floats.toArray(datapoint.getValuesList());
        mSensorType = sensorType;
        Assert.assertTrue(sensorTypeIsValid(mSensorType));
    }

    /*
    * This is the AP datapoint ctor. Construct a datapoint using the timestamp and floats observed
    * from Android framework.
    *
    * @param sensorEvent The sensor event that this datapoint info comes from.
    */
    /*package*/
    SensorDatapoint(SensorEvent sensorEvent) {
        mTimestamp = sensorEvent.timestamp;
        mValues = sensorEvent.values.clone();
        mSensorType = sensorEvent.sensor.getType();
        Assert.assertTrue(sensorTypeIsValid(mSensorType));
    }

    /*
    * @param dp1 The first SensorDatapoint object to compare.
    * @param dp2 The second SensorDatapoint object to compare.
    * @return true if the datapoint timestamps are similar.
    */
    /*package*/
    static boolean timestampsAreSimilar(SensorDatapoint dp1, SensorDatapoint dp2) {
        return Math.abs(dp1.mTimestamp - dp2.mTimestamp) < MAX_TIMESTAMP_DIFF_NS;
    }

    /*package*/
    long getTimestamp() {
        return mTimestamp;
    }

    /*
    * @return The human readable string representing this datapoint.
    */
    @Override
    public String toString() {
        String str = String.format("<SensorDatapoint timestamp: %d, values: [ ", mTimestamp);
        for (float val : mValues) {
            str += String.format("%f ", val);
        }
        str += "]";
        return str;
    }

    /*
    * @param dp1 The first SensorDatapoint object to compare.
    * @param dp2 The second SensorDatapoint object to compare.
    * @param errorMargin The amount that each value in values array can differ between the two
    *     datapoints.
    * @return true if the datapoint values are all similar.
    */
    /*package*/
    static boolean datapointsAreSimilar(SensorDatapoint dp1, SensorDatapoint dp2,
            float errorMargin) {
        Assert.assertEquals(dp1.mValues.length, dp2.mValues.length);
        for (int i = 0; i < dp1.mValues.length; i++) {
            float val1 = dp1.mValues[i];
            float val2 = dp2.mValues[i];
            float diff = Math.abs(val1 - val2);
            if (diff > errorMargin) {
                return false;
            }
        }
        return true;
    }

    /**
    * Check if a sensor type is valid for a SensorDatapoint object.
    *
    * @param sensorType The type of sensor found as static ints in android.hardware.Sensor class.
    * @return true if sensor type is a valid sensor found in VALID_SENSOR_TYPES.
    */
    private static boolean sensorTypeIsValid(int sensorType) {
        return VALID_SENSOR_TYPES.contains(sensorType);
    }
}
