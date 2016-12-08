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

#ifndef CHRE_UTIL_DYNAMIC_VECTOR_H_
#define CHRE_UTIL_DYNAMIC_VECTOR_H_

#include <cstddef>

namespace chre {

/**
 * A container for storing a sequential array of elements. This container
 * resizes dynamically using heap allocations.
 */
template<typename ElementType>
class DynamicVector {
 public:
  /**
   * Destructs the objects and releases the memory owned by the vector.
   */
  ~DynamicVector();

  /**
   * Returns a pointer to the underlying buffer. Note that this should not be
   * considered to be persistent as the vector will be moved and resized
   * automatically.
   *
   * @return the pointer to the underlying buffer.
   */
  ElementType *data();

  /**
   * Returns a const pointer to the underlying buffer. Note that this should not
   * be considered to be persistent as the vector will be moved and resized
   * automatically.
   *
   * @return the const pointer to the underlying buffer.
   */
  const ElementType *data() const;

  /**
   * Returns the current number of elements in the vector.
   *
   * @return the number of elements in the vector.
   */
  size_t size() const;

  /**
   * Returns the maximum number of elements that can be stored in this vector
   * without a resize operation.
   *
   * @return the capacity of the vector.
   */
  size_t capacity() const;

  /**
   * Determines whether the vector is empty or not.
   *
   * @return Returns true if the vector is empty.
   */
  bool empty() const;

  /**
   * Pushes an element onto the back of the vector. If the vector requires a
   * resize and that allocation fails this function will return false.
   *
   * @param The element to push onto the vector.
   * @return Returns true if the element was pushed successfully.
   */
  bool push_back(const ElementType& element);

  /**
   * Constructs an element onto the back of the vector. It is illegal to
   * construct an item onto a full vector. The user of the API must check the
   * return of the full() function prior to constructing another element.
   *
   * @param The arguments to the constructor
   */
  template<typename... Args>
  bool emplace_back(Args&&... args);

  /**
   * Obtains an element of the vector given an index. It is illegal to index
   * this vector out of bounds and the user of the API must check the size()
   * function prior to indexing this vector to ensure that they will not read
   * out of bounds.
   *
   * @param The index of the element.
   * @return The element.
   */
  ElementType& operator[](size_t index);

  /**
   * Obtains a const element of the vector given an index. It is illegal to
   * index this vector out of bounds and the user of the API must check the
   * size() function prior to indexing this vector to ensure that they will not
   * read out of bounds.
   *
   * @param The index of the element.
   * @return The element.
   */
  const ElementType& operator[](size_t index) const;

  /**
   * Resizes the vector to a new capacity returning true if allocation was
   * successful. If the new capacity is smaller than the current capacity,
   * the operation is a no-op and true is returned. If a memory allocation
   * fails, the contents of the vector are not modified and false is returned.
   * This is intended to be similar to the reserve function of the std::vector.
   *
   * @param The new capacity of the vector.
   * @return True if the resize operation was successful.
   */
  bool reserve(size_t newCapacity);

 private:
  //! A pointer to the underlying data buffer.
  ElementType *mData = nullptr;

  //! The current size of the vector, as in the number of elements stored.
  size_t mSize = 0;

  //! The current capacity of the vector, as in the maximum number of elements
  //! that can be stored.
  size_t mCapacity = 0;

  /**
   * Prepares a vector to push one element onto the back. The vector may be
   * resized if required.
   *
   * @return Whether or not the resize was successful.
   */
  bool prepareForPush();
};

}  // namespace chre

#include "chre/util/dynamic_vector_impl.h"

#endif  // CHRE_UTIL_DYNAMIC_VECTOR_H_
