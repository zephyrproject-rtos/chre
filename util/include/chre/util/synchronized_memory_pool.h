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

#ifndef CHRE_UTIL_SYNCHRONIZED_MEMORY_POOL_H_
#define CHRE_UTIL_SYNCHRONIZED_MEMORY_POOL_H_

#include "chre/platform/mutex.h"
#include "chre/util/memory_pool.h"

namespace chre {

/**
 * This is a thread-safe version of the MemoryPool.
 */
template<typename ElementType, size_t kSize>
class SynchronizedMemoryPool : public NonCopyable {
 public:
  // TODO: Implement this class.

 private:
  Mutex mMutex;
  MemoryPool<ElementType, kSize> mMemoryPool;
};

}  // namespace chre

#include "chre/util/synchronized_memory_pool_impl.h"

#endif  // CHRE_UTIL_SYNCHRONIZED_MEMORY_POOL_H_
