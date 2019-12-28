/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef CHPP_TRANSPORT_H_
#define CHPP_TRANSPORT_H_

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "chpp/log.h"
#include "chpp/macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************
 *  Public Definitions
 ***********************************************/

/**
 * CHPP Transport header flags bitmap
 *
 * @defgroup CHPP_TRANSPORT_FLAG
 * @{
 */
// Set if a packet is part of a fragmented message except for the last fragment
#define CHPP_TRANSPORT_FLAG_UNFINISHED_FRAGMENT 0x01
// Set for first packet after bootup or to reset after irrecoverable error
#define CHPP_TRANSPORT_FLAG_RESET 0x02
// Reserved for future use
#define CHPP_TRANSPORT_FLAG_RESERVED 0xfc
/** @} */

/**
 * Preamble (i.e. packet start delimiter).
 * Any future backwards-incompatible versions of CHPP Transport will use a
 * different preamble.
 *
 * @defgroup CHPP_PREAMBLE
 * @{
 */
#define CHPP_PREAMBLE_DATA 0x6843
#define CHPP_PREAMBLE_LEN_BYTES 2
/** @} */

/**
 * Error codes optionally reported in ChppTransportHeader
 */
enum ChppErrorCode {
  // No error reported (either ACK or implicit NACK)
  CHPP_ERROR_NONE = 0,
  // Checksum failure
  CHPP_ERROR_CHECKSUM = 1,
  // Invalid header
  CHPP_ERROR_HEADER = 2,
  // Busy
  CHPP_ERROR_BUSY = 3,
  // Out of memory
  CHPP_ERROR_OOM = 4,
};

/**
 * CHPP Transport Layer header (not including the preamble)
 */
CHPP_PACKED_START
struct ChppTransportHeader {
  // Flags bitmap, defined as CHPP_TRANSPORT_FLAG_...
  uint8_t flags;

  // Error info (2 nibbles)
  // LS Nibble: Defined in ChppErrorCode enum
  // MS Nibble: Reserved
  uint8_t errorCode;

  // Next expected sequence number for a payload-bearing packet
  uint8_t ackSeq;

  // Sequence number
  uint8_t seq;

  // Payload length in bytes (not including header / footer)
  uint16_t length;

  // Reserved? TBD
  uint16_t reserved;
} CHPP_PACKED_ATTR;
CHPP_PACKED_END

/**
 * CHPP Transport Layer footer (containing the checksum)
 */
CHPP_PACKED_START
struct ChppTransportFooter {
  // Checksum algo TBD. Maybe IEEE CRC-32?
  uint32_t checksum;
} CHPP_PACKED_ATTR;
CHPP_PACKED_END

/************************************************
 *  Public functions
 ***********************************************/

/**
 * Processes all incoming data on the serial port based on the Rx state.
 * stream. Checks checksum, triggering the correct response (ACK / NACK).
 * Moves the state to CHPP_STATE_PREAMBLE afterwards.
 *
 * TODO: Add requirements, e.g. must not be interrupt context, must always be
 * called from the same thread
 *
 * TODO: Add sufficient outward facing documentation
 *
 * @param buf Input data
 * @param len Length of input data in bytes
 *
 * @return true informs the serial port driver that we are waiting for a
 * preamble. This allows the driver to (optionally) filter incoming zeros and
 * save processing
 */
bool chppRxData(const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif  // CHPP_TRANSPORT_H_
