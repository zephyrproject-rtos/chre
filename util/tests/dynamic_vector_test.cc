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

#include "chre/util/dynamic_vector.h"

using chre::DynamicVector;

TEST(DynamicVector, EmptyByDefault) {
  DynamicVector<int> vector;
  ASSERT_EQ(vector.data(), nullptr);
  ASSERT_EQ(vector.size(), 0);
  ASSERT_EQ(vector.capacity(), 0);
}

TEST(DynamicVector, PushBackAndRead) {
  DynamicVector<int> vector;
  ASSERT_TRUE(vector.push_back(0x1337));
  ASSERT_EQ(vector[0], 0x1337);
  ASSERT_EQ(vector.data()[0], 0x1337);
}

TEST(DynamicVector, PushBackReserveAndRead) {
  DynamicVector<int> vector;
  ASSERT_TRUE(vector.push_back(0x1337));
  ASSERT_TRUE(vector.push_back(0xface));
  ASSERT_TRUE(vector.reserve(4));
  ASSERT_EQ(vector[0], 0x1337);
  ASSERT_EQ(vector.data()[0], 0x1337);
  ASSERT_EQ(vector[1], 0xface);
  ASSERT_EQ(vector.data()[1], 0xface);
}

/**
 * A simple test helper object to count number of construction and destructions.
 */
class Foo {
 public:
  /**
   * Construct an object storing a simple integer. Increment the number of
   * objects that have been constructed of this type.
   */
  Foo(int value) : value(value) {
    sConstructedCounter++;
  }

  /**
   * Tear down the object, decrementing the number of objects that have been
   * constructed of this type.
   */
  ~Foo() {
    sConstructedCounter--;
  }

  //! The number of objects of this type that have been constructed.
  static size_t sConstructedCounter;

  //! The value stored in the object to verify the contents of this object after
  //! construction.
  int value;
};

//! Storage for the Foo reference counter.
size_t Foo::sConstructedCounter = 0;

TEST(DynamicVector, EmplaceBackAndDestruct) {
  {
    DynamicVector<Foo> vector;
    ASSERT_TRUE(vector.emplace_back(1000));
    ASSERT_TRUE(vector.emplace_back(2000));
    ASSERT_TRUE(vector.emplace_back(3000));
    ASSERT_TRUE(vector.emplace_back(4000));

    ASSERT_EQ(vector[0].value, 1000);
    ASSERT_EQ(vector.data()[0].value, 1000);
    ASSERT_EQ(vector[1].value, 2000);
    ASSERT_EQ(vector.data()[1].value, 2000);
    ASSERT_EQ(vector[2].value, 3000);
    ASSERT_EQ(vector.data()[2].value, 3000);
    ASSERT_EQ(vector[3].value, 4000);
    ASSERT_EQ(vector.data()[3].value, 4000);

    ASSERT_EQ(Foo::sConstructedCounter, 4);
  }

  ASSERT_EQ(Foo::sConstructedCounter, 0);
}
