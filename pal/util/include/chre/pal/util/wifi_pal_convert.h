/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef CHRE_PAL_WIFI_PAL_CONVERT_H_
#define CHRE_PAL_WIFI_PAL_CONVERT_H_

/**
 * @file
 * Defines helper functions to convert data into CHRE-defined structures.
 *
 * These functions can be used by the CHRE WiFi PAL implementation to help
 * convert WLAN data to CHRE-defined structures so they can be delivered through
 * the PAL interface.
 */

#include <stdbool.h>
#include <stdint.h>

#include "chre_api/chre/wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

// The bit-level definitions of the LCI IE data specified by the
// IEEE P802.11-REVmc/D8.0.
// | Element name (number of bits) |
// -------------------------------------------------------------
// | Latitude uncertainty (6) | Latitude (34)
// | Longitude uncertainty (6) | Longitude (34)
// | Altitude type (4) | Altitude uncertainty (6)
// | Altitude (30) | Datum (3) | RegLog Agreement (1)
// | RegLog DSE (1) | Dependent STA (1)
// | Version (2)
#define CHRE_LCI_IE_DATA_LEN_BYTES 16

/**
 * Converts LCI IE data specified by IEEE P802.11-REVmc/D8.0 spec section
 * 9.4.2.22, under Measurement Report Element.
 *
 * @param ieData The LCI IE data array.
 * @param len The length of ieData in bytes.
 * @param out A non-null pointer to store the conversion result.
 *
 * @return true if the conversion succeeded.
 */
bool chreWifiLciFromIe(const uint8_t *ieData, size_t len,
                       struct chreWifiRangingResult *out);

#ifdef __cplusplus
}
#endif

#endif  // CHRE_PAL_WIFI_PAL_CONVERT_H_
