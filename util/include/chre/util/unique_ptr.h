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

#ifndef CHRE_UTIL_UNIQUE_PTR_H_
#define CHRE_UTIL_UNIQUE_PTR_H_

#include "chre/util/non_copyable.h"

namespace chre {

/**
 * Wraps a pointer to a dynamically allocated object and manages the underlying
 * memory. The goal is to be similar to std::unique_ptr.
 */
template<typename ObjectType>
class UniquePtr : public NonCopyable {
 public:
  /**
   * Allocates and constructs a new object of type ObjectType on the heap. If
   * memory allocation fails the constructor is not invoked. This constructor is
   * similar to std::make_unique.
   *
   * @param args The arguments to pass to the object's constructor.
   */
  template<typename... Args>
  UniquePtr(Args&&... args);

  /**
   * Deconstructs the object (if necessary) and releases associated memory.
   */
  ~UniquePtr();

  /**
   * Determines if the object was constructed correctly.
   *
   * @return Returns true if the object is null and not constructed.
   */
  bool isNull() const;

  /**
   * @return A pointer to the underlying object or nullptr if this object
   * is not currently valid.
   */
  ObjectType *get() const;

  /**
   * @return A pointer to the underlying object.
   */
  ObjectType *operator->() const;

  /**
   * @return A reference to the underlying object.
   */
  ObjectType& operator*() const;

  /**
   * @param index The index of an object in the underlying array object.
   * @return A reference to the underlying object at an index.
   */
  ObjectType& operator[](size_t index) const;

  /**
   * Move assignment operator. Ownership of this object is transferred and the
   * other object is left in an invalid state.
   *
   * @param other The other object being moved.
   * @return A reference to the newly moved object.
   */
  UniquePtr<ObjectType>& operator=(UniquePtr<ObjectType>&& other);

 private:
  //! A pointer to the underlying storage for this object.
  ObjectType *mObject;
};

}  // namespace chre

#include "chre/util/unique_ptr_impl.h"

#endif  // CHRE_UTIL_UNIQUE_PTR_H_
