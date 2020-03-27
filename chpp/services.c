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

#include "chpp/services.h"
#include "chpp/services/wwan.h"

/************************************************
 *  Prototypes
 ***********************************************/

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Convert UUID to a human-readable, null-terminated string.
 *
 * @param uuid Input UUID
 * @param strOut Output null-terminated string
 */
void uuidToStr(const uint8_t uuid[CHPP_SERVICE_UUID_LEN],
               char strOut[CHPP_SERVICE_UUID_STRING_LEN]) {
  sprintf(
      strOut,
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
      uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14],
      uuid[15]);
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppRegisterCommonServices(struct ChppAppState *context) {
#ifdef CHPP_SERVICE_ENABLED_WWAN
  chppWwanServiceInit(context);
#endif

#ifdef CHPP_SERVICE_ENABLED_WLAN
  chppWlanServiceInit(context);
#endif

#ifdef CHPP_SERVICE_ENABLED_GNSS
  chppGnssServiceInit(context);
#endif
}

void chppRegisterService(struct ChppAppState *context,
                         const struct ChppService *newService) {
  CHPP_NOT_NULL(newService);

  if (context->registeredServiceCount >= CHPP_MAX_REGISTERED_SERVICES) {
    LOGE("Cannot register new service # %d. Already hit maximum",
         context->registeredServiceCount);
    CHPP_ASSERT();

  } else {
    context->registeredServices[context->registeredServiceCount] = newService;
    context->registeredServiceCount++;

    char uuidText[CHPP_SERVICE_UUID_STRING_LEN];
    uuidToStr(newService->uuid, uuidText);
    LOGI(
        "Registered service %d on handle %x with name=%s, UUID=%s, "
        "version=%d.%d.%d, min_len=%d ",
        context->registeredServiceCount,
        context->registeredServiceCount + CHPP_HANDLE_NEGOTIATED_RANGE_START,
        newService->name, uuidText, newService->versionMajor,
        newService->versionMinor, newService->versionPatch,
        newService->minLength);
  }
}
