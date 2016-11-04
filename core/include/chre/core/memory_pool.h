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

#ifndef CHRE_CORE_MEMORY_POOL_H_
#define CHRE_CORE_MEMORY_POOL_H_

/**
 * @file
 * TODO: placeholder for memory pool (slab allocator) to be used for frequently
 * created objects (like events)
 *
 * TODO:
 *  - needs to be thread safe (or there needs to be a thread safe version)
 *  - would it help in using some sort of template for placement new type stuff?
 *    like std::allocator_traits
 *  - we should handle construction w/non-default args inside the memory
 *    pool, e.g. http://en.cppreference.com/w/cpp/memory/allocator_traits/construct
 *    likely using std::forward, so we can allocate + construct in one function
 *    call
 *  - TBD whether we have different fixed/non-fixed size versions...
 */

#include <cstdlib>

namespace chre {

template<typename T>
class MemoryPool {
 public:
  // TODO: these really shouldn't be static... just making them static for now
  // until we figure out who owns the pool, who needs to access it, etc.
  static T *allocate() {
    return static_cast<T*>(malloc(sizeof(T)));
  }

  static void deallocate(T *item) {
    free(item);
  }
};

}  // namespace chre

#endif  // CHRE_CORE_MEMORY_POOL_H_
