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

#ifndef CHRE_WIFI_OFFLOAD_CHRE_TYPES_H_
#define CHRE_WIFI_OFFLOAD_CHRE_TYPES_H_

/*
 * This file defines all the data types and definitions that are neccessary for
 * successfully building flatbuffers serialization functions for offload HAL
 * without exposing the entire chre library.
 *
 */

#ifndef BUILD_FOR_CHRE_WIFI_OFFLOAD  // Only when building for offload HAL

/**
 * The maximum number of octets in an SSID (see 802.11 7.3.2.1)
 */
#define CHRE_WIFI_SSID_MAX_LEN (32)

/**
 * The number of octets in a BSSID (see 802.11 7.1.3.3.3)
 */
#define CHRE_WIFI_BSSID_LEN (6)

/**
 * Set of flags which can either indicate a frequency band. Specified as a bit
 * mask to allow for combinations in future API versions.
 * @defgroup CHRE_WIFI_BAND_MASK
 * @{
 */

#define CHRE_WIFI_BAND_MASK_2_4_GHZ UINT8_C(1 << 0)  //!< 2.4 GHz
#define CHRE_WIFI_BAND_MASK_5_GHZ UINT8_C(1 << 1)    //!< 5 GHz

/** @} */

/**
 * Identifies a WiFi frequency band
 */
enum chreWifiBand {
  CHRE_WIFI_BAND_2_4_GHZ = CHRE_WIFI_BAND_MASK_2_4_GHZ,
  CHRE_WIFI_BAND_5_GHZ = CHRE_WIFI_BAND_MASK_5_GHZ,
};

/**
 * SSID with an explicit length field, used when an array of SSIDs is supplied.
 */
struct chreWifiSsidListItem {
  //! Number of valid bytes in ssid. Valid range [0, CHRE_WIFI_SSID_MAX_LEN]
  uint8_t ssidLen;

  //! Service Set Identifier (SSID)
  uint8_t ssid[CHRE_WIFI_SSID_MAX_LEN];
};

/**
 * Identifies the authentication methods supported by an AP. Note that not every
 * combination of flags may be possible. Based on WIFI_PNO_AUTH_CODE_* from
 * hardware/libhardware_legacy/include/hardware_legacy/gscan.h in Android.
 * @defgroup CHRE_WIFI_SECURITY_MODE_FLAGS
 * @{
 */

#define CHRE_WIFI_SECURITY_MODE_UNKONWN UINT8_C(0)

#define CHRE_WIFI_SECURITY_MODE_OPEN UINT8_C(1 << 0)  //!< No auth/security
#define CHRE_WIFI_SECURITY_MODE_WEP UINT8_C(1 << 1)
#define CHRE_WIFI_SECURITY_MODE_PSK UINT8_C(1 << 2)  //!< WPA-PSK or WPA2-PSK
#define CHRE_WIFI_SECURITY_MODE_EAP UINT8_C(1 << 3)  //!< Any type of EAPOL

/** @} */

/**
 * Indicates the BSS operating channel width determined from the VHT and/or HT
 * Operation elements. Refer to VHT 8.4.2.161 and HT 7.3.2.57.
 */
enum chreWifiChannelWidth {
  CHRE_WIFI_CHANNEL_WIDTH_20_MHZ = 0,
  CHRE_WIFI_CHANNEL_WIDTH_40_MHZ = 1,
  CHRE_WIFI_CHANNEL_WIDTH_80_MHZ = 2,
  CHRE_WIFI_CHANNEL_WIDTH_160_MHZ = 3,
  CHRE_WIFI_CHANNEL_WIDTH_80_PLUS_80_MHZ = 4,
};

/**
 * Provides information about a single access point (AP) detected in a scan.
 */
struct chreWifiScanResult {
  //! Number of milliseconds prior to referenceTime in the enclosing
  //! chreWifiScanEvent struct when the probe response or beacon frame that
  //! was used to populate this structure was received.
  uint32_t ageMs;

  //! Capability Information field sent by the AP (see 802.11 7.3.1.4). This
  //! field must reflect native byte order and bit ordering, such that
  //! (capabilityInfo & 1) gives the bit for the ESS subfield.
  uint16_t capabilityInfo;

  //! Number of valid bytes in ssid. Valid range [0, CHRE_WIFI_SSID_MAX_LEN]
  uint8_t ssidLen;

  //! Service Set Identifier (SSID), a series of 0 to 32 octets identifying
  //! the access point. Note that this is commonly a human-readable ASCII
  //! string, but this is not the required encoding per the standard.
  uint8_t ssid[CHRE_WIFI_SSID_MAX_LEN];

  //! Basic Service Set Identifier (BSSID), represented in big-endian byte
  //! order, such that the first octet of the OUI is accessed in byte index 0.
  uint8_t bssid[CHRE_WIFI_BSSID_LEN];

  //! A set of flags from CHRE_WIFI_SCAN_RESULT_FLAGS_*
  uint8_t flags;

  //! RSSI (Received Signal Strength Indicator), in dBm. Typically negative.
  int8_t rssi;

  //! Operating band, set to a value from enum chreWifiBand
  uint8_t band;

  /**
   * Indicates the center frequency of the primary 20MHz channel, given in
   * MHz. This value is derived from the channel number via the formula:
   *
   *     primaryChannel (MHz) = CSF + 5 * primaryChannelNumber
   *
   * Where CSF is the channel starting frequency (in MHz) given by the
   * operating class/band (i.e. 2407 or 5000), and primaryChannelNumber is the
   * channel number in the range [1, 200].
   *
   * Refer to VHT 22.3.14.
   */
  uint32_t primaryChannel;

  /**
   * If the channel width is 20 MHz, this field is not relevant and set to 0.
   * If the channel width is 40, 80, or 160 MHz, then this denotes the channel
   * center frequency (in MHz). If the channel is 80+80 MHz, then this denotes
   * the center frequency of segment 0, which contains the primary channel.
   * This value is derived from the frequency index using the same formula as
   * for primaryChannel.
   *
   * Refer to VHT 8.4.2.161, and VHT 22.3.14.
   *
   * @see #primaryChannel
   */
  uint32_t centerFreqPrimary;

  /**
   * If the channel width is 80+80MHz, then this denotes the center frequency
   * of segment 1, which does not contain the primary channel. Otherwise, this
   * field is not relevant and set to 0.
   *
   * @see #centerFreqPrimary
   */
  uint32_t centerFreqSecondary;

  //! @see #chreWifiChannelWidth
  uint8_t channelWidth;

  //! Flags from CHRE_WIFI_SECURITY_MODE_* indicating supported authentication
  //! and associated security modes
  //! @see CHRE_WIFI_SECURITY_MODE_FLAGS
  uint8_t securityMode;

  //! Reserved; set to 0
  uint8_t reserved[10];
};

#endif  // BUILD_FOR_CHRE_WIFI_OFFLOAD

#endif  // CHRE_WIFI_OFFLOAD_CHRE_TYPES_H_
