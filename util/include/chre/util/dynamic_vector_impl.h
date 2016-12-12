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

#ifndef CHRE_UTIL_DYNAMIC_VECTOR_IMPL_H_
#define CHRE_UTIL_DYNAMIC_VECTOR_IMPL_H_

#include <utility>

#include "chre/platform/assert.h"
#include "chre/platform/memory.h"
#include "chre/util/dynamic_vector.h"

namespace chre {

template<typename ElementType>
DynamicVector<ElementType>::~DynamicVector() {
  for (size_t i = 0; i < mSize; i++) {
    mData[i].~ElementType();
  }

  memoryFree(mData);
}

template<typename ElementType>
ElementType *DynamicVector<ElementType>::data() {
  return mData;
}

template<typename ElementType>
const ElementType *DynamicVector<ElementType>::data() const {
  return mData;
}

template<typename ElementType>
size_t DynamicVector<ElementType>::size() const {
  return mSize;
}

template<typename ElementType>
size_t DynamicVector<ElementType>::capacity() const {
  return mCapacity;
}

template<typename ElementType>
bool DynamicVector<ElementType>::empty() const {
  return (mSize == 0);
}

template<typename ElementType>
bool DynamicVector<ElementType>::push_back(const ElementType& element) {
  bool spaceAvailable = prepareForPush();
  if (spaceAvailable) {
    mData[mSize++] = element;
  }

  return spaceAvailable;
}

template<typename ElementType>
template<typename... Args>
bool DynamicVector<ElementType>::emplace_back(Args&&... args) {
  bool spaceAvailable = prepareForPush();
  if (spaceAvailable) {
    new (&data()[mSize++]) ElementType(std::forward<Args>(args)...);
  }

  return spaceAvailable;
}

template<typename ElementType>
ElementType& DynamicVector<ElementType>::operator[](size_t index) {
  ASSERT(index < mSize);
  if (index >= mSize) {
    index = mSize - 1;
  }

  return data()[index];
}

template<typename ElementType>
const ElementType& DynamicVector<ElementType>::operator[](size_t index) const {
  ASSERT(index < mSize);
  if (index >= mSize) {
    index = mSize - 1;
  }

  return data()[index];
}

template<typename ElementType>
bool DynamicVector<ElementType>::reserve(size_t newCapacity) {
  // If the new capacity is less than or equal to the current capacitywe can
  // avoid any memory allocation/deallocation/copying.
  if (newCapacity <= mCapacity) {
    return true;
  }

  ElementType *newData = static_cast<ElementType *>(
      memoryAlloc(newCapacity * sizeof(ElementType)));
  if (newData == nullptr) {
    return false;
  }

  std::copy(mData, mData + mSize, newData);
  memoryFree(mData);
  mData = newData;
  mCapacity = newCapacity;
  return true;
}

template<typename ElementType>
bool DynamicVector<ElementType>::insert(size_t index,
    const ElementType& element) {
  // Ensure that the user is not trying to insert an element after the
  // contiguous block of elements.
  if (index > mSize) {
    return false;
  }

  // Allocate space if needed.
  if (!prepareForPush()) {
    return false;
  }

  // Shift all elements starting at the given index backward one position.
  for (size_t i = mSize; i > index; i--) {
    mData[i] = std::move(mData[i - 1]);
  }

  mData[index].~ElementType();

  // Insert the new element.
  mData[index] = element;
  mSize++;
  return true;
}

template<typename ElementType>
void DynamicVector<ElementType>::erase(size_t index) {
  ASSERT(index < mSize);
  if (index < mSize) {
    mSize--;
    for (size_t i = index; i < mSize; i++) {
      mData[i] = std::move(mData[i + 1]);
    }

    mData[mSize].~ElementType();
  }
}

template<typename ElementType>
bool DynamicVector<ElementType>::prepareForPush() {
  bool spaceAvailable = true;
  if (mSize == mCapacity) {
    size_t newCapacity = mCapacity * 2;
    if (newCapacity == 0) {
      newCapacity = 1;
    }

    if (!reserve(newCapacity)) {
      spaceAvailable = false;
    }
  }

  return spaceAvailable;
}

}  // namespace chre

#endif  // CHRE_UTIL_DYNAMIC_VECTOR_IMPL_H_
