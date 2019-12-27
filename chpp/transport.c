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

#include "transport.h"

/************************************************
 *  Private Constants
 ***********************************************/

/************************************************
 *  Private Type Definitions
 ***********************************************/

enum ChppRxState {
  // Waiting for, or processing, the preamble (i.e. packet start delimiter)
  // Moves to CHPP_STATE_HEADER as soon as it has seen a complete preamble.
  CHPP_STATE_PREAMBLE = 0,

  // Processing the packet header. Moves to CHPP_STATE_PAYLOAD after processing
  // the expected length of the header.
  CHPP_STATE_HEADER,

  // Copying the packet payload. The payload length is determined by the header.
  // Moves to CHPP_STATE_FOOTER afterwards.
  CHPP_STATE_PAYLOAD,

  // Processing the packet footer (checksum) and responding accordingly. Moves
  // to CHPP_STATE_PREAMBLE afterwards.
  CHPP_STATE_FOOTER
};

struct ChppRxStatus {
  // Current receiving state, as described in ChppRxState.
  enum ChppRxState state;

  // Location counter in bytes within each state. Must always be reinitialized
  // to 0 when switching states.
  size_t loc;
};

/************************************************
 *  Global (to this file) Variables
 ***********************************************/

static struct ChppRxStatus gRxStatus;         // Rx state and location within
static struct ChppTransportHeader gRxHeader;  // Rx packet header
static struct ChppTransportFooter gRxFooter;  // Rx packet footer (checksum)

/************************************************
 *  Prototypes
 ***********************************************/

static void chppUpdateRxState(enum ChppRxState newState);
static size_t chppConsumePreamble(const uint8_t *buf, size_t len);
static size_t chppConsumeHeader(const uint8_t *buf, size_t len);
static size_t chppConsumePayload(const uint8_t *buf, size_t len);
static size_t chppConsumeFooter(const uint8_t *buf, size_t len);

/************************************************
 *  Tx
 ***********************************************/
/*
static void chppTxStartReTxTimer(void) {
  // starts / restarts timeout timer for retx of a packet
  // TODO
}

static void chppTxStopReTxTimer(void) {
  // TODO
}
*/
/************************************************
 *  Rx
 ***********************************************/

/**
 * Called any time the Rx state needs to be changed. Ensures that the location
 * counter among that state (gRxStatus.loc) is also reset at the same time.
 *
 * @param newState Next Rx state
 */
static void chppUpdateRxState(enum ChppRxState newState) {
  LOGD("Changing state from %d to %d", gRxStatus.state, newState);
  gRxStatus.loc = 0;
  gRxStatus.state = newState;
}

/**
 * Called by chppRxData to find a preamble (i.e. packet start delimiter) in the
 * incoming data stream.
 * Moves the state to CHPP_STATE_HEADER as soon as it has seen a complete
 * preamble.
 * Any future backwards-incompatible versions of CHPP Transport will use a
 * different preamble.
 *
 * @param buf Input data
 * @param len Length of input data in bytes
 *
 * @return Length of consumed data in bytes
 */
static size_t chppConsumePreamble(const uint8_t *buf, size_t len) {
  size_t consumed = 0;

  // TODO: Optimize loop, maybe using memchr() / memcmp() / SIMD, especially if
  // serial port calling chppRxData does not implement zero filter
  while (consumed < len && gRxStatus.loc < CHPP_PREAMBLE_LEN_BYTES) {
    if (buf[consumed] ==
        ((CHPP_PREAMBLE_DATA >> (CHPP_PREAMBLE_LEN_BYTES - gRxStatus.loc - 1)) &
         0xff)) {
      // Correct byte of preamble observed
      gRxStatus.loc++;
    } else if (buf[consumed] ==
               ((CHPP_PREAMBLE_DATA >> (CHPP_PREAMBLE_LEN_BYTES - 1)) & 0xff)) {
      // Previous search failed but first byte of another preamble observed
      gRxStatus.loc = 1;
    } else {
      // Continue search for a valid preamble from the start
      gRxStatus.loc = 0;
    }

    consumed++;
  }

  // Let's see why we exited the above loop
  if (gRxStatus.loc == CHPP_PREAMBLE_LEN_BYTES) {
    // Complete preamble observed, move on to next state
    chppUpdateRxState(CHPP_STATE_HEADER);
  }

  return consumed;
}

/**
 * Called by chppRxData to process the packet header from the incoming data
 * stream.
 * Moves the Rx state to CHPP_STATE_PAYLOAD afterwards.
 *
 * @param buf Input data
 * @param len Length of input data in bytes
 *
 * @return Length of consumed data in bytes
 */
static size_t chppConsumeHeader(const uint8_t *buf, size_t len) {
  CHPP_ASSERT(gRxStatus.loc < sizeof(struct ChppTransportHeader));
  size_t bytesToCopy =
      MIN(len, (sizeof(struct ChppTransportHeader) - gRxStatus.loc));

  LOGD("Copying %zu bytes of header", bytesToCopy);
  memcpy(((uint8_t *)&gRxHeader) + gRxStatus.loc, buf, bytesToCopy);

  gRxStatus.loc += bytesToCopy;
  if (gRxStatus.loc == sizeof(struct ChppTransportHeader)) {
    // Header copied. Move on
    chppUpdateRxState(CHPP_STATE_PAYLOAD);
  }

  return bytesToCopy;
}

/**
 * Called by chppRxData to copy the payload, the length of which is determined
 * by the header, from the incoming data stream.
 * Moves the Rx state to CHPP_STATE_FOOTER afterwards.
 *
 * @param buf Input data
 * @param len Length of input data in bytes
 *
 * @return Length of consumed data in bytes
 */
static size_t chppConsumePayload(const uint8_t *buf, size_t len) {
  CHPP_ASSERT(gRxStatus.loc < gRxHeader.length);
  size_t bytesToCopy = MIN(len, (gRxHeader.length - gRxStatus.loc));

  LOGD("Copying %zu bytes of payload", bytesToCopy);

  // TODO: deal with the payload, making sure to prevent buffer overflows
  UNUSED_VAR(buf);

  gRxStatus.loc += bytesToCopy;
  if (gRxStatus.loc == gRxHeader.length) {
    // Payload copied. Move on
    chppUpdateRxState(CHPP_STATE_FOOTER);
  }

  return bytesToCopy;
}

/**
 * Called by chppRxData to process the packet footer from the incoming data
 * stream. Checks checksum, triggering the correct response (ACK / NACK).
 * Moves the Rx state to CHPP_STATE_PREAMBLE afterwards.
 *
 * @param buf Input data
 * @param len Length of input data in bytes
 *
 * @return Length of consumed data in bytes
 */
static size_t chppConsumeFooter(const uint8_t *buf, size_t len) {
  CHPP_ASSERT(gRxStatus.loc < sizeof(struct ChppTransportFooter));
  size_t bytesToCopy =
      MIN(len, (sizeof(struct ChppTransportFooter) - gRxStatus.loc));

  LOGD("Copying %zu bytes of footer (checksum)", bytesToCopy);
  memcpy(((uint8_t *)&gRxFooter) + gRxStatus.loc, buf, bytesToCopy);

  gRxStatus.loc += bytesToCopy;
  if (gRxStatus.loc == sizeof(struct ChppTransportFooter)) {
    // Footer copied. Move on

    // TODO: Check checksum, ACK / NACK accordingly, and send off packet

    chppUpdateRxState(CHPP_STATE_PREAMBLE);
  }

  return bytesToCopy;
}

// Public function
bool chppRxData(const uint8_t *buf, size_t len) {
  LOGD("chppRxData received %zu bytes (state = %d)", len, gRxStatus.state);
  CHPP_NOT_NULL(buf);

  size_t consumed = 0;
  while (consumed < len) {
    switch (gRxStatus.state) {
      case CHPP_STATE_PREAMBLE:
        consumed += chppConsumePreamble(&buf[consumed], len - consumed);
        break;

      case CHPP_STATE_HEADER:
        consumed += chppConsumeHeader(&buf[consumed], len - consumed);
        break;

      case CHPP_STATE_PAYLOAD:
        consumed += chppConsumePayload(&buf[consumed], len - consumed);
        break;

      case CHPP_STATE_FOOTER:
        consumed += chppConsumeFooter(&buf[consumed], len - consumed);
        break;

      default:
        LOGE("Invalid state %d", gRxStatus.state);
        chppUpdateRxState(CHPP_STATE_PREAMBLE);
    }

    LOGD("chppRxData consumed %zu of %zu bytes (state = %d)", consumed, len,
         gRxStatus.state);
  }

  return (gRxStatus.state == CHPP_STATE_PREAMBLE && gRxStatus.loc == 0);
}
