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

#ifndef CHRE_UTIL_BLOCKING_QUEUE_H
#define CHRE_UTIL_BLOCKING_QUEUE_H

// TODO: using STL+stdlib mutex/condition_variable here, need to replace with
// our platform abstraction and our own implementation that does not result in
// copying, etc.
// this is shamelessly pulled from stackoverflow to get the ball rolling

#include <condition_variable>
#include <deque>
#include <mutex>

template <typename T>
class BlockingQueue
{
 public:
  void push(T const& value) {
    {
      std::unique_lock<std::mutex> lock(mMutex);
      mQueue.push_back(value);
    }
    mCondition.notify_one();
  }

  T pop() {
    std::unique_lock<std::mutex> lock(mMutex);
    mCondition.wait(lock, [=]{ return !mQueue.empty(); });
    T rc(std::move(mQueue.back()));
    mQueue.pop_front();
    return rc;
  }

  bool empty() {
    return mQueue.empty();
  }

 private:
  std::mutex mMutex;
  std::condition_variable mCondition;
  std::deque<T> mQueue;
};

#endif  // CHRE_BLOCKING_QUEUE_H
