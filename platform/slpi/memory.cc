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

#include "chre/platform/memory.h"
#include "chre/platform/slpi/memory.h"

#include <cstdlib>

extern "C" {

#if defined(CHRE_SLPI_SMGR) || defined (CHRE_SLPI_SEE)
#include "sns_memmgr.h"
#endif

} // extern "C"

namespace chre {

void *memoryAlloc(size_t size) {
#ifdef CHRE_SLPI_UIMG_ENABLED
  #if defined(CHRE_SLPI_SMGR)
    return SNS_OS_U_MALLOC(SNS_CHRE, size);
  #elif defined(CHRE_SLPI_SEE)
    return sns_malloc(SNS_HEAP_ISLAND, size);
  #else
    #error SLPI UIMG memory allocation not supported
  #endif
#else
  return malloc(size);
#endif // CHRE_SLPI_UIMG_ENABLED
}

void *memoryAllocBigImage(size_t size) {
  return malloc(size);
}

void *palSystemApiMemoryAlloc(size_t size) {
  return malloc(size);
}

void memoryFree(void *pointer) {
#ifdef CHRE_SLPI_UIMG_ENABLED
  #if defined(CHRE_SLPI_SMGR)
    SNS_OS_FREE(pointer);
  #elif defined(CHRE_SLPI_SEE)
    sns_free(pointer);
  #else
    #error SLPI UIMG memory free not supported
  #endif
#else
  free(pointer);
#endif // CHRE_SLPI_UIMG_ENABLED
}

void memoryFreeBigImage(void *pointer) {
  free(pointer);
}

void palSystemApiMemoryFree(void *pointer) {
  free(pointer);
}

}  // namespace chre
