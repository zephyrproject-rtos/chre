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

#include "chre/util/array.h"
#include "chre/util/dynamic_vector.h"

using chre::DynamicVector;

namespace {
constexpr int kMaxTestCapacity = 10;
int gDestructorCount[kMaxTestCapacity];

class Dummy {
 public:
  ~Dummy() {
    if (mValue >= 0) {
      gDestructorCount[mValue]++;
    }
  };
  void setValue(int value) {
    mValue = value;
  }
  int getValue() {
    return mValue;
  }

 private:
  int mValue = -1;
};

void resetDestructorCounts() {
  for (size_t i = 0; i < ARRAY_SIZE(gDestructorCount); i++) {
    gDestructorCount[i] = 0;
  }
}
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
  EXPECT_CHRE_ASSERT(EXPECT_FALSE(vector.insert(1, 0x1337)));
  ASSERT_TRUE(vector.insert(0, 0x1337));
  EXPECT_EQ(vector[0], 0x1337);
  EXPECT_EQ(vector.data()[0], 0x1337);
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
  resetDestructorCounts();

  DynamicVector<Dummy> vector;
  for (size_t i = 0; i < 4; ++i) {
    vector.push_back(Dummy());
    vector[i].setValue(i);
  }

  // last item before erase is '3'.
  vector.erase(1);
  EXPECT_EQ(0, gDestructorCount[0]);
  EXPECT_EQ(0, gDestructorCount[1]);
  EXPECT_EQ(0, gDestructorCount[2]);
  EXPECT_EQ(1, gDestructorCount[3]);

  // last item before erase is still '3'.
  vector.erase(2);
  EXPECT_EQ(0, gDestructorCount[0]);
  EXPECT_EQ(0, gDestructorCount[1]);
  EXPECT_EQ(0, gDestructorCount[2]);
  EXPECT_EQ(2, gDestructorCount[3]);

  // last item before erase is now '2'.
  vector.erase(0);
  EXPECT_EQ(0, gDestructorCount[0]);
  EXPECT_EQ(0, gDestructorCount[1]);
  EXPECT_EQ(1, gDestructorCount[2]);
  EXPECT_EQ(2, gDestructorCount[3]);
}

TEST(DynamicVector, Clear) {
  resetDestructorCounts();

  DynamicVector<Dummy> vector;
  for (size_t i = 0; i < 4; ++i) {
    vector.push_back(Dummy());
    vector[i].setValue(i);
  }

  vector.clear();
  EXPECT_EQ(vector.size(), 0);
  EXPECT_EQ(vector.capacity(), 4);

  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(gDestructorCount[i], 1);
  }
}

// Make sure that a vector wrapping an array doesn't call the destructor when
// the vector is destructed
TEST(DynamicVector, WrapDoesntCallDestructor) {
  resetDestructorCounts();

  Dummy array[4];
  for (size_t i = 0; i < 4; ++i) {
    array[i].setValue(i);
  }

  {
    DynamicVector<Dummy> vector;
    vector.wrap(array, ARRAY_SIZE(array));
  }

  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(gDestructorCount[i], 0);
  }
}

// Make sure that a wrapped vector does call the destructor when it's expected
// as part of an API call
TEST(DynamicVector, WrapExplicitlyCallsDestructor) {
  resetDestructorCounts();

  Dummy array[4];
  constexpr size_t kSize = ARRAY_SIZE(array);
  static_assert(ARRAY_SIZE(array) <= ARRAY_SIZE(gDestructorCount),
                "gDestructorCount array must fit test array");
  for (size_t i = 0; i < kSize; ++i) {
    array[i].setValue(i);
  }
  DynamicVector<Dummy> vector;
  vector.wrap(array, ARRAY_SIZE(array));

  vector.erase(kSize - 1);
  for (size_t i = 0; i < kSize - 1; i++) {
    EXPECT_EQ(gDestructorCount[i], 0);
  }
  EXPECT_EQ(gDestructorCount[kSize - 1], 1);

  vector.clear();
  for (size_t i = 0; i < kSize; ++i) {
    EXPECT_EQ(gDestructorCount[i], 1);
  }
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

TEST(DynamicVector, BackFront) {
  DynamicVector<int> vector;
  vector.push_back(0x1337);
  EXPECT_EQ(vector.front(), 0x1337);
  EXPECT_EQ(vector.back(), 0x1337);
  vector.push_back(0xcafe);
  EXPECT_EQ(vector.front(), 0x1337);
  EXPECT_EQ(vector.back(), 0xcafe);
  vector.erase(0);
  EXPECT_EQ(vector.front(), 0xcafe);
  EXPECT_EQ(vector.back(), 0xcafe);
}

TEST(DynamicVector, Iterator) {
  DynamicVector<int> vector;
  vector.push_back(0);
  vector.push_back(1);
  vector.push_back(2);

  size_t index = 0;
  for (DynamicVector<int>::iterator it = vector.begin();
       it != vector.end(); ++it) {
    EXPECT_EQ(vector[index++], *it);
  }

  DynamicVector<int>::iterator it = vector.begin() + vector.size() - 1;
  EXPECT_EQ(vector[vector.size() - 1], *it);

  it = vector.begin() + vector.size();
  EXPECT_TRUE(it == vector.end());
}

TEST(DynamicVector, ConstIterator) {
  DynamicVector<int> vector;
  vector.push_back(0);
  vector.push_back(1);
  vector.push_back(2);

  size_t index = 0;
  for (DynamicVector<int>::const_iterator cit = vector.cbegin();
       cit != vector.cend(); ++cit) {
    EXPECT_EQ(vector[index++], *cit);
  }

  DynamicVector<int>::const_iterator cit = vector.cbegin() + vector.size() - 1;
  EXPECT_EQ(vector[vector.size() - 1], *cit);

  cit = vector.cbegin() + vector.size();
  EXPECT_TRUE(cit == vector.cend());
}

TEST(DynamicVector, IteratorAndPushBack) {
  DynamicVector<int> vector;
  vector.push_back(0);
  vector.push_back(1);
  vector.push_back(2);
  size_t oldCapacity = vector.capacity();

  DynamicVector<int>::iterator it_b = vector.begin();
  DynamicVector<int>::iterator it_e = vector.end();

  vector.push_back(3);
  ASSERT_TRUE(oldCapacity == vector.capacity());

  size_t index = 0;
  for (; it_b != it_e; ++it_b) {
    EXPECT_EQ(vector[index++], *it_b);
  }
}

TEST(DynamicVector, IteratorAndEmplaceBack) {
  DynamicVector<int> vector;
  vector.push_back(0);
  vector.push_back(1);
  vector.push_back(2);
  size_t oldCapacity = vector.capacity();

  DynamicVector<int>::iterator it_b = vector.begin();
  DynamicVector<int>::iterator it_e = vector.end();

  vector.emplace_back(3);
  ASSERT_TRUE(oldCapacity == vector.capacity());

  size_t index = 0;
  for (; it_b != it_e; ++it_b) {
    EXPECT_EQ(vector[index++], *it_b);
  }
}

TEST(DynamicVector, IteratorAndReserve) {
  DynamicVector<int> vector;
  vector.push_back(0);
  vector.push_back(1);
  vector.push_back(2);
  size_t oldCapacity = vector.capacity();

  DynamicVector<int>::iterator it_b = vector.begin();
  DynamicVector<int>::iterator it_e = vector.end();

  vector.reserve(oldCapacity);
  ASSERT_TRUE(oldCapacity == vector.capacity());

  size_t index = 0;
  for (; it_b != it_e; ++it_b) {
    EXPECT_EQ(vector[index++], *it_b);
  }
}

TEST(DynamicVector, IteratorAndInsert) {
  DynamicVector<int> vector;
  vector.push_back(0);
  vector.push_back(1);
  vector.push_back(2);
  size_t oldCapacity = vector.capacity();

  DynamicVector<int>::iterator it_b = vector.begin();

  vector.insert(2, 3);
  ASSERT_TRUE(oldCapacity == vector.capacity());

  size_t index = 0;
  while (index < 2) {
    EXPECT_EQ(vector[index++], *it_b++);
  }
}

TEST(DynamicVector, IteratorAndErase) {
  DynamicVector<int> vector;
  vector.push_back(0);
  vector.push_back(1);
  vector.push_back(2);

  DynamicVector<int>::iterator it_b = vector.begin();

  vector.erase(2);

  size_t index = 0;
  while (index < 2) {
    EXPECT_EQ(vector[index++], *it_b++);
  }
}

TEST(DynamicVector, IteratorAndSwap) {
  DynamicVector<int> vector;
  vector.push_back(0);
  vector.push_back(1);
  vector.push_back(2);
  vector.push_back(3);

  DynamicVector<int>::iterator it_b = vector.begin();

  vector.swap(1, 3);

  size_t index = 0;
  while (index < 4) {
    if (index != 1 && index != 3) {
      EXPECT_EQ(vector[index], *it_b);
    }
    index++;
    it_b++;
  }
}

TEST(DynamicVector, MoveConstruct) {
  DynamicVector<int> vector;
  ASSERT_TRUE(vector.push_back(0));
  ASSERT_TRUE(vector.push_back(1));
  ASSERT_TRUE(vector.push_back(2));

  DynamicVector<int> movedVector(std::move(vector));
  EXPECT_EQ(vector.data(), nullptr);
  EXPECT_NE(movedVector.data(), nullptr);
  EXPECT_EQ(vector.size(), 0);
  EXPECT_EQ(movedVector.size(), 3);
  EXPECT_EQ(vector.capacity(), 0);
  EXPECT_EQ(movedVector.capacity(), 4);
}

// Tests basic functionality of a vector wrapping an array
TEST(DynamicVector, Wrap) {
  constexpr size_t kSize = 4;
  int buf[kSize];
  for (size_t i = 0; i < kSize; i++) {
    buf[i] = i;
  }

  DynamicVector<int> vector;
  EXPECT_TRUE(vector.owns_data());
  vector.wrap(buf, kSize);
  EXPECT_FALSE(vector.owns_data());
  EXPECT_EQ(vector.size(), kSize);
  EXPECT_EQ(vector.capacity(), kSize);
  EXPECT_EQ(vector.data(), buf);

  EXPECT_CHRE_ASSERT(EXPECT_FALSE(vector.reserve(8)));
  EXPECT_CHRE_ASSERT(EXPECT_FALSE(vector.push_back(-1)));
  EXPECT_CHRE_ASSERT(EXPECT_FALSE(vector.emplace_back(-1)));
  EXPECT_CHRE_ASSERT(EXPECT_FALSE(vector.insert(1, -1)));
  EXPECT_CHRE_ASSERT(EXPECT_FALSE(vector.copy_array(buf, kSize)));

  for (size_t i = 0; i < kSize; i++) {
    EXPECT_EQ(vector[i], i);
  }

  vector.erase(0);
  for (size_t i = 0; i < kSize - 1; i++) {
    EXPECT_EQ(vector[i], i + 1);
  }

  EXPECT_TRUE(vector.push_back(kSize + 1));
  EXPECT_EQ(vector.back(), kSize + 1);
}

TEST(DynamicVector, MoveWrappedVector) {
  constexpr size_t kSize = 4;
  int buf[kSize];
  for (size_t i = 0; i < kSize; i++) {
    buf[i] = i;
  }

  DynamicVector<int> vector1;
  vector1.wrap(buf, kSize);

  DynamicVector<int> vector2 = std::move(vector1);
  EXPECT_TRUE(vector1.owns_data());
  EXPECT_EQ(vector1.size(), 0);
  EXPECT_EQ(vector1.capacity(), 0);
  EXPECT_EQ(vector1.data(), nullptr);

  EXPECT_FALSE(vector2.owns_data());
  EXPECT_EQ(vector2.size(), kSize);
  EXPECT_EQ(vector2.capacity(), kSize);
  EXPECT_EQ(vector2.data(), buf);
}

TEST(DynamicVector, Unwrap) {
  constexpr size_t kSize = 4;
  int buf[kSize];
  for (size_t i = 0; i < kSize; i++) {
    buf[i] = i;
  }

  DynamicVector<int> vec;
  vec.wrap(buf, kSize);
  ASSERT_FALSE(vec.owns_data());

  vec.unwrap();
  EXPECT_TRUE(vec.owns_data());
  EXPECT_EQ(vec.size(), 0);
  EXPECT_EQ(vec.capacity(), 0);
  EXPECT_EQ(vec.data(), nullptr);

  EXPECT_TRUE(vec.push_back(1));
}

TEST(DynamicVector, CopyArray) {
  constexpr size_t kSize = 4;
  int buf[kSize];
  for (size_t i = 0; i < kSize; i++) {
    buf[i] = i;
  }

  DynamicVector<int> vec;
  ASSERT_TRUE(vec.copy_array(buf, kSize));
  EXPECT_TRUE(vec.owns_data());

  EXPECT_EQ(vec.size(), kSize);
  EXPECT_EQ(vec.capacity(), kSize);
  EXPECT_NE(vec.data(), buf);

  EXPECT_TRUE(vec.push_back(kSize));
  EXPECT_EQ(vec.size(), kSize + 1);
  EXPECT_GE(vec.capacity(), kSize + 1);

  for (size_t i = 0; i < kSize + 1; i++) {
    EXPECT_EQ(vec[i], i);
  }
}

TEST(DynamicVector, CopyArrayHandlesDestructor) {
  resetDestructorCounts();
  constexpr size_t kSize = 4;

  {
    DynamicVector<Dummy> vec;
    {
      Dummy array[kSize];
      for (size_t i = 0; i < kSize; i++) {
        array[i].setValue(i);
      }

      ASSERT_TRUE(vec.copy_array(array, kSize));
    }

    for (size_t i = 0; i < kSize; i++) {
      EXPECT_EQ(gDestructorCount[i], 1);
    }

    for (size_t i = 0; i < kSize; i++) {
      ASSERT_TRUE(vec[i].getValue() == i);
    }
  }

  for (size_t i = 0; i < kSize; i++) {
    EXPECT_EQ(gDestructorCount[i], 2);
  }
}

TEST(DynamicVector, CopyEmptyArray) {
  DynamicVector<int> vec;

  EXPECT_TRUE(vec.copy_array(nullptr, 0));
  EXPECT_EQ(vec.size(), 0);

  vec.emplace_back(1);
  EXPECT_TRUE(vec.copy_array(nullptr, 0));
  EXPECT_EQ(vec.size(), 0);
}

TEST(DynamicVector, PrepareForPush) {
  DynamicVector<int> vector;
  EXPECT_EQ(vector.size(), 0);
  EXPECT_EQ(vector.capacity(), 0);

  // Perform an initial prepareForPush operation which causes a size of one.
  ASSERT_TRUE(vector.prepareForPush());
  EXPECT_EQ(vector.size(), 0);
  EXPECT_EQ(vector.capacity(), 1);
  ASSERT_TRUE(vector.push_back(0xcafe));
  EXPECT_EQ(vector.size(), 1);
  EXPECT_EQ(vector.capacity(), 1);

  // Verify that it becomes larger
  ASSERT_TRUE(vector.prepareForPush());
  EXPECT_EQ(vector[0], 0xcafe);
  EXPECT_EQ(vector.size(), 1);
  EXPECT_EQ(vector.capacity(), 2);

  // The vector should not become any larger than necessary.
  ASSERT_TRUE(vector.prepareForPush());
  EXPECT_EQ(vector[0], 0xcafe);
  EXPECT_EQ(vector.size(), 1);
  EXPECT_EQ(vector.capacity(), 2);
}
