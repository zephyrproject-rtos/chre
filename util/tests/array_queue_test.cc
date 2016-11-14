#include "gtest/gtest.h"
#include "chre/util/array_queue.h"

using chre::ArrayQueue;

// TODO: do we need this? go/deathtest
//typedef ArrayQueueTest ArrayQueueDeathTest;

namespace {
constexpr int kMaxTestCapacity = 10;
int destructor_count[kMaxTestCapacity];
int constructor_count;

class DummyElement {
 public:
  DummyElement() {
    constructor_count++;
  };
  DummyElement(int i) {
    val_ = i;
    constructor_count++;
  };
  ~DummyElement() {
    if (val_ >= 0 && val_ < kMaxTestCapacity) {
      destructor_count[val_]++;
    }
  };
  void setValue(int i) {
    val_ = i;
  }

 private:
  int val_ = kMaxTestCapacity - 1;
};
}

TEST(ArrayQueueTest, IsEmptyInitially) {
  ArrayQueue<int, 4> q;
  EXPECT_TRUE(q.empty());
  EXPECT_EQ(0, q.size());
}

TEST(ArrayQueueTest, SimplePushPop) {
  ArrayQueue<int, 3> q;
  EXPECT_TRUE(q.push(1));
  EXPECT_TRUE(q.push(2));
  q.pop();
  EXPECT_TRUE(q.push(3));
}

TEST(ArrayQueueTest, TestSize) {
  ArrayQueue<int, 2> q;
  q.push(1);
  EXPECT_EQ(1, q.size());
  q.push(2);
  EXPECT_EQ(2, q.size());
  q.pop();
  EXPECT_EQ(1, q.size());
  q.pop();
}

TEST(ArrayQueueTest, TestEmpty) {
  ArrayQueue<int, 2> q;
  q.push(1);
  EXPECT_FALSE(q.empty());
  q.push(2);
  EXPECT_FALSE(q.empty());
  q.pop();
  EXPECT_FALSE(q.empty());
  q.pop();
  EXPECT_TRUE(q.empty());
}

TEST(ArrayQueueTest, PopWhenEmpty) {
  ArrayQueue<int, 4> q;
  q.pop();
  EXPECT_EQ(0, q.size());
}

TEST(ArrayQueueTest, PushWhenFull) {
  ArrayQueue<int, 2> q;
  q.push(1);
  q.push(2);
  EXPECT_FALSE(q.push(3));
}

TEST(ArrayQueueTest, FrontWhenEmpty) {
  ArrayQueue<int, 4> q;
  EXPECT_DEATH(q.front(), "");
}

TEST(ArrayQueueTest, TestFront) {
  ArrayQueue<int, 3> q;
  q.push(1);
  EXPECT_EQ(1, q.front());
  q.pop();
  q.push(2);
  EXPECT_EQ(2, q.front());
}

TEST(ArrayQueueTest, InvalidSubscript) {
  ArrayQueue<int, 2> q;
  EXPECT_DEATH(q[0], "");
 }

TEST(ArrayQueueTest, Subscript) {
  ArrayQueue<int, 2> q;
  q.push(1);
  q.push(2);
  EXPECT_EQ(1, q[0]);
  EXPECT_EQ(2, q[1]);
  q.pop();
  EXPECT_EQ(2, q[0]);
}

TEST(ArrayQueueTest, RemoveWithInvalidIndex) {
  ArrayQueue<int, 3> q;
  EXPECT_FALSE(q.remove(0));
}

TEST(ArrayQueueTest, RemoveWithIndex) {
  ArrayQueue<int, 3> q;
  q.push(1);
  q.push(2);
  q.remove(0);
  EXPECT_EQ(2, q.front());
  EXPECT_EQ(1, q.size());
  q.push(3);
  q.remove(1);
  EXPECT_EQ(2, q.front());
  EXPECT_EQ(1, q.size());
}

TEST(ArrayQueueTest, DestructorCalledOnPop) {
  for (size_t i = 0; i < kMaxTestCapacity; ++i) {
    destructor_count[i] = 0;
  }

  ArrayQueue<DummyElement, 3> q;
  DummyElement e;
  q.push(e);
  q.push(e);

  q.front().setValue(0);
  q.pop();
  EXPECT_EQ(1, destructor_count[0]);

  q.front().setValue(1);
  q.pop();
  EXPECT_EQ(1, destructor_count[1]);
}

TEST(ArrayQueueTest, ElementsDestructedWhenQueueDestructed) {
  for (size_t i = 0; i < kMaxTestCapacity; ++i) {
    destructor_count[i] = 0;
  }

  // Put q and e in the scope so their destructor will be called going
  // out of scope.
  { ArrayQueue<DummyElement, 4> q;
    DummyElement e;

    for (size_t i = 0; i < 3; ++i) {
      q.push(e);
      q[i].setValue(i);
    }

    q.~ArrayQueue();

    for (size_t i = 0; i < 3; ++i) {
      EXPECT_EQ(1, destructor_count[i]);
    }
  }

  // Check destructor count.
  for (size_t i = 0; i < 3; ++i) {
    EXPECT_EQ(1, destructor_count[i]);
  }
  EXPECT_EQ(0, destructor_count[3]);
  EXPECT_EQ(1, destructor_count[kMaxTestCapacity - 1]);
}

TEST(ArrayQueueTest, EmplaceTest) {
  constructor_count = 0;
  ArrayQueue<DummyElement, 2> q;

  EXPECT_TRUE(q.emplace(0));
  EXPECT_EQ(1, constructor_count);
  EXPECT_EQ(1, q.size());

  EXPECT_TRUE(q.emplace(1));
  EXPECT_EQ(2, constructor_count);
  EXPECT_EQ(2, q.size());

  EXPECT_FALSE(q.emplace(2));
  EXPECT_EQ(2, constructor_count);
  EXPECT_EQ(2, q.size());
}

// TODO: StressTest
