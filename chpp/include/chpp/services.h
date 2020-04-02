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

#ifndef CHPP_SERVICES_H_
#define CHPP_SERVICES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "chpp/platform/log.h"

#include "chpp/app.h"
#include "chpp/macros.h"
#include "chpp/memory.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ChppAppState;
struct ChppService;

/************************************************
 *  Public Definitions
 ***********************************************/

/**
 * Uses chppAllocServiceResponse() to allocate a variable-length response
 * message of a specific type.
 *
 * @param requestHeader client request header, as per
 * chppAllocServiceResponse().
 * @param type Type of response which includes an arrayed member.
 * @param count number of items in the array of arrayField.
 * @param arrayField The arrayed member field.
 *
 * @return Pointer to allocated memory
 */
#define chppAllocServiceResponseTypedArray(requestHeader, type, count, \
                                           arrayField)                 \
  (type *)chppAllocServiceResponse(                                    \
      requestHeader,                                                   \
      sizeof(type) + (count)*sizeof_member(type, arrayField[0]))

/**
 * Uses chppAllocServiceResponse() to allocate a response message of a specific
 * type and its corresponding length.
 *
 * @param requestHeader client request header, as per
 * chppAllocServiceResponse().
 * @param type Type of response.
 *
 * @return Pointer to allocated memory
 */
#define chppAllocServiceResponseFixed(requestHeader, type) \
  (type *)chppAllocServiceResponse(requestHeader, sizeof(type))

/************************************************
 *  Public functions
 ***********************************************/

/**
 * Registers common services with the CHPP app layer. These services are enabled
 * by CHPP_SERVICE_ENABLED_xxxx definitions. This function is automatically
 * called by chppAppInit().
 *
 * @param context Maintains status for each app layer instance.
 * @param newService The service to be registered on this platform.
 */
void chppRegisterCommonServices(struct ChppAppState *context);

/**
 * Registers a new service on CHPP. This function is to be called by the
 * platform initialization code for every non-common service available on a
 * server (if any), i.e. except those that are registered through
 * chppRegisterCommonServices().
 *
 * Note that the maximum number of services that can be registered on a platform
 * can specified as CHPP_MAX_REGISTERED_SERVICES by the initialization code.
 * Otherwise, a default value will be used.
 *
 * @param context Maintains status for each app layer instance.
 * @param newService The service to be registered on this platform.
 *
 * @return Handle number of the registered service, or zero indicating failure.
 */
void chppRegisterService(struct ChppAppState *context,
                         const struct ChppService *newService);

/**
 * Allocates a response message of a specified length, populating the (app
 * layer) server response header accorging to the provided client request (app
 * layer) header.
 *
 * It is expected that for most use cases, the chppAllocServiceResponseFixed()
 * or chppAllocServiceResponseTypedArray() macros shall be used rather than
 * calling this function directly.
 *
 * @param requestHeader client request header
 * @param len Length of the response message (including header) in bytes. Note
 * that the specified length must be at least equal to the lendth of the app
 * layer header.
 *
 * @return Pointer to allocated memory
 */
struct ChppAppHeader *chppAllocServiceResponse(
    const struct ChppAppHeader *requestHeader, size_t len);

#ifdef __cplusplus
}
#endif

#endif  // CHPP_SERVICES_H_
