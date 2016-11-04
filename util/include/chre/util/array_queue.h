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

#ifndef CHRE_UTIL_ARRAY_QUEUE_H_
#define CHRE_UTIL_ARRAY_QUEUE_H_

#include <type_traits>

namespace chre {

// TODO: this code is not done, just left for now as a reference

/*
 * TODO: need to implement thread-safe fifo queue that doesn't use dynamic
 * memory... we can have something like this:
 *  - array of data types
 *  - head/tail index for data
 *  - head for free list (only needs to be singly-linked)
 *    - note that free list will use same array as linkage for active elements
 *
 * Don't worry about this for now, just use STL.
 *
 * This file is a WIP and probably doesn't work...
 */


// TODO: create a thread-safe wrapper for this, e.g. LockedArrayQueue. Starting
// with single-threaded implementation for now

template<typename ElementType, uint16_t kSize>
class ArrayQueue {
 public:
  ArrayQueue() {
    for (size_t i = 0; i < kSize - 1; i++) {
      mNext[i] = i + 1;
    }
    mNext[kSize - 1] = kInvalidItem;
    mHead = kInvalidItem;
    mTail = kInvalidItem;
    mFreeHead = 0;
  }

  // TODO: assert that T is trivially move/copy constructable?

  static_assert(kSize < UINT16_MAX,
                "ArrayList only supports up to UINT16_MAX-1 elements");
  typedef std::conditional<(kSize >= UINT8_MAX), uint16_t, uint8_t>::type
      IndexType;
  constexpr IndexType kInvalidItem = (sizeof(IndexType) == sizeof(uint8_t)) ?
                                     UINT8_MAX : UINT16_MAX;

  bool pushBack(const ElementType &data) {
    if (mFreeHead != kInvalidItem) {
      mData[mFreeHead] = data;
      mNext[mTail] = mFreeHead;
      mTail = mFreeHead;
      if (mHead == kInvalidItem) {
        mHead = mTail;
      }
      mFreeHead = mNext[mFreeHead];
      return true;
    } else {
      // todo: no space...
      LOGE("Tried to push to full queue");
      return false;
    }
  }

  // todo: this queue needs to be single consumer, so we know that front()
  // will not get freed while we're using it. maybe we add extra debug on here
  // too like ASSERT(getThreadId() == mLastPopThreadId)...

  bool empty() const {
    return (mHead == kInvalidItem);
  }

  ElementType &front() {
    ASSERT(mHead < kSize);
    return mData[mHead];
  }

  const ElementType &front() const {
    ASSERT(mHead < kSize);
    return mData[mHead];
  }

  // general usage semantics
  // while (!q.empty()) { T x = q.front(); x.doSomething(); q.popFront(); }

  // Unlinks the front item from the list, and marks its storage as available
  // for pushing new items to the queue.
  void popFront() {
    if (mHead != kInvalidItem) {
      mData[mHead].~ElementType();
      LinkType newHead = mNext[mHead];
      mNext[mHead] = mFreeHead;
      mFreeHead = mHead;
      mHead = newHead;
      if (mHead == kInvalidItem) {
        mTail = kInvalidItem;
      }
    } else {
      LOGE("Tried to pop empty list");
    }
  }

 private:
  ElementType mData[kSize];

  // Contains the index for the "next" element in the queue, or kInvalidItem
  // if there is no next item. Note that this array contains two
  // non-overlapping lists, one that identifies which indices in mData are
  // free (starting from mFreeHead), and another for the active part of the
  // list (mHead and mTail).
  IndexType mNext[kSize];
  IndexType mHead;
  IndexType mTail;
  IndexType mFreeHead;

  // TODO: lock
};

}  // namespace chre

#endif  // CHRE_UTIL_ARRAY_QUEUE_H_
