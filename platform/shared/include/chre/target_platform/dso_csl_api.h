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

#ifndef CHRE_PLATFORM_DSO_CSL_API_H_
#define CHRE_PLATFORM_DSO_CSL_API_H_

/**
 * @file
 * This provides the interface that the dynamic shared object (DSO) nanoapp
 * client support library (CSL) uses to interface with the underlying CHRE
 * implementation in a compatible manner. These functions are not directly
 * called by the nanoapp itself, but rather the nanoapp calls CHRE APIs, which
 * are implemented in the CSL to call through to APIs retrieved via this
 * interface. While originally intended for use with the SLPI, this API can be
 * applied to any platform where the nanoapp is a dynamic module that can
 * directly call system functions, but the nanoapp does not know ahead of time
 * which functions are implemented, therefore a layer of indirection is
 * necessary to avoid unresolved symbol errors when running on older platforms.
 *
 * Note that this is *not* required to implemented/supported on all CHRE
 * platforms, only those that use the DSO CSL.
 */

// TODO: sidenote - not planning on using this for Linux, at least not
// initially... we can just implement the CHRE APIs directly and compile
// nanoapps into the system executable for initial testing. Will need to use
// this for SLPI, though, for compatibility reasons. And once it's used on SLPI,
// it would make sense to have it there on Linux too, to make sure we are
// testing apples to apples.

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // TODO: populate with references to any function calls that were not present
  // in the initial API release, or could potentially be changed in the future.
  // note that we're not completely against the nanoapp calling directly into
  // the CHRE system for things that are unlikely to change; we would just need
  // to use this method after any change happens. consider the case where we
  // have:
  //   chreFoo(int x); // v1.0
  //   chreFoo(int x, int y); // v1.1
  // if nanoapps called chreFoo(int) directly @ v1.0, then that version of the
  // function must be kept like that indefinitely for v1.0 apps to use. v1.1
  // apps would need to go through this indirection to call into something like
  // chreFoo_v1_1

  char placeholder;
} chreSlpiCoreSystemApi;

typedef struct {
  // TODO
  char placeholder;
} chreSlpiGnssApi;

typedef struct {
  // TODO
  char placeholder;
} chreSlpiWifiApi;

typedef struct {
  // TODO
  char placeholder;
} chreSlpiWwanApi;

enum chreDsoCslApiId {
  CHRE_DSO_CSL_API_ID_CORE_SYSTEM = 1,
  CHRE_DSO_CSL_API_ID_SENSORS = 2,
  CHRE_DSO_CSL_API_ID_GNSS = 3,
  CHRE_DSO_CSL_API_ID_WIFI = 4,
  CHRE_DSO_CSL_API_ID_WWAN = 5,
};

/**
 * TODO
 *
 * @param apiId
 * @param apiHandle If this function returns true, this will be set to a pointer
 *        to the associated structure containing the API
 *
 * @return true if the requested API is supported, false otherwise
 */
extern "C" bool chreDsoCslGetApi(uint32_t apiId, void **apiHandle);

#ifdef __cplusplus
}
#endif


#endif  // CHRE_NANOAPP_API_H_
