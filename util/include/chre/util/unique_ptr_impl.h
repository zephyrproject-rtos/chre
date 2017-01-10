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

#ifndef CHRE_UTIL_UNIQUE_PTR_IMPL_H_
#define CHRE_UTIL_UNIQUE_PTR_IMPL_H_

#include <utility>

#include "chre/platform/memory.h"

namespace chre {

template<typename ObjectType>
template<typename... Args>
UniquePtr<ObjectType>::UniquePtr(Args&&... args) {
  mObject = static_cast<ObjectType *>(memoryAlloc(sizeof(ObjectType)));
  if (mObject != nullptr) {
    new (mObject) ObjectType(std::forward<Args>(args)...);
  }
}

template<typename ObjectType>
UniquePtr<ObjectType>::~UniquePtr() {
  if (mObject != nullptr) {
    mObject->~ObjectType();
    memoryFree(mObject);
    mObject = nullptr;
  }
}

template<typename ObjectType>
bool UniquePtr<ObjectType>::isNull() const {
  return (mObject == nullptr);
}

template<typename ObjectType>
ObjectType *UniquePtr<ObjectType>::get() const {
  return mObject;
}

template<typename ObjectType>
ObjectType *UniquePtr<ObjectType>::operator->() const {
  return get();
}

template<typename ObjectType>
ObjectType& UniquePtr<ObjectType>::operator*() const {
  return *get();
}

template<typename ObjectType>
ObjectType& UniquePtr<ObjectType>::operator[](size_t index) const {
  return get()[index];
}

template<typename ObjectType>
UniquePtr<ObjectType>& UniquePtr<ObjectType>::operator=(
    UniquePtr<ObjectType>&& other) {
  this->~UniquePtr<ObjectType>();
  mObject = other.mObject;
  other.mObject = nullptr;
  return *this;
}

}  // namespace chre

#endif  // CHRE_UTIL_UNIQUE_PTR_IMPL_H_
