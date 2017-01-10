#include "gtest/gtest.h"

#include "chre/util/unique_ptr.h"

using chre::UniquePtr;

struct Value {
  Value(int value) : value(value) {
    constructionCounter++;
  }

  ~Value() {
    constructionCounter--;
  }

  Value& operator=(Value&& other) {
    value = other.value;
    return *this;
  }

  int value;
  static int constructionCounter;
};

int Value::constructionCounter = 0;

TEST(UniquePtr, Construct) {
  UniquePtr<Value> myInt(0xcafe);
  ASSERT_FALSE(myInt.isNull());
  EXPECT_EQ(myInt.get()->value, 0xcafe);
  EXPECT_EQ(myInt->value, 0xcafe);
  EXPECT_EQ((*myInt).value, 0xcafe);
  EXPECT_EQ(myInt[0].value, 0xcafe);
}

TEST(UniquePtr, Move) {
  EXPECT_EQ(Value::constructionCounter, 0);

  {
    UniquePtr<Value> myInt(0xcafe);
    ASSERT_FALSE(myInt.isNull());
    EXPECT_EQ(Value::constructionCounter, 1);

    UniquePtr<Value> myMovedInt(0);
    ASSERT_FALSE(myMovedInt.isNull());
    EXPECT_EQ(Value::constructionCounter, 2);
    myMovedInt = std::move(myInt);
    ASSERT_FALSE(myMovedInt.isNull());
    ASSERT_TRUE(myInt.isNull());
    EXPECT_EQ(myMovedInt.get()->value, 0xcafe);
  }

  EXPECT_EQ(Value::constructionCounter, 0);
}
