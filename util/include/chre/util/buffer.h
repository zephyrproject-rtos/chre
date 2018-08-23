/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef CHRE_UTIL_BUFFER_H_
#define CHRE_UTIL_BUFFER_H_

#include <cstddef>
#include <cstdint>

#include "chre/util/non_copyable.h"

#include "chre/util/memory.h"

namespace chre {

/**
 * Manages a buffer of objects. This buffer may be allocated by this object or
 * wrapped into this object. Usage of this template is restricted to POD types.
 *
 * The intent of this container is to group a pointer with the number of
 * elements in that pointer. This allows passing a pointer from one place to
 * another with an optional memory release when the copy_array API below is
 * used. Destructors are not called on the memory owned here, hence the
 * restriction to POD types. This is the C-equivalent to wrapping a void pointer
 * and size into a simple struct, but with the added benefit of type safety.
 */
template<typename ElementType>
class Buffer : public NonCopyable {
 public:
  static_assert(std::is_pod<ElementType>::value, "ElementType must be POD");

  /**
   * Cleans up for the buffer. If the buffer is currently owned by this object,
   * it is released.
   */
  ~Buffer() {
    if (mBufferRequiresFree) {
      memoryFree(mBuffer);
    }
  }

  /**
   * @return the data buffered here.
   */
  ElementType *data() const {
    return mBuffer;
  }

  /**
   * @return the number of elements in the underlying buffer.
   */
  size_t size() const {
    return mSize;
  }

  /**
   * Wraps an existing C-style array so it can be used as a Buffer. A
   * reference to the supplied array is kept, as opposed to making a copy. The
   * caller retains ownership of the memory. Calling code must therefore ensure
   * that the lifetime of the supplied array is at least as long as that of this
   * object, and that the memory is released after this object is destructed, as
   * this object will not attempt to free the memory itself.
   *
   * @param buffer A pointer to a pre-allocated array.
   * @param size The number of elements in the array.
   */
  void wrap(ElementType *buffer, size_t size) {
    // If buffer is nullptr, size must also be 0.
    CHRE_ASSERT(buffer != nullptr || size == 0);
    this->~Buffer();
    mBufferRequiresFree = false;
    mBuffer = buffer;
    mSize = size;
  }

  /**
   * Copies the supplied array into the buffer managed by this object. In the
   * interest of simplicity and codesize, the underlying buffer is always
   * reallocated. The expected use of this object is to copy just once. This
   * also avoids the issue of copying a very large buffer, then copying a
   * smaller buffer and being left with a very large outstanding allocation.
   *
   * @param buffer A pointer to an array to copy.
   * @param size The number of elements in the array.
   * @return true if capacity was reserved to fit the supplied buffer and the
   *         supplied buffer was copied into the internal buffer of this object,
   *         false otherwise.
   */
  bool copy_array(const ElementType *buffer, size_t size) {
    // If buffer is nullptr, size must also be 0.
    CHRE_ASSERT(buffer != nullptr || size == 0);

    bool success = false;
    this->~Buffer();
    mBufferRequiresFree = true;
    size_t copyBytes = size * sizeof(ElementType);
    mBuffer = static_cast<ElementType *>(memoryAlloc(copyBytes));
    if (mBuffer != nullptr) {
      memcpy(mBuffer, buffer, copyBytes);
      mSize = size;
      success = true;
    }

    return success;
  }

 private:
  //! The buffer to manage.
  ElementType *mBuffer = nullptr;

  //! The number of elements in the buffer.
  size_t mSize = 0;

  //! Set to true when mBuffer needs to be released by the destructor.
  bool mBufferRequiresFree = false;
};

}  // namespace chre

#endif  // CHRE_UTIL_BUFFER_H_
