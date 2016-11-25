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

#include "chre/util/fixed_size_vector.h"

using chre::FixedSizeVector;

TEST(FixedSizeVector, EmptyWithCapacityWithDefault) {
  FixedSizeVector<int, 8> testVector;
  ASSERT_NE(testVector.data(), nullptr);
  ASSERT_EQ(testVector.size(), 0);
  ASSERT_EQ(testVector.capacity(), 8);
  ASSERT_TRUE(testVector.empty());
  ASSERT_FALSE(testVector.full());
}

TEST(FixedSizeVector, PushBackOneAndRead) {
  FixedSizeVector<int, 8> testVector;
  testVector.push_back(0x1337);
  ASSERT_NE(testVector.data(), nullptr);
  ASSERT_EQ(testVector.size(), 1);
  ASSERT_EQ(testVector.capacity(), 8);
  ASSERT_FALSE(testVector.empty());
  ASSERT_FALSE(testVector.full());
  ASSERT_EQ(testVector[0], 0x1337);
}

TEST(FixedSizeVector, PushBackUntilFullAndRead) {
  FixedSizeVector<int, 4> testVector;
  testVector.push_back(1000);
  testVector.push_back(2000);
  testVector.push_back(3000);
  testVector.push_back(4000);

  ASSERT_NE(testVector.data(), nullptr);
  ASSERT_TRUE(testVector.full());
  ASSERT_FALSE(testVector.empty());
  ASSERT_EQ(testVector.size(), 4);
  ASSERT_EQ(testVector[0], 1000);
  ASSERT_EQ(testVector[1], 2000);
  ASSERT_EQ(testVector[2], 3000);
  ASSERT_EQ(testVector[3], 4000);

  ASSERT_EQ(testVector.data()[0], 1000);
  ASSERT_EQ(testVector.data()[1], 2000);
  ASSERT_EQ(testVector.data()[2], 3000);
  ASSERT_EQ(testVector.data()[3], 4000);
}
