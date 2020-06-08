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

/*
 * Implementation Notes:
 * Each platform must supply a platform-specific platform_link.h to provide the
 * definitions and a platform-specific link.c to provide the implementation for
 * the definitions in this file.
 * The platform must also initialize the ChppPlatformLinkParameters for each
 * link (context.linkParams).
 */

#ifndef CHPP_LINK_H_
#define CHPP_LINK_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform-specific struct with link details / parameters.
 */
struct ChppPlatformLinkParameters;

/*
 * Platform-specific function to send Tx data over to the link layer.
 *
 * @param params Platform-specific struct with link details / parameters.
 * @param buf Data to be sent.
 * @param len Length of the data to be sent in bytes.
 *
 * @return True if the platform implementation for this function is synchronous,
 * i.e. it is done with buf and len once the function returns. A return value of
 * False indicates that this function is implemented asynchronously. In this
 * case, it is up to the platform implementation to call chppLinkSendDoneCb()
 * after processing the contents of buf and len.
 */
bool chppPlatformLinkSend(struct ChppPlatformLinkParameters *params,
                          uint8_t *buf, size_t len);

/*
 * Platform-specific function to reset a non-synchronous link, where the link
 * implementation is responsible for calling chppLinkSendDoneCb() after
 * processing the contents of buf and len. For such links, a reset called before
 * chppLinkSendDoneCb() indicates to the link to abort sending out buf, and that
 * the contents of buf and len will become invalid.
 *
 * @param params Platform-specific struct with link details / parameters.
 */
void chppPlatformLinkReset(struct ChppPlatformLinkParameters *params);

#ifdef __cplusplus
}
#endif

#include "chpp/platform/platform_link.h"

#endif  // CHPP_LINK_H_
