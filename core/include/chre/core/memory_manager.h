/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef CHRE_CORE_MEMORY_MANAGER_H_
#define CHRE_CORE_MEMORY_MANAGER_H_

#include <cstddef>
#include <cstdint>

#include "chre/core/nanoapp.h"
#include "chre/util/non_copyable.h"

namespace chre {

/**
 * The MemoryManager keeps track of heap memory allocated/deallocated by all
 * nanoapps.
 * TODO: Free memory space when nanoapps are unloaded.
 * TODO: Move this implementation to platform-specific code area.
 *
 */
class MemoryManager : public NonCopyable {
 public:
  /**
   * Initializes a MemoryManager.
   */
  MemoryManager();

  /**
   * Allocate heap memory in CHRE.
   * @param bytes The size in bytes to allocate.
   * @param app Pointer to the nanoapp requesting memory.
   * @return the allocated memory pointer. nullptr if the allocation fails.
   */
  void *nanoappAlloc(Nanoapp *app, uint32_t bytes);

  /**
   * Free heap memory in CHRE.
   * @param ptr The pointer to the memory to deallocate.
   */
  void nanoappFree(void *ptr);

  /**
   * @return current total allocated memory in bytes.
   */
  size_t getTotalAllocatedBytes() const;

  /**
   * @return current count of allocated memory spaces.
   */
  size_t getAllocationCount() const;

  /**
   * @return max total allocatable memory in bytes.
   */
  size_t getMaxAllocationBytes() const;

  /**
   * @return max allocatable memory counts.
   */
  size_t getMaxAllocationCount() const;

 private:
  /**
   * Header to store allocation details for tracking.
   * We use a union to ensure proper memory alignment.
   */
  union AllocHeader {
    struct {
      //! The amount of memory in bytes allocated (not including header).
      uint32_t bytes;

      //! The ID of nanoapp requesting memory allocation.
      uint32_t instanceId;
    } data;

    //! Makes sure header is a multiple of the size of max_align_t
    max_align_t aligner;
  };

  //! Stores total allocated memory in bytes (not including header).
  size_t mTotalAllocatedBytes;

  //! Stores total number of allocated memory spaces.
  size_t mAllocationCount;

  //! The maximum allowable total allocated memory in bytes for all nanoapps.
  static constexpr size_t kMaxAllocationBytes = (128 * 1024);

  //! The maximum allowable count of memory allocations for all nanoapps.
  static constexpr size_t kMaxAllocationCount = (8 * 1024);
};

}  // namespace chre

#endif  // CHRE_CORE_MEMORY_MANAGER_H_
