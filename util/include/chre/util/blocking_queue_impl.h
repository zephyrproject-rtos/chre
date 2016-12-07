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

#ifndef CHRE_UTIL_BLOCKING_QUEUE_IMPL_H_
#define CHRE_UTIL_BLOCKING_QUEUE_IMPL_H_

#include "chre/util/blocking_queue.h"

namespace chre {

template<typename ElementType>
void BlockingQueue<ElementType>::push(const ElementType& element) {
  {
    std::lock_guard<Mutex> lock(mMutex);
    mQueue.push_back(element);
  }
  mConditionVariable.notify_one();
}

template<typename ElementType>
ElementType BlockingQueue<ElementType>::pop() {
  std::lock_guard<Mutex> lock(mMutex);
  while (mQueue.empty()) {
    mConditionVariable.wait(mMutex);
  }

  ElementType element(std::move(mQueue.front()));
  mQueue.pop_front();
  return element;
}

template<typename ElementType>
bool BlockingQueue<ElementType>::empty() {
  std::lock_guard<Mutex> lock(mMutex);
  return mQueue.empty();
}

}  // namespace chre

#endif  // CHRE_UTIL_BLOCKING_QUEUE_IMPL_H_
