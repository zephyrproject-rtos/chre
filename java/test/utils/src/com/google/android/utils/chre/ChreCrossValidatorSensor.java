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

import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.hardware.location.ContextHubInfo;
import android.hardware.location.ContextHubManager;
import android.hardware.location.NanoAppBinary;
import android.hardware.location.NanoAppMessage;
import java.util.ArrayList;
import java.util.List;

public class ChreCrossValidatorSensor
    extends ChreCrossValidatorBase implements SensorEventListener {
  private List<SensorDatapoint> mApDatapoints;
  private List<SensorDatapoint> mChreDatapoints;

  private SensorManager mSensorManager;
  private Sensor mSensor;

  private int mSensorType;

  /*
   * @param contextHubManager The context hub manager that will be passed to super ctor.
   * @param contextHubInfo The context hub info that will be passed to super ctor.
   * @param nappAppBinary The nanoapp binary that will be passed to super ctor.
   * @param sensorType The sensor type that this sensor validator will validate against. This must
   *     be one of the int constants starting with TYPE_ defined in android.hardware.Sensor class.
   */
  public ChreCrossValidatorSensor(ContextHubManager contextHubManager,
      ContextHubInfo contextHubInfo, NanoAppBinary nanoAppBinary, int sensorType) {
    super(contextHubManager, contextHubInfo, nanoAppBinary);
    mApDatapoints = new ArrayList<SensorDatapoint>();
    mChreDatapoints = new ArrayList<SensorDatapoint>();
    mSensorType = sensorType;
  }

  @Override
  protected NanoAppMessage makeStartNanoAppMessage() {
    // TODO: Implement
    return NanoAppMessage.createMessageToNanoApp(0, 0, new byte[0]);
  }

  @Override
  protected void parseDataFromNanoAppMessage(NanoAppMessage message) {
    // TODO: Implement
  }

  @Override
  protected void registerApDataListener() {
    // TODO: Implement
  }

  @Override
  protected void unregisterApDataListener() {
    // TODO: Implement
  }

  @Override
  protected void assertApAndChreDataSimilar() throws AssertionError {
    // TODO: Implement
  }

  @Override
  public void onSensorChanged(SensorEvent event) {
    // TODO: Implement
  }

  @Override
  public void onAccuracyChanged(Sensor accel, int accuracy) {
    // TODO: Implement
  }
}
