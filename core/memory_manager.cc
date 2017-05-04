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

#include "chre/core/memory_manager.h"

namespace chre {

MemoryManager::MemoryManager()
    : mTotalAllocatedBytes(0), mAllocationCount(0) {}

void *MemoryManager::nanoappAlloc(Nanoapp *app, uint32_t bytes) {
  // TODO: Make this thread-safe (only needed if nanoapps execute out of
  // multiple threads)
  AllocHeader *header = nullptr;
  if (bytes > 0) {
    if (mAllocationCount >= kMaxAllocationCount) {
      LOGE("Failed to allocate memory from Nanoapp ID %" PRIu32
           ": allocation count exceeded limit.", app->getInstanceId());
    } else if ((mTotalAllocatedBytes + bytes) > kMaxAllocationBytes) {
      LOGE("Failed to allocate memory from Nanoapp ID %" PRIu32
           ": not enough space.", app->getInstanceId());
    } else {
      header = static_cast<AllocHeader*>(
          chre::memoryAlloc(sizeof(AllocHeader) + bytes));

      if (header != nullptr) {
        mTotalAllocatedBytes += bytes;
        mAllocationCount++;
        header->data.bytes = bytes;
        header->data.instanceId = app->getInstanceId();
        header++;
      }
    }
  }
  return header;
}

void MemoryManager::nanoappFree(void *ptr) {
  // TODO: Make this thread-safe (only needed if nanoapps execute out of
  // multiple threads)
  if (ptr != nullptr) {
    AllocHeader *header = static_cast<AllocHeader*>(ptr);
    header--;

    if (mTotalAllocatedBytes >= header->data.bytes) {
      mTotalAllocatedBytes -= header->data.bytes;
    } else {
      mTotalAllocatedBytes = 0;
    }
    if (mAllocationCount > 0) {
      mAllocationCount--;
    }

    memoryFree(header);
  }
}

size_t MemoryManager::getTotalAllocatedBytes() const {
  return mTotalAllocatedBytes;
}

size_t MemoryManager::getAllocationCount() const {
  return mAllocationCount;
}

size_t MemoryManager::getMaxAllocationBytes() const {
  return kMaxAllocationBytes;
}

size_t MemoryManager::getMaxAllocationCount() const {
  return kMaxAllocationCount;
}

}  // namespace chre
