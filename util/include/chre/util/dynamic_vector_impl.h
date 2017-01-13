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

#include <algorithm>
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
bool DynamicVector<ElementType>::push_back(ElementType&& element) {
  bool spaceAvailable = prepareForPush();
  if (spaceAvailable) {
    mData[mSize++] = std::move(element);
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
  CHRE_ASSERT(index < mSize);
  if (index >= mSize) {
    index = mSize - 1;
  }

  return data()[index];
}

template<typename ElementType>
const ElementType& DynamicVector<ElementType>::operator[](size_t index) const {
  CHRE_ASSERT(index < mSize);
  if (index >= mSize) {
    index = mSize - 1;
  }

  return data()[index];
}

/**
 *  Moves a range of data items. This is part of the template specialization for
 *  when the underlying type is move-assignable.
 *
 *  @param data The beginning of the data to move.
 *  @param count The number of data items to move.
 *  @param newData The location to move these items to.
 */
template<typename ElementType>
void moveOrCopy(ElementType *data, size_t count, ElementType *newData,
                std::true_type) {
  std::move(data, data + count, newData);
}

/**
 *  Copies a range of data items. This is part of the template specialization
 *  for when the underlying type is not move-assignable.
 *
 *  @param data The beginning of the data to copy.
 *  @param count The number of data items to copy.
 *  @param newData The location to copy these items to.
 */
template<typename ElementType>
void moveOrCopy(ElementType *data, size_t count, ElementType *newData,
                std::false_type) {
  std::copy(data, data + count, newData);
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

  moveOrCopy(mData, mSize, newData,
             typename std::is_move_assignable<ElementType>::type());

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
  CHRE_ASSERT(index < mSize);
  if (index < mSize) {
    mSize--;
    for (size_t i = index; i < mSize; i++) {
      mData[i] = std::move(mData[i + 1]);
    }

    mData[mSize].~ElementType();
  }
}

template<typename ElementType>
size_t DynamicVector<ElementType>::find(const ElementType& element) const {
  // TODO: Consider adding iterator support and making this a free function.
  for (size_t i = 0; i < size(); i++) {
    if (mData[i] == element) {
      return i;
    }
  }

  return size();
}

template<typename ElementType>
void DynamicVector<ElementType>::swap(size_t index0, size_t index1) {
  CHRE_ASSERT(index0 < mSize && index1 < mSize);
  if (index0 < mSize && index1 < mSize) {
    ElementType temp = std::move(mData[index0]);
    mData[index0] = std::move(mData[index1]);
    mData[index1] = std::move(temp);
  }
}

template<typename ElementType>
ElementType& DynamicVector<ElementType>::back() {
  CHRE_ASSERT(mSize > 0);
  return mData[mSize - 1];
}

template<typename ElementType>
const ElementType& DynamicVector<ElementType>::back() const {
  CHRE_ASSERT(mSize > 0);
  return mData[mSize - 1];
}

template<typename ElementType>
typename DynamicVector<ElementType>::iterator DynamicVector<ElementType>::begin() {
  return mData;
}

template<typename ElementType>
typename DynamicVector<ElementType>::iterator DynamicVector<ElementType>::end() {
  return (mData + mSize);
}

template<typename ElementType>
typename DynamicVector<ElementType>::const_iterator DynamicVector<ElementType>::
    begin() const {
  return mData;
}

template<typename ElementType>
typename DynamicVector<ElementType>::const_iterator DynamicVector<ElementType>::
    end() const {
  return (mData + mSize);
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
