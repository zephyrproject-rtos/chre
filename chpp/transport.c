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

#include "chpp/transport.h"

/************************************************
 *  Private Constants
 ***********************************************/

/************************************************
 *  Private Type Definitions
 ***********************************************/


/************************************************
 *  Prototypes
 ***********************************************/

static void chppUpdateRxState(struct ChppTransportState *context,
                              enum ChppRxState newState);
static size_t chppConsumePreamble(struct ChppTransportState *context,
                                  const uint8_t *buf, size_t len);
static size_t chppConsumeHeader(struct ChppTransportState *context,
                                const uint8_t *buf, size_t len);
static size_t chppConsumePayload(struct ChppTransportState *context,
                                 const uint8_t *buf, size_t len);
static size_t chppConsumeFooter(struct ChppTransportState *context,
                                const uint8_t *buf, size_t len);
static bool chppValidateChecksum(struct ChppTransportState *context);

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
 * counter among that state (rxStatus.loc) is also reset at the same time.
 *
 * @param context Is used to maintain status. Must be provided, zero
 * initialized, for each transport layer instance. Cannot be null.
 * @param newState Next Rx state
 */
static void chppUpdateRxState(struct ChppTransportState *context,
                              enum ChppRxState newState) {
  LOGD("Changing state from %d to %d", context->rxStatus.state, newState);
  context->rxStatus.loc = 0;
  context->rxStatus.state = newState;
}

/**
 * Called by chppRxData to find a preamble (i.e. packet start delimiter) in the
 * incoming data stream.
 * Moves the state to CHPP_STATE_HEADER as soon as it has seen a complete
 * preamble.
 * Any future backwards-incompatible versions of CHPP Transport will use a
 * different preamble.
 *
 * @param context Is used to maintain status. Must be provided, zero
 * initialized, for each transport layer instance. Cannot be null.
 * @param buf Input data
 * @param len Length of input data in bytes
 *
 * @return Length of consumed data in bytes
 */
static size_t chppConsumePreamble(struct ChppTransportState *context,
                                  const uint8_t *buf, size_t len) {
  size_t consumed = 0;

  // TODO: Optimize loop, maybe using memchr() / memcmp() / SIMD, especially if
  // serial port calling chppRxData does not implement zero filter
  while (consumed < len && context->rxStatus.loc < CHPP_PREAMBLE_LEN_BYTES) {
    if (buf[consumed] == ((CHPP_PREAMBLE_DATA >> (CHPP_PREAMBLE_LEN_BYTES -
                                                  context->rxStatus.loc - 1)) &
                          0xff)) {
      // Correct byte of preamble observed
      context->rxStatus.loc++;
    } else if (buf[consumed] ==
               ((CHPP_PREAMBLE_DATA >> (CHPP_PREAMBLE_LEN_BYTES - 1)) & 0xff)) {
      // Previous search failed but first byte of another preamble observed
      context->rxStatus.loc = 1;
    } else {
      // Continue search for a valid preamble from the start
      context->rxStatus.loc = 0;
    }

    consumed++;
  }

  // Let's see why we exited the above loop
  if (context->rxStatus.loc == CHPP_PREAMBLE_LEN_BYTES) {
    // Complete preamble observed, move on to next state
    chppUpdateRxState(context, CHPP_STATE_HEADER);
  }

  return consumed;
}

/**
 * Called by chppRxData to process the packet header from the incoming data
 * stream.
 * Moves the Rx state to CHPP_STATE_PAYLOAD afterwards.
 *
 * @param context Is used to maintain status. Must be provided, zero
 * initialized, for each transport layer instance. Cannot be null.
 * @param buf Input data
 * @param len Length of input data in bytes
 *
 * @return Length of consumed data in bytes
 */
static size_t chppConsumeHeader(struct ChppTransportState *context,
                                const uint8_t *buf, size_t len) {
  CHPP_ASSERT(context->rxStatus.loc < sizeof(struct ChppTransportHeader));
  size_t bytesToCopy =
      MIN(len, (sizeof(struct ChppTransportHeader) - context->rxStatus.loc));

  LOGD("Copying %zu bytes of header", bytesToCopy);
  memcpy(((uint8_t *)&context->rxHeader) + context->rxStatus.loc, buf,
         bytesToCopy);

  context->rxStatus.loc += bytesToCopy;
  if (context->rxStatus.loc == sizeof(struct ChppTransportHeader)) {
    // Header copied. Move on
    // TODO: Sanity check header

    if (context->rxHeader.length > 0) {
      // Payload bearing packet
      uint8_t *tempPayload;

      if (context->rxDatagram.length > 0) {
        // Packet is a continuation of a fragmented datagram
        tempPayload =
            chppRealloc(context->rxDatagram.payload,
                        context->rxDatagram.length + context->rxHeader.length,
                        context->rxDatagram.length);
      } else {
        // Packet is a new datagram
        tempPayload = chppMalloc(context->rxHeader.length);
      }

      if (tempPayload == NULL) {
        LOGE("OOM for packet %d, len=%u. Previous fragment(s) total len=%zu",
             context->rxHeader.seq, context->rxHeader.length,
             context->rxDatagram.length);

        // TODO: handle OOM
      } else {
        context->rxDatagram.payload = tempPayload;
        context->rxDatagram.length += context->rxHeader.length;
        chppUpdateRxState(context, CHPP_STATE_PAYLOAD);
      }
    }
  }

  return bytesToCopy;
}

/**
 * Called by chppRxData to copy the payload, the length of which is determined
 * by the header, from the incoming data stream.
 * Moves the Rx state to CHPP_STATE_FOOTER afterwards.
 *
 * @param context Is used to maintain status. Must be provided, zero
 * initialized, for each transport layer instance. Cannot be null.
 * @param buf Input data
 * @param len Length of input data in bytes
 *
 * @return Length of consumed data in bytes
 */
static size_t chppConsumePayload(struct ChppTransportState *context,
                                 const uint8_t *buf, size_t len) {
  CHPP_ASSERT(context->rxStatus.loc < context->rxHeader.length);
  size_t bytesToCopy =
      MIN(len, (context->rxHeader.length - context->rxStatus.loc));

  LOGD("Copying %zu bytes of payload", bytesToCopy);

  memcpy(&context->rxDatagram.payload + context->rxDatagram.loc, buf,
         bytesToCopy);
  context->rxDatagram.loc += bytesToCopy;

  context->rxStatus.loc += bytesToCopy;
  if (context->rxStatus.loc == context->rxHeader.length) {
    // Payload copied. Move on

    chppUpdateRxState(context, CHPP_STATE_FOOTER);
  }

  return bytesToCopy;
}

/**
 * Called by chppRxData to process the packet footer from the incoming data
 * stream. Checks checksum, triggering the correct response (ACK / NACK).
 * Moves the Rx state to CHPP_STATE_PREAMBLE afterwards.
 *
 * @param context Is used to maintain status. Must be provided, zero
 * initialized, for each transport layer instance. Cannot be null.
 * @param buf Input data
 * @param len Length of input data in bytes
 *
 * @return Length of consumed data in bytes
 */
static size_t chppConsumeFooter(struct ChppTransportState *context,
                                const uint8_t *buf, size_t len) {
  CHPP_ASSERT(context->rxStatus.loc < sizeof(struct ChppTransportFooter));
  size_t bytesToCopy =
      MIN(len, (sizeof(struct ChppTransportFooter) - context->rxStatus.loc));

  LOGD("Copying %zu bytes of footer (checksum)", bytesToCopy);
  memcpy(((uint8_t *)&context->rxFooter) + context->rxStatus.loc, buf,
         bytesToCopy);

  context->rxStatus.loc += bytesToCopy;
  if (context->rxStatus.loc == sizeof(struct ChppTransportFooter)) {
    // Footer copied. Move on

    // Check checksum, ACK / NACK / send off datagram accordingly
    if (!chppValidateChecksum(context)) {
      // Packet is bad. Roll back and enqueue NACK
      context->rxDatagram.length -= context->rxHeader.length;
      context->rxDatagram.loc -= context->rxHeader.length;

      uint8_t *tempPayload =
          chppRealloc(context->rxDatagram.payload, context->rxDatagram.length,
                      context->rxDatagram.length + context->rxHeader.length);
      if (tempPayload == NULL) {
        LOGE(
            "OOM reallocating to discard bad continuation packet %d len=%u. "
            "Previous fragment(s) total len=%zu",
            context->rxHeader.seq, context->rxHeader.length,
            context->rxDatagram.length);
      } else {
        context->rxDatagram.payload = tempPayload;
      }

      // TODO: enqueue NACK

    } else {
      // Packet is good, enqueue ACK
      // TODO: enqueue ACK

      if (!(context->rxHeader.flags &
            CHPP_TRANSPORT_FLAG_UNFINISHED_DATAGRAM)) {
        // End of this packet is end of a datagram

        // TODO: do something with the data

        context->rxDatagram.loc = 0;
        context->rxDatagram.length = 0;
        free(context->rxDatagram.payload);
      }  // else, packet is part of a larger datagram
    }

    chppUpdateRxState(context, CHPP_STATE_PREAMBLE);
  }

  return bytesToCopy;
}

// Public function
bool chppRxData(struct ChppTransportState *context, const uint8_t *buf,
                size_t len) {
  LOGD("chppRxData received %zu bytes (state = %d)", len,
       context->rxStatus.state);
  CHPP_NOT_NULL(buf);
  CHPP_NOT_NULL(context);

  size_t consumed = 0;
  while (consumed < len) {
    switch (context->rxStatus.state) {
      case CHPP_STATE_PREAMBLE:
        consumed +=
            chppConsumePreamble(context, &buf[consumed], len - consumed);
        break;

      case CHPP_STATE_HEADER:
        consumed += chppConsumeHeader(context, &buf[consumed], len - consumed);
        break;

      case CHPP_STATE_PAYLOAD:
        consumed += chppConsumePayload(context, &buf[consumed], len - consumed);
        break;

      case CHPP_STATE_FOOTER:
        consumed += chppConsumeFooter(context, &buf[consumed], len - consumed);
        break;

      default:
        LOGE("Invalid state %d", context->rxStatus.state);
        chppUpdateRxState(context, CHPP_STATE_PREAMBLE);
    }

    LOGD("chppRxData consumed %zu of %zu bytes (state = %d)", consumed, len,
         context->rxStatus.state);
  }

  return (context->rxStatus.state == CHPP_STATE_PREAMBLE &&
          context->rxStatus.loc == 0);
}

bool chppValidateChecksum(struct ChppTransportState *context) {
  // TODO
  UNUSED_VAR(context);

  LOGE("Blindly assuming checksum is correct");
  return true;
}
