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

#ifndef CHRE_UTIL_PRIORITY_QUEUE_IMPL_H_
#define CHRE_UTIL_PRIORITY_QUEUE_IMPL_H_

#include <utility>

#include "chre/platform/assert.h"
#include "chre/util/dynamic_vector.h"

namespace chre {

template<typename ElementType, typename CompareType>
PriorityQueue<ElementType, CompareType>::PriorityQueue() {}

template<typename ElementType, typename CompareType>
PriorityQueue<ElementType, CompareType>::PriorityQueue(const CompareType& compare)
    : mCompare(compare) {}

template<typename ElementType, typename CompareType>
size_t PriorityQueue<ElementType, CompareType>::size() const {
  return mData.size();
}

template<typename ElementType, typename CompareType>
size_t PriorityQueue<ElementType, CompareType>::capacity() const {
  return mData.capacity();
}

template<typename ElementType, typename CompareType>
bool PriorityQueue<ElementType, CompareType>::empty() const {
  return mData.empty();
}

template<typename ElementType, typename CompareType>
bool PriorityQueue<ElementType, CompareType>::push(const ElementType& element) {
  bool success = mData.push_back(element);
  if (success) {
    // TODO: replace by push_heap()
    arrangeQueue();
  }
  return success;
}

template<typename ElementType, typename CompareType>
template<typename... Args>
bool PriorityQueue<ElementType, CompareType>::emplace(Args&&... args) {
  bool success = mData.emplace_back(args...);
  if (success) {
    // TODO: replace by push_heap()
    arrangeQueue();
  }
  return success;
}

template<typename ElementType, typename CompareType>
ElementType& PriorityQueue<ElementType, CompareType>::operator[](size_t index) {
  return mData[index];
}

template<typename ElementType, typename CompareType>
const ElementType& PriorityQueue<ElementType, CompareType>::operator[](size_t index) const {
  return mData[index];
}

template<typename ElementType, typename CompareType>
ElementType& PriorityQueue<ElementType, CompareType>::top() {
  return mData[0];
}

template<typename ElementType, typename CompareType>
const ElementType& PriorityQueue<ElementType, CompareType>::top() const {
  return mData[0];
}

template<typename ElementType, typename CompareType>
void PriorityQueue<ElementType, CompareType>::pop() {
  if (mData.size() > 0) {
    // TODO: replace by pop_heap() & mData.erase(mData.size() - 1)
    remove(0);
  }
}

template<typename ElementType, typename CompareType>
void PriorityQueue<ElementType, CompareType>::remove(size_t index) {
  // TODO: replace by erase_heap(index) & mData.erase(mData.size() - 1)
  mData.erase(index);

  // TODO: consider resizing the dynamic array to mData.capacity()/2
  // when mData.size() <= mData.capacity()/4.
}

template<typename ElementType, typename CompareType>
void PriorityQueue<ElementType, CompareType>::arrangeQueue() {
  // Search for the index where the newly added element should be,
  // assuming that element is at the end of the dynamic vector.
  for (size_t i = 0; i < mData.size() - 1; ++i) {
    if (mCompare(mData[i], mData[mData.size() - 1])) {
      // This is not the most efficient way even for a linear array,
      // but just something to make it work till heap is ready.
      ElementType e = mData[mData.size() - 1];
      mData.erase(mData.size() - 1);
      mData.insert(i, e);
      break;
    }
  }
}
}  // namespace chre

#endif  // CHRE_UTIL_PRIORITY_QUEUE_IMPL_H_
