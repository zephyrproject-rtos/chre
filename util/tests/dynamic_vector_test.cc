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

namespace {
constexpr int kMaxTestCapacity = 10;
int destructor_count[kMaxTestCapacity];

class Dummy {
 public:
  ~Dummy() {
    if (mValue >= 0) {
      destructor_count[mValue]++;
    }
  };
  void setValue(int value) {
    mValue = value;
  }

 private:
  int mValue = -1;
};
}

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

class MovableButNonCopyable : public chre::NonCopyable {
 public:
  MovableButNonCopyable(int value) : mValue(value) {}

  MovableButNonCopyable(MovableButNonCopyable&& other) {
    mValue = other.mValue;
  }

  MovableButNonCopyable& operator=(MovableButNonCopyable&& other) {
    mValue = other.mValue;
    return *this;
  }

  int getValue() const {
    return mValue;
  }

 private:
  int mValue;
};

TEST(DynamicVector, PushBackReserveAndReadMovableButNonCopyable) {
  DynamicVector<MovableButNonCopyable> vector;
  ASSERT_TRUE(vector.emplace_back(0x1337));
  ASSERT_TRUE(vector.emplace_back(0xface));
  ASSERT_TRUE(vector.reserve(4));
  EXPECT_EQ(vector[0].getValue(), 0x1337);
  EXPECT_EQ(vector.data()[0].getValue(), 0x1337);
  EXPECT_EQ(vector[1].getValue(), 0xface);
  EXPECT_EQ(vector.data()[1].getValue(), 0xface);
}

class CopyableButNonMovable {
 public:
  CopyableButNonMovable(int value) : mValue(value) {}

  CopyableButNonMovable(const CopyableButNonMovable& other) {
    mValue = other.mValue;
  }

  CopyableButNonMovable(CopyableButNonMovable&& other) = delete;

  CopyableButNonMovable& operator=(const CopyableButNonMovable& other) {
    mValue = other.mValue;
    return *this;
  }

  CopyableButNonMovable& operator=(CopyableButNonMovable&& other) = delete;

  int getValue() const {
    return mValue;
  }

 private:
  int mValue;
};

TEST(DynamicVector, PushBackReserveAndReadCopyableButNonMovable) {
  DynamicVector<CopyableButNonMovable> vector;
  ASSERT_TRUE(vector.emplace_back(0xcafe));
  ASSERT_TRUE(vector.emplace_back(0xface));
  ASSERT_TRUE(vector.reserve(4));
  EXPECT_EQ(vector[0].getValue(), 0xcafe);
  EXPECT_EQ(vector.data()[0].getValue(), 0xcafe);
  EXPECT_EQ(vector[1].getValue(), 0xface);
  EXPECT_EQ(vector.data()[1].getValue(), 0xface);
}

class MovableAndCopyable {
 public:
  MovableAndCopyable(int value) : mValue(value) {}

  MovableAndCopyable(const MovableAndCopyable& other) {
    mValue = other.mValue;
  }

  MovableAndCopyable(MovableAndCopyable&& other) {
    mValue = other.mValue;
  }

  MovableAndCopyable& operator=(const MovableAndCopyable& other) {
    mValue = other.mValue;
    return *this;
  }

  MovableAndCopyable& operator=(MovableAndCopyable&& other) {
    // The move operation multiplies the value by 2 so that we can see that the
    // move assignment operator was used.
    mValue = other.mValue * 2;
    return *this;
  }

  int getValue() const {
    return mValue;
  }

 private:
  int mValue;
};

TEST(DynamicVector, PushBackReserveAndReadMovableAndCopyable) {
  // Ensure that preference is given to std::move.
  DynamicVector<MovableAndCopyable> vector;

  // Reserve enough space for the first two elements.
  ASSERT_TRUE(vector.reserve(2));
  ASSERT_TRUE(vector.emplace_back(1000));
  ASSERT_TRUE(vector.emplace_back(2000));

  // Reserve more than enough space causing a move to be required.
  ASSERT_TRUE(vector.reserve(4));

  // Move on this type results in a multiplication by 2. Verify that all
  // elements have been multiplied by 2.
  EXPECT_EQ(vector[0].getValue(), 2000);
  EXPECT_EQ(vector.data()[0].getValue(), 2000);
  EXPECT_EQ(vector[1].getValue(), 4000);
  EXPECT_EQ(vector.data()[1].getValue(), 4000);
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

TEST(DynamicVector, InsertEmpty) {
  DynamicVector<int> vector;
  ASSERT_FALSE(vector.insert(1, 0x1337));
  ASSERT_TRUE(vector.insert(0, 0x1337));
  ASSERT_EQ(vector[0], 0x1337);
  ASSERT_EQ(vector.data()[0], 0x1337);
}

TEST(DynamicVector, PushBackInsertInMiddleAndRead) {
  DynamicVector<int> vector;
  ASSERT_TRUE(vector.push_back(0x1337));
  ASSERT_TRUE(vector.push_back(0xface));
  ASSERT_TRUE(vector.push_back(0xcafe));
  ASSERT_TRUE(vector.insert(1, 0xbeef));

  ASSERT_EQ(vector[0], 0x1337);
  ASSERT_EQ(vector.data()[0], 0x1337);
  ASSERT_EQ(vector[1], 0xbeef);
  ASSERT_EQ(vector.data()[1], 0xbeef);
  ASSERT_EQ(vector[2], 0xface);
  ASSERT_EQ(vector.data()[2], 0xface);
  ASSERT_EQ(vector[3], 0xcafe);
  ASSERT_EQ(vector.data()[3], 0xcafe);
}

TEST(DynamicVector, PushBackAndErase) {
  DynamicVector<int> vector;
  ASSERT_TRUE(vector.push_back(0x1337));
  ASSERT_TRUE(vector.push_back(0xcafe));
  ASSERT_TRUE(vector.push_back(0xbeef));
  ASSERT_TRUE(vector.push_back(0xface));

  vector.erase(1);

  ASSERT_EQ(vector[0], 0x1337);
  ASSERT_EQ(vector.data()[0], 0x1337);
  ASSERT_EQ(vector[1], 0xbeef);
  ASSERT_EQ(vector.data()[1], 0xbeef);
  ASSERT_EQ(vector[2], 0xface);
  ASSERT_EQ(vector.data()[2], 0xface);
  ASSERT_EQ(vector.size(), 3);
}

TEST(DynamicVector, FindEmpty) {
  DynamicVector<int> vector;
  ASSERT_EQ(vector.find(0), 0);
}

TEST(DynamicVector, FindWithElements) {
  DynamicVector<int> vector;
  ASSERT_TRUE(vector.push_back(0x1337));
  ASSERT_TRUE(vector.push_back(0xcafe));
  ASSERT_TRUE(vector.push_back(0xbeef));

  ASSERT_EQ(vector.find(0x1337), 0);
  ASSERT_EQ(vector.find(0xcafe), 1);
  ASSERT_EQ(vector.find(0xbeef), 2);
  ASSERT_EQ(vector.find(1000), 3);
}

TEST(DynamicVector, EraseDestructorCalled) {
  DynamicVector<Dummy> vector;
  for (size_t i = 0; i < 4; ++i) {
    vector.push_back(Dummy());
    vector[i].setValue(i);
  }

  // last item before erase is '3'.
  vector.erase(1);
  EXPECT_EQ(0, destructor_count[0]);
  EXPECT_EQ(0, destructor_count[1]);
  EXPECT_EQ(0, destructor_count[2]);
  EXPECT_EQ(1, destructor_count[3]);

  // last item before erase is still '3'.
  vector.erase(2);
  EXPECT_EQ(0, destructor_count[0]);
  EXPECT_EQ(0, destructor_count[1]);
  EXPECT_EQ(0, destructor_count[2]);
  EXPECT_EQ(2, destructor_count[3]);

  // last item before erase is now '2'.
  vector.erase(0);
  EXPECT_EQ(0, destructor_count[0]);
  EXPECT_EQ(0, destructor_count[1]);
  EXPECT_EQ(1, destructor_count[2]);
  EXPECT_EQ(2, destructor_count[3]);
}

TEST(DynamicVectorDeathTest, SwapWithInvalidIndex) {
  DynamicVector<int> vector;
  vector.push_back(0x1337);
  vector.push_back(0xcafe);
  EXPECT_DEATH(vector.swap(0, 2), "");
}

TEST(DynamicVectorDeathTest, SwapWithInvalidIndices) {
  DynamicVector<int> vector;
  vector.push_back(0x1337);
  vector.push_back(0xcafe);
  EXPECT_DEATH(vector.swap(2, 3), "");
}

TEST(DynamicVector, Swap) {
  DynamicVector<int> vector;
  vector.push_back(0x1337);
  vector.push_back(0xcafe);

  vector.swap(0, 1);
  EXPECT_EQ(vector[0], 0xcafe);
  EXPECT_EQ(vector[1], 0x1337);
}

TEST(DynamicVector, Back) {
  DynamicVector<int> vector;
  vector.push_back(0x1337);
  EXPECT_EQ(vector.back(), 0x1337);
  vector.push_back(0xcafe);
  EXPECT_EQ(vector.back(), 0xcafe);
}
