/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "chre/core/gnss_request.h"
#include "chre/util/time.h"

using chre::GnssRequest;
using chre::Milliseconds;

TEST(GnssRequest, DefaultMinimalPriority) {
  GnssRequest request;
  EXPECT_FALSE(request.isEnabled());
  EXPECT_EQ(request.getMinInterval().getMilliseconds(), UINT64_MAX);
}

TEST(GnssRequest, LowerIntervalIsHigherPriority) {
  GnssRequest lowFrequencyRequest(Milliseconds(1000));
  GnssRequest highFrequencyRequest(Milliseconds(500));

  GnssRequest mergedRequest;
  EXPECT_TRUE(mergedRequest.mergeWith(lowFrequencyRequest));
  EXPECT_TRUE(mergedRequest.mergeWith(highFrequencyRequest));
  EXPECT_EQ(mergedRequest.getMinInterval(), Milliseconds(500));
}

TEST(GnssRequest, HigherIntervalIsNotHigherPriority) {
  GnssRequest lowFrequencyRequest(Milliseconds(1000));
  GnssRequest highFrequencyRequest(Milliseconds(500));

  GnssRequest mergedRequest;
  EXPECT_TRUE(mergedRequest.mergeWith(highFrequencyRequest));
  EXPECT_FALSE(mergedRequest.mergeWith(lowFrequencyRequest));
  EXPECT_EQ(mergedRequest.getMinInterval(), Milliseconds(500));
}
