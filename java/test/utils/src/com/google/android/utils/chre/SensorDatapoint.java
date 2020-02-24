/*
 * Copyright (C) 2020 Google LLC.
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
package com.google.android.utils.chre;

import android.hardware.SensorEvent;

/*
 * Class that all types of sensor datapoints inherit from which supports comparison to another
 * datapoint via a static method. Objects of this type are used in the sensor CHRE cross validator.
 */
public class SensorDatapoint {
  private long mTimestamp;
  private float[] mValues;

  /*package*/
  SensorDatapoint(long timestamp, float[] values) {
    mTimestamp = timestamp;
    mValues = values;
  }

  /*package*/
  SensorDatapoint(SensorEvent sensorEvent) {
    mTimestamp = sensorEvent.timestamp;
    mValues = sensorEvent.values;
  }

  /*package*/
  static boolean datapointsAreSimilar(SensorDatapoint dp1, SensorDatapoint dp2) {
    // TODO: Implement
    return false;
  }
}
