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

#include "chre/util/dynamic_vector.h"

namespace chre {

template<typename ElementType>
ElementType *DynamicVector<ElementType>::data() const {
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

}  // namespace chre

#endif  // CHRE_UTIL_DYNAMIC_VECTOR_IMPL_H_
