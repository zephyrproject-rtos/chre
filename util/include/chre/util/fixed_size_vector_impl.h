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

#ifndef CHRE_UTIL_FIXED_SIZE_VECTOR_IMPL_H_
#define CHRE_UTIL_FIXED_SIZE_VECTOR_IMPL_H_

#include "chre/platform/assert.h"
#include "chre/util/fixed_size_vector.h"

namespace chre {

template<typename ElementType, size_t kCapacity>
ElementType *FixedSizeVector<ElementType, kCapacity>::data() {
  return reinterpret_cast<ElementType *>(mData);
}

template<typename ElementType, size_t kCapacity>
const ElementType *FixedSizeVector<ElementType, kCapacity>::data() const {
  return reinterpret_cast<const ElementType *>(mData);
}

template<typename ElementType, size_t kCapacity>
size_t FixedSizeVector<ElementType, kCapacity>::size() const {
  return mSize;
}

template<typename ElementType, size_t kCapacity>
size_t FixedSizeVector<ElementType, kCapacity>::capacity() const {
  return kCapacity;
}

template<typename ElementType, size_t kCapacity>
bool FixedSizeVector<ElementType, kCapacity>::empty() const {
  return (mSize == 0);
}

template<typename ElementType, size_t kCapacity>
bool FixedSizeVector<ElementType, kCapacity>::full() const {
  return (mSize == kCapacity);
}

template<typename ElementType, size_t kCapacity>
void FixedSizeVector<ElementType, kCapacity>::push_back(
    const ElementType& element) {
  CHRE_ASSERT(!full());
  if (!full()) {
    data()[mSize++] = element;
  }
}

template<typename ElementType, size_t kCapacity>
template<typename... Args>
void FixedSizeVector<ElementType, kCapacity>::emplace_back(Args&&... args) {
  CHRE_ASSERT(!full());
  if (!full()) {
    new (&data()[mSize++]) ElementType(std::forward<Args>(args)...);
  }
}

template<typename ElementType, size_t kCapacity>
ElementType& FixedSizeVector<ElementType, kCapacity>::operator[](
    size_t index) {
  CHRE_ASSERT(index < mSize);
  if (index >= mSize) {
    index = mSize - 1;
  }

  return data()[index];
}

template<typename ElementType, size_t kCapacity>
const ElementType& FixedSizeVector<ElementType, kCapacity>::operator[](
    size_t index) const {
  CHRE_ASSERT(index < mSize);
  if (index >= mSize) {
    index = mSize - 1;
  }

  return data()[index];
}

template<typename ElementType, size_t kCapacity>
void FixedSizeVector<ElementType, kCapacity>::erase(size_t index) {
  CHRE_ASSERT(index < mSize);
  if (index < mSize) {
    mSize--;
    for (size_t i = index; i < mSize; i++) {
      data()[i] = std::move(data()[i + 1]);
    }

    data()[mSize].~ElementType();
  }
}

template<typename ElementType, size_t kCapacity>
void FixedSizeVector<ElementType, kCapacity>::swap(size_t index0,
                                                   size_t index1) {
  CHRE_ASSERT(index0 < mSize && index1 < mSize);
  if (index0 < mSize && index1 < mSize) {
    ElementType temp = std::move(data()[index0]);
    data()[index0] = std::move(data()[index1]);
    data()[index1] = std::move(temp);
  }
}

template<typename ElementType, size_t kCapacity>
typename FixedSizeVector<ElementType, kCapacity>::iterator
    FixedSizeVector<ElementType, kCapacity>::begin() {
  return data();
}

template<typename ElementType, size_t kCapacity>
typename FixedSizeVector<ElementType, kCapacity>::iterator
    FixedSizeVector<ElementType, kCapacity>::end() {
  return (data() + mSize);
}

template<typename ElementType, size_t kCapacity>
typename FixedSizeVector<ElementType, kCapacity>::const_iterator
    FixedSizeVector<ElementType, kCapacity>::begin() const {
  return data();
}

template<typename ElementType, size_t kCapacity>
typename FixedSizeVector<ElementType, kCapacity>::const_iterator
    FixedSizeVector<ElementType, kCapacity>::end() const {
  return (data() + mSize);
}

template<typename ElementType, size_t kCapacity>
void FixedSizeVector<ElementType, kCapacity>::resize(size_t newSize) {
  CHRE_ASSERT(newSize <= kCapacity);
  if (newSize > kCapacity) {
    newSize = kCapacity;
  }

  if (newSize > size()) {
    for (size_t i = size(); i < newSize; i++) {
      emplace_back();
    }
  } else {
    for (size_t i = newSize; i < size(); i++) {
      data()[i].~ElementType();
    }

    mSize = newSize;
  }
}

}  // namespace chre

#endif  // CHRE_UTIL_FIXED_SIZE_VECTOR_IMPL_H_
