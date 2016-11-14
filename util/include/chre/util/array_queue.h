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

/*
 * TODO: need to implement thread-safe fifo queue that doesn't use dynamic
 * memory.
 * TODO: create a thread-safe wrapper for this, e.g. LockedArrayQueue. Starting
 * with single-threaded implementation for now
 */

template<typename ElementType, size_t kCapacity>
class ArrayQueue {
 public:
  /**
   * Calls the destructor of all the elements in the array queue.
   */
  ~ArrayQueue();

  /**
   * Determines whether the array queue is empty or not.
   *
   * @return true if the array queue is empty.
   */
  bool empty() const;

  /**
   * Obtains the number of elements currently stored in the array queue.
   *
   * @return The number of elements currently stored in the array queue.
   */
  size_t size() const;

  /**
   * Obtains the front element of the array queue. It is illegal to access the
   * front element when the array queue is empty. The user of the API must check
   * the size() or empty() function prior to accessing the front element to
   * ensure that they will not read out of bounds.
   *
   * @return The front element.
   */
  ElementType& front();

  /**
   * Obtains the front element of the array queue. It is illegal to access the
   * front element when the array queue is empty. The user of the API must check
   * the size() or empty() function prior to accessing the front element to
   * ensure that they will not read out of bounds.
   *
   * @return The front element.
   */
  const ElementType& front() const;

  /**
   * Obtains an element of the array queue given an index. It is illegal to
   * index this array queue out of bounds and the user of the API must check the
   * size() function prior to indexing this array queue to ensure that they will
   * not read out of bounds.
   *
   * @param index Requested index in range [0,size()-1]
   * @return The element.
   */
  ElementType& operator[](size_t index);

  /**
   * Obtains an element of the array queue given an index. It is illegal to
   * index this array queue out of bounds and the user of the API must check the
   * size() function prior to indexing this array queue to ensure that they will
   * not read out of bounds.
   *
   * @param index Requested index in range [0,size()-1]
   * @return The element.
   */
  const ElementType& operator[](size_t index) const;

  /**
   * Pushes an element onto the back of the array queue. It returns false if
   * the array queue is full already and there is no room for the element.
   *
   * @param element The element to push onto the array queue.
   * @return true if the element is pushed successfully.
   */
  bool push(const ElementType& element);

  /**
   * Removes the front element from the array queue if the array queue is not
   * empty.
   */
  void pop();

  /**
   * Removes an element from the array queue given an index. It returns false if the
   * array queue contains fewer items than the index.
   *
   * @param index Requested index in range [0,size()-1]
   * @return true if the indexed element has been removed successfully.
   */
  bool remove(size_t index);

  /**
   * Constructs an element onto the back of the array queue.
   *
   * @param The arguments to the constructor
   * @return true if the element is constructed successfully.
   */
  template<typename... Args>
  bool emplace(Args&&... args);

 private:
  /**
   * Storage for array queue elements. To avoid static initialization of
   * members, std::aligned_storage is used.
   */
  typename std::aligned_storage<sizeof(ElementType),
                                alignof(ElementType)>::type mData[kCapacity];

  /*
   * Initialize mTail to be (kCapacity-1). When an element is pushed in,
   * mHead and mTail will align. Also, this is consistent with
   * mSize = (mTail - mHead)%kCapacity + 1 for mSize > 0.
   */
  //! Index of the front element
  size_t mHead = 0;

  //! Index of the back element
  size_t mTail = kCapacity - 1;

  //! Number of elements in the array queue
  size_t mSize = 0;

  /**
   * Obtains a pointer to the underlying storage for the vector.
   *
   * @return A pointer to the storage used for elements in this vector.
   */
  ElementType *data();

  /**
   * Obtains a pointer to the underlying storage for the vector.
   *
   * @return A pointer to the storage used for elements in this vector.
   */
  const ElementType *data() const;

  /**
   * Converts relative index with respect to mHead to absolute index in the
   * storage array.
   *
   * @param index Relative index in range [0,size()-1]
   * @return The index of the storage array in range [0,kCapacity-1]
   */
  size_t relativeIndexToAbsolute(size_t index) const;

  /*
   * Pulls mHead to the next element in the array queue and decrements mSize
   * accordingly. It is illegal to call this function on an empty array queue.
   */
  void pullHead();

  /*
   * Pushes mTail to the next available storage space and increments mSize
   * accordingly.
   *
   * @return true if the array queue is not full.
   */
  bool pushTail();
};

}  // namespace chre

#include "chre/util/array_queue_impl.h"

#endif  // CHRE_UTIL_ARRAY_QUEUE_H_
