/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "gtest/gtest.h"

#include "chre/target_platform/platform_sensor_util.h"

using chre::intervalToSmgrReportRate;
using chre::Milliseconds;
using chre::Nanoseconds;
using chre::Seconds;

TEST(SmgrReportRateTest, Zero) {
  uint16_t rate = intervalToSmgrReportRate(Nanoseconds(0));
  EXPECT_EQ(rate, 0);
}

TEST(SmgrReportRateTest, FiftyHertz) {
  constexpr Nanoseconds kTenHertzInterval(Milliseconds(20));
  uint16_t rate = intervalToSmgrReportRate(kTenHertzInterval);
  EXPECT_EQ(rate, 50);
}

TEST(SmgrReportRateTest, ZeroPointFiveHertz) {
  constexpr Nanoseconds kZeroPointFiveHertzInterval(Seconds(2));
  uint16_t rate = intervalToSmgrReportRate(kZeroPointFiveHertzInterval);
  EXPECT_EQ(rate, 2000);
}
