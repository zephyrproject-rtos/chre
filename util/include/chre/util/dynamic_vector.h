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
   * Returns a pointer to the underlying buffer. Note that this should not be
   * considered to be persistent as the vector will be moved and resized
   * automatically.
   *
   * @return the pointer to the underlying buffer.
   */
  ElementType *data() const;

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

 private:
  //! A pointer to the underlying data buffer.
  ElementType *mData = nullptr;

  //! The current size of the vector, as in the number of elements stored.
  size_t mSize = 0;

  //! The current capacity of the vector, as in the maximum number of elements
  //  that can be stored.
  size_t mCapacity = 0;
};

}  // namespace chre

#include "chre/util/dynamic_vector_impl.h"

#endif  // CHRE_UTIL_DYNAMIC_VECTOR_H_
