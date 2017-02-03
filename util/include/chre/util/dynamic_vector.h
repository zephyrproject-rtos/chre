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

#include "chre/util/non_copyable.h"

namespace chre {

/**
 * A container for storing a sequential array of elements. This container
 * resizes dynamically using heap allocations.
 */
template<typename ElementType>
class DynamicVector : public NonCopyable {
 public:
  /**
   * Default-constructs a dynamic vector.
   */
  DynamicVector();

  /**
   * Move-constructs a dynamic vector from another. The other dynamic vector is
   * left in an empty state.
   *
   * @param other The other vector to move from.
   */
  DynamicVector(DynamicVector<ElementType>&& other);

  /**
   * Destructs the objects and releases the memory owned by the vector.
   */
  ~DynamicVector();

  /**
   * Returns a pointer to the underlying buffer. Note that this should not be
   * considered to be persistent as the vector will be moved and resized
   * automatically.
   *
   * @return The pointer to the underlying buffer.
   */
  ElementType *data();

  /**
   * Returns a const pointer to the underlying buffer. Note that this should not
   * be considered to be persistent as the vector will be moved and resized
   * automatically.
   *
   * @return The const pointer to the underlying buffer.
   */
  const ElementType *data() const;

  /**
   * Returns the current number of elements in the vector.
   *
   * @return The number of elements in the vector.
   */
  size_t size() const;

  /**
   * Returns the maximum number of elements that can be stored in this vector
   * without a resize operation.
   *
   * @return The capacity of the vector.
   */
  size_t capacity() const;

  /**
   * Determines whether the vector is empty or not.
   *
   * @return true if the vector is empty.
   */
  bool empty() const;

  /**
   * Pushes an element onto the back of the vector. If the vector requires a
   * resize and that allocation fails this function will return false. All
   * iterators and references are invalidated if the container has been
   * resized. Otherwise, only the past-the-end iterator is invalidated.
   *
   * @param The element to push onto the vector.
   * @return true if the element was pushed successfully.
   */
  bool push_back(const ElementType& element);

  /**
   * Moves an element onto the back of the vector. If the vector requires a
   * resize and that allocation fails this function will return false. All
   * iterators and references are invalidated if the container has been
   * resized. Otherwise, only the past-the-end iterator is invalidated.
   *
   * @param The element to move onto the vector.
   * @return true if the element was moved successfully.
   */
  bool push_back(ElementType&& element);

  /**
   * Constructs an element onto the back of the vector. All iterators and
   * references are invalidated if the container has been resized. Otherwise,
   * only the past-the-end iterator is invalidated.
   *
   * @param The arguments to the constructor
   * @return true is the element is constructed successfully.
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
   * All iterators and references are invalidated unless the container did not
   * resize.
   *
   * @param The new capacity of the vector.
   * @return true if the resize operation was successful.
   */
  bool reserve(size_t newCapacity);

  /**
   * Inserts an element into the vector at a given index. If a resize of the
   * vector is required and the allocation fails, false will be returned. This
   * will shift all vector elements after the given index one position backward
   * in the list. The supplied index must be <= the size of the vector. It is
   * not possible to have a sparse list of items. If the index is > the current
   * size of the vector, false will be returned. All iterators and references
   * to and after the indexed element are invalidated. Iterators and references
   * to before the indexed elements are unaffected if the container did not resize.
   *
   * @param index The index to insert an element at.
   * @param element The element to insert.
   * @return Whether or not the insert operation was successful.
   */
  bool insert(size_t index, const ElementType& element);

  /**
   * Removes an element from the vector given an index. All elements after the
   * indexed one are moved forward one position. The destructor is invoked on
   * on the invalid item left at the end of the vector. The index passed in
   * must be less than the size() of the vector. If the index is greater than or
   * equal to the size no operation is performed. All iterators and references
   * to and after the indexed element are invalidated.
   *
   * @param index The index to remove an element at.
   */
  void erase(size_t index);

  /**
   * Searches the vector for an element.
   *
   * @param element The element to comare against.
   * @return The index of the element found. If the return is equal to size()
   *         then the element was not found.
   */
  size_t find(const ElementType& element) const;

  /**
   * Swaps the location of two elements stored in the vector. The indices
   * passed in must be less than the size() of the vector. If the index is
   * greater than or equal to the size, no operation is performed. All
   * iterators and references to these two indexed elements are invalidated.
   *
   * @param index0 The index of the first element
   * @param index1 The index of the second element
   */
  void swap(size_t index0, size_t index1);

  /**
   * Returns a reference to the last element in the vector. It is illegal to
   * call this on an empty vector.
   *
   * @return The last element in the vector.
   */
  ElementType& back();

  /**
   * Returns a const reference to the last element in the vector. It is illegal
   * to call this on an empty vector.
   *
   * @return The last element in the vector.
   */
  const ElementType& back() const;

  /**
   * Random-access iterator that points to some element in the container.
   */
  typedef ElementType* iterator;
  typedef const ElementType* const_iterator;

  /**
   * @return A random-access iterator to the beginning.
   */
  typename DynamicVector<ElementType>::iterator begin();
  typename DynamicVector<ElementType>::const_iterator cbegin() const;

  /**
   * @return A random-access iterator to the end.
   */
  typename DynamicVector<ElementType>::iterator end();
  typename DynamicVector<ElementType>::const_iterator cend() const;

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
