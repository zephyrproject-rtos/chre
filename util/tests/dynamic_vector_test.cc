#include "gtest/gtest.h"

#include "chre/util/dynamic_vector.h"

using chre::DynamicVector;

TEST(Vector, EmptyByDefault) {
  DynamicVector<int> testVector;
  ASSERT_EQ(testVector.data(), nullptr);
  ASSERT_EQ(testVector.size(), 0);
  ASSERT_EQ(testVector.capacity(), 0);
}
