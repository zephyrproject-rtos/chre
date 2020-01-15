/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef CHPP_MEMORY_H_
#define CHPP_MEMORY_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate memory
 *
 * TODO: Move to platform specific injectable interface
 */
void *chppMalloc(const size_t size) {
  return malloc(size);
}

/**
 * Reallocate memory.
 * Ideally should use realloc() for systems that efficiently support it.
 *
 * Sample implementation for systems that don't support realloc():
 *
 * void *newPtr = NULL;
 * if (newSize == 0) {
 *   free(oldPtr);
 * } else {
 *   newPtr = malloc(newSize);
 *   if (newPtr) {
 *     newPtr = memcpy(newPtr, oldPtr, MIN(oldSize, newSize));
 *     free(oldPtr);
 *   }
 * }
 * return newPtr;
 *
 * TODO: Move to platform specific injectable interface
 * TODO: Store fragments separately (e.g. linked list) and bubble up all of them
 */
void *chppRealloc(void *oldPtr, const size_t newSize, const size_t oldSize) {
  UNUSED_VAR(oldSize);
  return realloc(oldPtr, newSize);
}

#ifdef __cplusplus
}
#endif

#endif  // CHPP_MEMORY_H_
