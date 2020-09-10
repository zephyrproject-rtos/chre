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

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "chpp/app.h"
#include "chpp/clients/discovery.h"
#include "chpp/link.h"
#include "chpp/macros.h"
#include "chpp/memory.h"
#include "chpp/platform/log.h"

//! Signals to use in ChppNotifier in this program.
#define CHPP_SIGNAL_EXIT UINT32_C(1 << 0)
#define CHPP_SIGNAL_TRANSPORT_EVENT UINT32_C(1 << 1)

/************************************************
 *  Prototypes
 ***********************************************/

static void chppSetRxState(struct ChppTransportState *context,
                           enum ChppRxState newState);
static size_t chppConsumePreamble(struct ChppTransportState *context,
                                  const uint8_t *buf, size_t len);
static size_t chppConsumeHeader(struct ChppTransportState *context,
                                const uint8_t *buf, size_t len);
static size_t chppConsumePayload(struct ChppTransportState *context,
                                 const uint8_t *buf, size_t len);
static size_t chppConsumeFooter(struct ChppTransportState *context,
                                const uint8_t *buf, size_t len);
static void chppRxAbortPacket(struct ChppTransportState *context);
static void chppProcessReset(struct ChppTransportState *context);
static void chppProcessResetAck(struct ChppTransportState *context);
static void chppProcessRxPayload(struct ChppTransportState *context);
static void chppClearRxDatagram(struct ChppTransportState *context);
static bool chppRxChecksumIsOk(const struct ChppTransportState *context);
static enum ChppTransportErrorCode chppRxHeaderCheck(
    const struct ChppTransportState *context);
static void chppRegisterRxAck(struct ChppTransportState *context);

static void chppEnqueueTxPacket(struct ChppTransportState *context,
                                uint8_t packetCode);
static size_t chppAddPreamble(uint8_t *buf);
uint32_t chppCalculateChecksum(uint8_t *buf, size_t len);
bool chppDequeueTxDatagram(struct ChppTransportState *context);
static void chppTransportDoWork(struct ChppTransportState *context);
static void chppAppendToPendingTxPacket(struct PendingTxPacket *packet,
                                        const uint8_t *buf, size_t len);
static bool chppEnqueueTxDatagram(struct ChppTransportState *context,
                                  int8_t packetCode, void *buf, size_t len);

static void chppResetTransportContext(struct ChppTransportState *context);
static void chppReset(struct ChppTransportState *transportContext,
                      struct ChppAppState *appContext);
static void chppTransportSendReset(
    struct ChppTransportState *context,
    enum ChppTransportPacketAttributes resetType);

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Called any time the Rx state needs to be changed. Ensures that the location
 * counter among that state (rxStatus.locInState) is also reset at the same
 * time.
 *
 * @param context Maintains status for each transport layer instance.
 * @param newState Next Rx state.
 */
static void chppSetRxState(struct ChppTransportState *context,
                           enum ChppRxState newState) {
  CHPP_LOGD("Changing RX transport state from %" PRIu8 " to %" PRIu8
            " after %" PRIuSIZE " bytes",
            context->rxStatus.state, newState, context->rxStatus.locInState);
  context->rxStatus.locInState = 0;
  context->rxStatus.state = newState;
}

/**
 * Called by chppRxDataCb to find a preamble (i.e. packet start delimiter) in
 * the incoming data stream.
 * Moves the state to CHPP_STATE_HEADER as soon as it has seen a complete
 * preamble.
 * Any future backwards-incompatible versions of CHPP Transport will use a
 * different preamble.
 *
 * @param context Maintains status for each transport layer instance.
 * @param buf Input data.
 * @param len Length of input data in bytes.
 *
 * @return Length of consumed data in bytes.
 */
static size_t chppConsumePreamble(struct ChppTransportState *context,
                                  const uint8_t *buf, size_t len) {
  size_t consumed = 0;

  // TODO: Optimize loop, maybe using memchr() / memcmp() / SIMD, especially if
  // serial port calling chppRxDataCb does not implement zero filter
  while (consumed < len &&
         context->rxStatus.locInState < CHPP_PREAMBLE_LEN_BYTES) {
    size_t offset = context->rxStatus.locInState;
    if ((offset == 0 && buf[consumed] == CHPP_PREAMBLE_BYTE_FIRST) ||
        (offset == 1 && buf[consumed] == CHPP_PREAMBLE_BYTE_SECOND)) {
      // Correct byte of preamble observed
      context->rxStatus.locInState++;

    } else if (buf[consumed] == CHPP_PREAMBLE_BYTE_FIRST) {
      // Previous search failed but first byte of another preamble observed
      context->rxStatus.locInState = 1;

    } else {
      // Continue search for a valid preamble from the start
      context->rxStatus.locInState = 0;
    }

    consumed++;
  }

  // Let's see why we exited the above loop
  if (context->rxStatus.locInState == CHPP_PREAMBLE_LEN_BYTES) {
    // Complete preamble observed, move on to next state
    chppSetRxState(context, CHPP_STATE_HEADER);
  }

  return consumed;
}

/**
 * Called by chppRxDataCb to process the packet header from the incoming data
 * stream.
 * Moves the Rx state to CHPP_STATE_PAYLOAD afterwards.
 *
 * @param context Maintains status for each transport layer instance.
 * @param buf Input data.
 * @param len Length of input data in bytes.
 *
 * @return Length of consumed data in bytes.
 */
static size_t chppConsumeHeader(struct ChppTransportState *context,
                                const uint8_t *buf, size_t len) {
  CHPP_ASSERT(context->rxStatus.locInState <
              sizeof(struct ChppTransportHeader));
  size_t bytesToCopy = MIN(
      len, (sizeof(struct ChppTransportHeader) - context->rxStatus.locInState));
  memcpy(((uint8_t *)&context->rxHeader) + context->rxStatus.locInState, buf,
         bytesToCopy);

  context->rxStatus.locInState += bytesToCopy;
  if (context->rxStatus.locInState == sizeof(struct ChppTransportHeader)) {
    // Header fully copied. Move on

    enum ChppTransportErrorCode headerCheckResult = chppRxHeaderCheck(context);
    if (headerCheckResult != CHPP_TRANSPORT_ERROR_NONE) {
      // Header fails consistency check. NACK and return to preamble state
      chppEnqueueTxPacket(context, headerCheckResult);
      chppSetRxState(context, CHPP_STATE_PREAMBLE);

    } else if (context->rxHeader.length == 0) {
      // Non-payload packet
      chppSetRxState(context, CHPP_STATE_FOOTER);

    } else {
      // Payload bearing packet
      uint8_t *tempPayload;

      if (context->rxDatagram.length == 0) {
        // Packet is a new datagram
        tempPayload = chppMalloc(context->rxHeader.length);
      } else {
        // Packet is a continuation of a fragmented datagram
        tempPayload =
            chppRealloc(context->rxDatagram.payload,
                        context->rxDatagram.length + context->rxHeader.length,
                        context->rxDatagram.length);
      }

      if (tempPayload == NULL) {
        CHPP_LOG_OOM("packet# %" PRIu8 ", len=%" PRIu16
                     ". Previous fragment(s) total len=%" PRIuSIZE,
                     context->rxHeader.seq, context->rxHeader.length,
                     context->rxDatagram.length);
        chppEnqueueTxPacket(context, CHPP_TRANSPORT_ERROR_OOM);
        chppSetRxState(context, CHPP_STATE_PREAMBLE);
      } else {
        context->rxDatagram.payload = tempPayload;
        context->rxDatagram.length += context->rxHeader.length;
        chppSetRxState(context, CHPP_STATE_PAYLOAD);
      }
    }
  }

  return bytesToCopy;
}

/**
 * Called by chppRxDataCb to copy the payload, the length of which is determined
 * by the header, from the incoming data stream.
 * Moves the Rx state to CHPP_STATE_FOOTER afterwards.
 *
 * @param context Maintains status for each transport layer instance.
 * @param buf Input data
 * @param len Length of input data in bytes
 *
 * @return Length of consumed data in bytes
 */
static size_t chppConsumePayload(struct ChppTransportState *context,
                                 const uint8_t *buf, size_t len) {
  CHPP_ASSERT(context->rxStatus.locInState < context->rxHeader.length);
  size_t bytesToCopy =
      MIN(len, (context->rxHeader.length - context->rxStatus.locInState));
  memcpy(context->rxDatagram.payload + context->rxStatus.locInDatagram, buf,
         bytesToCopy);
  context->rxStatus.locInDatagram += bytesToCopy;

  context->rxStatus.locInState += bytesToCopy;
  if (context->rxStatus.locInState == context->rxHeader.length) {
    // Entire packet payload copied. Move on
    chppSetRxState(context, CHPP_STATE_FOOTER);
  }

  return bytesToCopy;
}

/**
 * Called by chppRxDataCb to process the packet footer from the incoming data
 * stream. Checks checksum, triggering the correct response (ACK / NACK).
 * Moves the Rx state to CHPP_STATE_PREAMBLE afterwards.
 *
 * @param context Maintains status for each transport layer instance.
 * @param buf Input data.
 * @param len Length of input data in bytes.
 *
 * @return Length of consumed data in bytes.
 */
static size_t chppConsumeFooter(struct ChppTransportState *context,
                                const uint8_t *buf, size_t len) {
  CHPP_ASSERT(context->rxStatus.locInState <
              sizeof(struct ChppTransportFooter));
  size_t bytesToCopy = MIN(
      len, (sizeof(struct ChppTransportFooter) - context->rxStatus.locInState));
  memcpy(((uint8_t *)&context->rxFooter) + context->rxStatus.locInState, buf,
         bytesToCopy);

  context->rxStatus.locInState += bytesToCopy;
  if (context->rxStatus.locInState == sizeof(struct ChppTransportFooter)) {
    // Footer copied. Move on

    // TODO: Handle duplicate packets (i.e. resent because ACK was lost)

    if (!chppRxChecksumIsOk(context)) {
      CHPP_LOGE("Discarding RX packet because of bad checksum. seq=%" PRIu8
                " len=%" PRIu16,
                context->rxHeader.seq, context->rxHeader.length);
      chppRxAbortPacket(context);
      chppEnqueueTxPacket(context, CHPP_TRANSPORT_ERROR_CHECKSUM);  // NACK

    } else if (CHPP_TRANSPORT_GET_ATTR(context->rxHeader.packetCode) ==
               CHPP_TRANSPORT_ATTR_RESET) {
      CHPP_LOGD("RX RESET packet. seq=%" PRIu8, context->rxHeader.seq);

      chppProcessReset(context);

      chppMutexUnlock(&context->mutex);
      chppTransportSendReset(context, CHPP_TRANSPORT_ATTR_RESET_ACK);
      chppInitiateDiscovery(context->appContext);
      chppMutexLock(&context->mutex);

    } else if (CHPP_TRANSPORT_GET_ATTR(context->rxHeader.packetCode) ==
               CHPP_TRANSPORT_ATTR_RESET_ACK) {
      CHPP_LOGD("RX RESET-ACK packet. seq=%" PRIu8, context->rxHeader.seq);

      chppProcessResetAck(context);

      chppMutexUnlock(&context->mutex);
      chppInitiateDiscovery(context->appContext);
      chppMutexLock(&context->mutex);

    } else if (context->resetState == CHPP_RESET_STATE_RESETTING) {
      CHPP_LOGE("Discarding RX packet because CHPP is resetting. seq=%" PRIu8
                ", len=%" PRIu16,
                context->rxHeader.seq, context->rxHeader.length);

    } else {
      CHPP_LOGD("RX good packet. payload len=%" PRIu8 ", seq=%" PRIu8
                ", ackSeq=%" PRIu8 ", flags=0x%" PRIx8 ", packetCode=0x%" PRIx8,
                context->rxHeader.length, context->rxHeader.seq,
                context->rxHeader.ackSeq, context->rxHeader.flags,
                context->rxHeader.packetCode);

      context->rxStatus.receivedPacketCode = context->rxHeader.packetCode;
      chppRegisterRxAck(context);

      if (context->txDatagramQueue.pending > 0) {
        // There are packets to send out (could be new or retx)
        chppEnqueueTxPacket(context, CHPP_TRANSPORT_ERROR_NONE);
      }

      if (context->rxHeader.length > 0) {
        // Process payload and send ACK
        chppProcessRxPayload(context);
      }
    }

    // Done with this packet. Wait for next packet
    chppSetRxState(context, CHPP_STATE_PREAMBLE);
  }

  return bytesToCopy;
}

/**
 * Discards of an incomplete Rx packet during receive (e.g. due to a timeout or
 * bad checksum).
 *
 * @param context Maintains status for each transport layer instance.
 */
static void chppRxAbortPacket(struct ChppTransportState *context) {
  if (context->rxHeader.length > 0) {
    // Packet has a payload we need to discard of

    context->rxDatagram.length -= context->rxHeader.length;
    context->rxStatus.locInDatagram -= context->rxHeader.length;

    if (context->rxDatagram.length == 0) {
      // Discarding this packet == discarding entire datagram

      CHPP_FREE_AND_NULLIFY(context->rxDatagram.payload);

    } else {
      // Discarding this packet == discarding part of datagram

      uint8_t *tempPayload =
          chppRealloc(context->rxDatagram.payload, context->rxDatagram.length,
                      context->rxDatagram.length + context->rxHeader.length);

      if (tempPayload == NULL) {
        CHPP_LOG_OOM("While discarding RX continuation packet. seq=%" PRIu8
                     ". datagram len=%" PRIuSIZE,
                     context->rxHeader.seq,
                     context->rxDatagram.length + context->rxHeader.length);
      } else {
        context->rxDatagram.payload = tempPayload;
      }
    }
  }
}

static void chppProcessReset(struct ChppTransportState *context) {
  // TODO: Configure transport layer based on (optional?) received config before
  // datagram is wiped

  chppReset(context, context->appContext);

  // context->rxHeader is not wiped in reset
  context->rxStatus.receivedPacketCode = context->rxHeader.packetCode;
  context->rxStatus.expectedSeq = context->rxHeader.seq + 1;
}

static void chppProcessResetAck(struct ChppTransportState *context) {
  context->resetState = CHPP_RESET_STATE_NONE;
  context->rxStatus.receivedPacketCode = context->rxHeader.packetCode;
  context->rxStatus.expectedSeq = context->rxHeader.seq + 1;
  chppRegisterRxAck(context);

  // TODO: Configure transport layer based on (optional?) received config

  chppAppProcessDoneCb(context, context->rxDatagram.payload);
  chppClearRxDatagram(context);
}

/**
 * Process the payload of a validated payload-bearing packet and send out the
 * ACK
 *
 * @param context Maintains status for each transport layer instance.
 */
static void chppProcessRxPayload(struct ChppTransportState *context) {
  context->rxStatus.expectedSeq = context->rxHeader.seq + 1;

  if (context->rxHeader.flags & CHPP_TRANSPORT_FLAG_UNFINISHED_DATAGRAM) {
    // Packet is part of a larger datagram
    CHPP_LOGD("RX packet for unfinished datagram. Seq=%" PRIu8 " len=%" PRIu16
              ". Datagram len so far is %" PRIuSIZE,
              context->rxHeader.seq, context->rxHeader.length,
              context->rxDatagram.length);

  } else {
    // End of this packet is end of a datagram

    // Send the payload to the App Layer
    // Note that it is up to the app layer to free the buffer using
    // chppAppProcessDoneCb() after is is done.
    chppMutexUnlock(&context->mutex);
    chppProcessRxDatagram(context->appContext, context->rxDatagram.payload,
                          context->rxDatagram.length);
    chppMutexLock(&context->mutex);

    chppClearRxDatagram(context);

    // Send ACK because we had RX a payload-bearing packet
    CHPP_LOGD("App layer processed datagram with len=%" PRIuSIZE
              ", ending packet seq="
              "%" PRIu8 ", len=%" PRIu16 ". Sending ACK=%" PRIu8
              " (previously sent=%" PRIu8 ")",
              context->rxDatagram.length, context->rxHeader.seq,
              context->rxHeader.length, context->rxStatus.expectedSeq,
              context->txStatus.sentAckSeq);
    chppEnqueueTxPacket(context, CHPP_TRANSPORT_ERROR_NONE);
  }
}

/**
 * Resets the incoming datagram state, i.e. after the datagram has been
 * processed.
 * Note that it is up to the app layer to inform the transport layer using
 * chppAppProcessDoneCb() once it is done with the buffer so it is freed.
 *
 * @param context Maintains status for each transport layer instance.
 */
static void chppClearRxDatagram(struct ChppTransportState *context) {
  context->rxStatus.locInDatagram = 0;
  context->rxDatagram.length = 0;
  context->rxDatagram.payload = NULL;
}

/**
 * Validates the checksum of an incoming packet.
 *
 * @param context Maintains status for each transport layer instance.
 *
 * @return True if and only if the checksum is correct.
 */
static bool chppRxChecksumIsOk(const struct ChppTransportState *context) {
  // TODO
  UNUSED_VAR(context);

  CHPP_LOGE("Unconditionally assuming checksum is correct");
  return true;
}

/**
 * Performs consistency checks on received packet header to determine if it is
 * obviously corrupt / invalid.
 *
 * @param context Maintains status for each transport layer instance.
 *
 * @return True if and only if header passes checks
 */
static enum ChppTransportErrorCode chppRxHeaderCheck(
    const struct ChppTransportState *context) {
  enum ChppTransportErrorCode result = CHPP_TRANSPORT_ERROR_NONE;

  bool invalidSeqNo = (context->rxHeader.seq != context->rxStatus.expectedSeq);
  bool hasPayload = (context->rxHeader.length > 0);
  if (invalidSeqNo && hasPayload) {
    // Note: For a future ACK window > 1, might make more sense to keep quiet
    // instead of flooding with out of order NACK errors
    result = CHPP_TRANSPORT_ERROR_ORDER;

    CHPP_LOGE("Invalid RX packet header. Unexpected seq=%" PRIu8
              " (expected=%" PRIu8 "), len=%" PRIu8,
              context->rxHeader.seq, context->rxStatus.expectedSeq,
              context->rxHeader.length);
  }

  // TODO: More checks

  return result;
}

/**
 * Registers a received ACK. If an outgoing datagram is fully ACKed, it is
 * popped from the Tx queue.
 *
 * @param context Maintains status for each transport layer instance.
 */
static void chppRegisterRxAck(struct ChppTransportState *context) {
  uint8_t rxAckSeq = context->rxHeader.ackSeq;

  if (context->rxStatus.receivedAckSeq != rxAckSeq) {
    // A previously sent packet was actually ACKed
    // Note: For a future ACK window >1, we should loop by # of ACKed packets
    if ((uint8_t)(context->rxStatus.receivedAckSeq + 1) != rxAckSeq) {
      CHPP_LOGE("Out of order ACK received (last registered=%" PRIu8
                ", received=%" PRIu8 ")",
                context->rxStatus.receivedAckSeq, rxAckSeq);
    } else {
      CHPP_LOGD(
          "ACK received (last registered=%" PRIu8 ", received=%" PRIu8
          "). Prior queue depth=%" PRIu8 ", front datagram=%" PRIu8
          " at loc=%" PRIuSIZE " of len=%" PRIuSIZE,
          context->rxStatus.receivedAckSeq, rxAckSeq,
          context->txDatagramQueue.pending, context->txDatagramQueue.front,
          context->txStatus.ackedLocInDatagram,
          context->txDatagramQueue.datagram[context->txDatagramQueue.front]
              .length);
    }

    context->rxStatus.receivedAckSeq = rxAckSeq;

    // Process and if necessary pop from Tx datagram queue
    context->txStatus.ackedLocInDatagram += CHPP_TRANSPORT_TX_MTU_BYTES;
    if (context->txStatus.ackedLocInDatagram >=
        context->txDatagramQueue.datagram[context->txDatagramQueue.front]
            .length) {
      // We are done with datagram

      context->txStatus.ackedLocInDatagram = 0;
      context->txStatus.sentLocInDatagram = 0;

      // Note: For a future ACK window >1, we should update which datagram too

      chppDequeueTxDatagram(context);
    }
  }  // else {nothing was ACKed}
}

/**
 * Enqueues an outgoing packet with the specified error code. The error code
 * refers to the optional reason behind a NACK, if any. An error code of
 * CHPP_TRANSPORT_ERROR_NONE indicates that no error was reported (i.e. either
 * an ACK or an implicit NACK)
 *
 * Note that the decision as to wheather to include a payload will be taken
 * later, i.e. before the packet is being sent out from the queue. A payload is
 * expected to be included if there is one or more pending Tx datagrams and we
 * are not waiting on a pending ACK. A (repeat) payload is also included if we
 * have received a NACK.
 *
 * Further note that even for systems with an ACK window greater than one, we
 * would only need to send an ACK for the last (correct) packet, hence we only
 * need a queue length of one here.
 *
 * @param context Maintains status for each transport layer instance.
 * @param packetCode Error code and packet attributes to be sent.
 */
static void chppEnqueueTxPacket(struct ChppTransportState *context,
                                uint8_t packetCode) {
  context->txStatus.hasPacketsToSend = true;
  context->txStatus.packetCodeToSend = packetCode;

  CHPP_LOGD("chppEnqueueTxPacket called with packet code=0x%" PRIx8 "(%serror)",
            packetCode,
            (CHPP_TRANSPORT_GET_ERROR(packetCode) == CHPP_TRANSPORT_ERROR_NONE)
                ? "no "
                : "");

  // Notifies the main CHPP Transport Layer to run chppTransportDoWork().
  chppNotifierSignal(&context->notifier, CHPP_TRANSPORT_SIGNAL_EVENT);
}

/**
 * Adds a CHPP preamble to the beginning of buf.
 *
 * @param buf The CHPP preamble will be added to buf.
 *
 * @return Size of the added preamble.
 */
static size_t chppAddPreamble(uint8_t *buf) {
  buf[0] = CHPP_PREAMBLE_BYTE_FIRST;
  buf[1] = CHPP_PREAMBLE_BYTE_SECOND;
  return CHPP_PREAMBLE_LEN_BYTES;
}

/**
 * Calculates the checksum on a buffer indicated by buf with length len.
 *
 * @param buf Pointer to buffer for the ckecksum to be calculated on.
 * @param len Length of the buffer.
 *
 * @return Calculated checksum.
 */
uint32_t chppCalculateChecksum(uint8_t *buf, size_t len) {
  // TODO

  UNUSED_VAR(buf);
  UNUSED_VAR(len);
  return 1;
}

/**
 * Dequeues the datagram at the front of the datagram tx queue, if any, and
 * frees the payload. Returns false if the queue is empty.
 *
 * @param context Maintains status for each transport layer instance.
 * @return True indicates success. False indicates failure, i.e. the queue was
 * empty.
 */
bool chppDequeueTxDatagram(struct ChppTransportState *context) {
  bool success = false;

  if (context->txDatagramQueue.pending == 0) {
    CHPP_LOGE("Can not dequeue datagram because queue is empty");

  } else {
    CHPP_LOGD("Dequeuing front datagram with index=%" PRIu8 ", len=%" PRIuSIZE
              ". Queue depth: %" PRIu8 "->%" PRIu8,
              context->txDatagramQueue.front,
              context->txDatagramQueue.datagram[context->txDatagramQueue.front]
                  .length,
              context->txDatagramQueue.pending,
              context->txDatagramQueue.pending - 1);

    CHPP_FREE_AND_NULLIFY(
        context->txDatagramQueue.datagram[context->txDatagramQueue.front]
            .payload);
    context->txDatagramQueue.datagram[context->txDatagramQueue.front].length =
        0;

    context->txDatagramQueue.pending--;
    context->txDatagramQueue.front++;
    context->txDatagramQueue.front %= CHPP_TX_DATAGRAM_QUEUE_LEN;

    // Note: For a future ACK window >1, we need to update the queue position of
    // the datagram being sent as well (relative to the front-of-queue). i.e.
    // context->txStatus.datagramBeingSent--;

    success = true;
  }

  return success;
}

/**
 * Sends out a pending outgoing packet based on a notification from
 * chppEnqueueTxPacket().
 *
 * A payload may or may not be included be according the following:
 * No payload: If Tx datagram queue is empty OR we are waiting on a pending ACK.
 * New payload: If there is one or more pending Tx datagrams and we are not
 * waiting on a pending ACK.
 * Repeat payload: If we haven't received an ACK yet for our previous payload,
 * i.e. we have registered an explicit or implicit NACK.
 *
 * @param context Maintains status for each transport layer instance.
 */
static void chppTransportDoWork(struct ChppTransportState *context) {
  // Note: For a future ACK window >1, there needs to be a loop outside the lock

  CHPP_LOGD(
      "chppTransportDoWork start. packets to send=%s, link=%s, pending "
      "datagrams=%" PRIu8 ", last RX ACK=%" PRIu8 " (last TX seq=%" PRIu8
      ", can%s add payload)",
      context->txStatus.hasPacketsToSend ? "true" : "false (can't run)",
      context->txStatus.linkBusy ? "busy (can't run)" : "not busy",
      context->txDatagramQueue.pending, context->rxStatus.receivedAckSeq,
      context->txStatus.sentSeq,
      ((uint8_t)(context->txStatus.sentSeq + 1) ==
       context->rxStatus.receivedAckSeq)
          ? ""
          : "'t");

  chppMutexLock(&context->mutex);

  if (context->txStatus.hasPacketsToSend && !context->txStatus.linkBusy) {
    // There are pending outgoing packets and the link isn't busy
    context->txStatus.linkBusy = true;

    context->pendingTxPacket.length = 0;
    memset(&context->pendingTxPacket.payload, 0, CHPP_LINK_TX_MTU_BYTES);

    // Add preamble
    context->pendingTxPacket.length +=
        chppAddPreamble(&context->pendingTxPacket.payload[0]);

    // Add header
    struct ChppTransportHeader *txHeader =
        (struct ChppTransportHeader *)&context->pendingTxPacket
            .payload[context->pendingTxPacket.length];
    context->pendingTxPacket.length += sizeof(*txHeader);

    txHeader->packetCode = context->txStatus.packetCodeToSend;
    context->txStatus.packetCodeToSend = CHPP_TRANSPORT_ERROR_NONE;

    txHeader->ackSeq = context->rxStatus.expectedSeq;
    context->txStatus.sentAckSeq = txHeader->ackSeq;

    // If applicable, add payload
    if ((context->txDatagramQueue.pending > 0) &&
        ((uint8_t)(context->txStatus.sentSeq + 1) ==
         context->rxStatus.receivedAckSeq)) {
      // Note: For a future ACK window >1, seq # check should be against the
      // window size.

      // Note: For a future ACK window >1, this is only valid for the
      // (context->rxStatus.receivedPacketCode != CHPP_TRANSPORT_ERROR_NONE)
      // case, i.e. we have registered an explicit or implicit NACK. Else,
      // txHeader->seq = ++(context->txStatus.sentSeq)
      txHeader->seq = context->rxStatus.receivedAckSeq;
      context->txStatus.sentSeq = txHeader->seq;

      size_t remainingBytes =
          context->txDatagramQueue.datagram[context->txDatagramQueue.front]
              .length -
          context->txStatus.sentLocInDatagram;

      if (remainingBytes > CHPP_TRANSPORT_TX_MTU_BYTES) {
        // Send an unfinished part of a datagram
        txHeader->flags = CHPP_TRANSPORT_FLAG_UNFINISHED_DATAGRAM;
        txHeader->length = CHPP_TRANSPORT_TX_MTU_BYTES;

      } else {
        // Send final (or only) part of a datagram
        txHeader->flags = CHPP_TRANSPORT_FLAG_FINISHED_DATAGRAM;
        txHeader->length = remainingBytes;
      }

      // Copy payload
      chppAppendToPendingTxPacket(
          &context->pendingTxPacket,
          context->txDatagramQueue.datagram[context->txDatagramQueue.front]
                  .payload +
              context->txStatus.sentLocInDatagram,
          txHeader->length);

      context->txStatus.sentLocInDatagram += txHeader->length;

    }  // else {no payload}

    // Populate checksum
    uint32_t checksum = chppCalculateChecksum(context->pendingTxPacket.payload,
                                              context->pendingTxPacket.length);
    chppAppendToPendingTxPacket(&context->pendingTxPacket, (uint8_t *)&checksum,
                                sizeof(checksum));

    // Note: For a future ACK window >1, this needs to be updated
    context->txStatus.hasPacketsToSend = false;

    // We are done with context. Unlock mutex ASAP.
    chppMutexUnlock(&context->mutex);

    // Send out the packet
    CHPP_LOGD("Handing over TX packet to link layer. (len=%" PRIuSIZE
              ", flags=0x%" PRIx8 ", packetCode=0x%" PRIx8 ", ackSeq=%" PRIu8
              ", seq=%" PRIu8 ", payload len=%" PRIu16 ", queue depth=%" PRIu8
              ")",
              context->pendingTxPacket.length, txHeader->flags,
              txHeader->packetCode, txHeader->ackSeq, txHeader->seq,
              txHeader->length, context->txDatagramQueue.pending);
    enum ChppLinkErrorCode error = chppPlatformLinkSend(
        &context->linkParams, context->pendingTxPacket.payload,
        context->pendingTxPacket.length);

    if (error != CHPP_LINK_ERROR_NONE_QUEUED) {
      // Platform implementation for platformLinkSend() is synchronous or an
      // error occurred. In either case, we should call chppLinkSendDoneCb()
      // here to release the contents of pendingTxPacket.
      chppLinkSendDoneCb(&context->linkParams, error);
    }  // else {Platform implementation for platformLinkSend() is asynchronous}

  } else {
    // Either there are no pending outgoing packets or we are blocked on the
    // link layer.
    CHPP_LOGW(
        "chppTransportDoWork could not run. packets to send=%s, link=%s"
        ", pending datagrams=%" PRIu8 ", RX state=%" PRIu8,
        context->txStatus.hasPacketsToSend ? "true" : "false (can't run)",
        context->txStatus.linkBusy ? "busy (can't run)" : "not busy",
        context->txDatagramQueue.pending, context->rxStatus.state);

    chppMutexUnlock(&context->mutex);
  }
}

/**
 * Appends data from a buffer of length len to a PendingTxPacket, updating its
 * length.
 *
 * @param packet The PendingTxBuffer to be appended to.
 * @param buf Input data to be copied from.
 * @param len Length of input data in bytes.
 */
static void chppAppendToPendingTxPacket(struct PendingTxPacket *packet,
                                        const uint8_t *buf, size_t len) {
  CHPP_ASSERT(packet->length + len <= sizeof(packet->payload));
  memcpy(&packet->payload[packet->length], buf, len);
  packet->length += len;
}

/**
 * Enqueues an outgoing datagram of a specified length. The payload must have
 * been allocated by the caller using chppMalloc.
 *
 * If enqueueing is successful, the payload will be freed by this function
 * once it has been sent out.
 * If enqueueing is unsuccessful, it is up to the caller to decide when or if
 * to free the payload and/or resend it later.
 *
 * @param context Maintains status for each transport layer instance.
 * @param packetCode Error code and packet attributes to be sent.
 * @param buf Datagram payload allocated through chppMalloc. Cannot be null.
 * @param len Datagram length in bytes.
 *
 * @return True informs the sender that the datagram was successfully enqueued.
 * False informs the sender that the queue was full.
 */
static bool chppEnqueueTxDatagram(struct ChppTransportState *context,
                                  int8_t packetCode, void *buf, size_t len) {
  bool success = false;

  if (len == 0) {
    CHPP_LOGE("chppEnqueueTxDatagram called with payload length of 0");
    CHPP_DEBUG_ASSERT(false);

  } else {
    uint8_t *handle = buf;

    if (len < sizeof(struct ChppAppHeader)) {
      CHPP_LOGD("Enqueueing TX datagram (packet code=0x%" PRIx8
                ", len=%" PRIuSIZE ") for handle=%" PRIu8
                ". Queue depth: %" PRIu8 "->%" PRIu8,
                packetCode, len, *handle, context->txDatagramQueue.pending,
                context->txDatagramQueue.pending + 1);
    } else {
      struct ChppAppHeader *header = buf;
      CHPP_LOGD("Enqueueing TX datagram (packet code=0x%" PRIx8
                ", len=%" PRIuSIZE ") for handle=%" PRIu8 ", type=0x%" PRIx8
                ", transaction ID=%" PRIu8 ", error=%" PRIu8
                ", command=0x%" PRIx16 ". Queue depth: %" PRIu8 "->%" PRIu8,
                packetCode, len, header->handle, header->type,
                header->transaction, header->error, header->command,
                context->txDatagramQueue.pending,
                context->txDatagramQueue.pending + 1);
    }

    chppMutexLock(&context->mutex);

    if (context->txDatagramQueue.pending >= CHPP_TX_DATAGRAM_QUEUE_LEN) {
      CHPP_LOGE("Queue full. Cannot enqueue TX datagram for handle=%" PRIu8,
                *handle);

    } else {
      uint16_t end =
          (context->txDatagramQueue.front + context->txDatagramQueue.pending) %
          CHPP_TX_DATAGRAM_QUEUE_LEN;
      context->txDatagramQueue.datagram[end].length = len;
      context->txDatagramQueue.datagram[end].payload = buf;
      context->txDatagramQueue.pending++;

      if (context->txDatagramQueue.pending == 1) {
        // Queue was empty prior. Need to kickstart transmission.
        chppEnqueueTxPacket(context, packetCode);
      }

      success = true;
    }

    chppMutexUnlock(&context->mutex);
  }

  return success;
}

/**
 * Resets the transport state, maintaining the link layer parameters.
 */
static void chppResetTransportContext(struct ChppTransportState *context) {
  memset(&context->rxStatus, 0, sizeof(struct ChppRxStatus));
  memset(&context->rxDatagram, 0, sizeof(struct ChppDatagram));

  memset(&context->txStatus, 0, sizeof(struct ChppTxStatus));
  memset(&context->txDatagramQueue, 0, sizeof(struct ChppTxDatagramQueue));

  context->txStatus.sentSeq =
      UINT8_MAX;  // So that the seq # of the first TX packet is 0
}

/**
 * Re-initializes the CHPP transport and app layer states, e.g. when receiving a
 * reset packet.
 *
 * If the link layer is busy, this function will reset the link as well.
 * This function retains and restores the platform-specific values of
 * transportContext.linkParams.
 *
 * @param transportContext Maintains status for each transport layer instance.
 * @param appContext The app layer status struct associated with this transport
 * layer instance.
 */
static void chppReset(struct ChppTransportState *transportContext,
                      struct ChppAppState *appContext) {
  transportContext->resetState = CHPP_RESET_STATE_RESETTING;

  // Deinitialize app layer (deregistering services and clients)
  chppAppDeinit(appContext);

  // Reset asynchronous link layer if busy
  if (transportContext->txStatus.linkBusy == true) {
    // TODO: Give time for link layer to finish before resorting to a reset

    chppPlatformLinkReset(&transportContext->linkParams);
  }

  // Free memory allocated for any ongoing rx datagrams
  if (transportContext->rxDatagram.length > 0) {
    transportContext->rxDatagram.length = 0;
    CHPP_FREE_AND_NULLIFY(transportContext->rxDatagram.payload);
  }

  // Free memory allocated for any ongoing tx datagrams
  for (size_t i = 0; i < CHPP_TX_DATAGRAM_QUEUE_LEN; i++) {
    if (transportContext->txDatagramQueue.datagram[i].length > 0) {
      CHPP_FREE_AND_NULLIFY(
          transportContext->txDatagramQueue.datagram[i].payload);
    }
  }

  // Reset Transport Layer
  chppResetTransportContext(transportContext);

  // Initialize app layer
  chppAppInitWithClientServiceSet(appContext, transportContext,
                                  appContext->clientServiceSet);
}

/**
 * Sends a reset or reset-ack packet over the link in order to reset the remote
 * side or inform the counterpart of a reset, respectively. The transport
 * layer's configuration is sent as the payload of the reset packet.
 *
 * This function is used immediately after initialization, for example upon boot
 * (to send a reset), or when a reset packet is received and acted upon (to send
 * a reset-ack).
 *
 * @param transportContext Maintains status for each transport layer instance.
 * @param resetType Distinguishes a reset from a reset-ack, as defined in the
 * ChppTransportPacketAttributes struct.
 */
static void chppTransportSendReset(
    struct ChppTransportState *context,
    enum ChppTransportPacketAttributes resetType) {
  // Make sure CHPP is in an initialized state
  if (context->txDatagramQueue.pending > 0 ||
      context->txDatagramQueue.front != 0) {
    CHPP_LOGE(
        "chppTransportSendReset called but CHPP not in initialized state.");
    CHPP_ASSERT(false);
  }

  struct ChppTransportConfiguration *config =
      chppMalloc(sizeof(struct ChppTransportConfiguration));

  // CHPP transport version
  config->version.major = 1;
  config->version.minor = 0;
  config->version.patch = 0;

  // Rx MTU size
  config->rxMtu = CHPP_PLATFORM_LINK_RX_MTU_BYTES;

  // Max Rx window size
  // Note: current implementation does not support a window size >1
  config->windowSize = 1;

  // Advertised transport layer (ACK) timeout
  config->timeoutInMs = CHPP_PLATFORM_TRANSPORT_TIMEOUT_MS;

  // Send out the reset datagram
  CHPP_LOGI("Sending out CHPP transport layer RESET%s",
            (resetType == CHPP_TRANSPORT_ATTR_RESET_ACK) ? "-ACK" : "");

  if (resetType == CHPP_TRANSPORT_ATTR_RESET_ACK) {
    context->resetState = CHPP_RESET_STATE_NONE;
  }

  chppEnqueueTxDatagram(context, resetType, config,
                        sizeof(struct ChppTransportConfiguration));
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppTransportInit(struct ChppTransportState *transportContext,
                       struct ChppAppState *appContext) {
  CHPP_NOT_NULL(transportContext);
  CHPP_NOT_NULL(appContext);

  CHPP_LOGI("Initializing the CHPP transport layer");

  chppResetTransportContext(transportContext);
  chppMutexInit(&transportContext->mutex);
  chppNotifierInit(&transportContext->notifier);
  transportContext->appContext = appContext;
  chppPlatformLinkInit(&transportContext->linkParams);
}

void chppTransportDeinit(struct ChppTransportState *transportContext) {
  CHPP_NOT_NULL(transportContext);

  CHPP_LOGI("Deinitializing the CHPP transport layer");

  chppPlatformLinkDeinit(&transportContext->linkParams);
  chppNotifierDeinit(&transportContext->notifier);
  chppMutexDeinit(&transportContext->mutex);

  // TODO: Do other cleanup
}

bool chppRxDataCb(struct ChppTransportState *context, const uint8_t *buf,
                  size_t len) {
  CHPP_NOT_NULL(buf);
  CHPP_NOT_NULL(context);

  CHPP_LOGD("chppRxDataCb received %" PRIuSIZE
            " bytes while in RX state=%" PRIu8,
            len, context->rxStatus.state);

  size_t consumed = 0;
  while (consumed < len) {
    chppMutexLock(&context->mutex);
    // TODO: Investigate fine-grained locking, e.g. separating variables that
    // are only relevant to a particular path.
    // Also consider removing some of the finer-grained locks altogether for
    // non-multithreaded environments with clear documentation.

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
        CHPP_LOGE("Invalid RX state %" PRIu8, context->rxStatus.state);
        CHPP_DEBUG_ASSERT(false);
        chppSetRxState(context, CHPP_STATE_PREAMBLE);
    }

    chppMutexUnlock(&context->mutex);
  }

  return (context->rxStatus.state == CHPP_STATE_PREAMBLE &&
          context->rxStatus.locInState == 0);
}

void chppTxTimeoutTimerCb(struct ChppTransportState *context) {
  chppMutexLock(&context->mutex);

  // Implicit NACK. Set received error code accordingly
  context->rxStatus.receivedPacketCode = CHPP_TRANSPORT_ERROR_TIMEOUT;

  // Enqueue Tx packet which will be a retransmission based on the above
  chppEnqueueTxPacket(context, CHPP_TRANSPORT_ERROR_NONE);

  chppMutexUnlock(&context->mutex);
}

void chppRxTimeoutTimerCb(struct ChppTransportState *context) {
  CHPP_LOGE("Rx timeout during state %" PRIu8 ". Aborting packet# %" PRIu8
            " len=%" PRIu16,
            context->rxStatus.state, context->rxHeader.seq,
            context->rxHeader.length);

  chppMutexLock(&context->mutex);

  chppRxAbortPacket(context);
  chppSetRxState(context, CHPP_STATE_PREAMBLE);

  chppMutexUnlock(&context->mutex);
}

bool chppEnqueueTxDatagramOrFail(struct ChppTransportState *context, void *buf,
                                 size_t len) {
  bool success = false;
  bool resetting = (context->resetState == CHPP_RESET_STATE_RESETTING);

  if (len == 0) {
    CHPP_LOGE("chppEnqueueTxDatagramOrFail called with data length of 0");
    CHPP_DEBUG_ASSERT(false);

  } else if (resetting || !chppEnqueueTxDatagram(
                              context, CHPP_TRANSPORT_ERROR_NONE, buf, len)) {
    uint8_t *handle = buf;
    CHPP_LOGE("%s. Discarding TX datagram of %" PRIuSIZE
              " bytes for handle=%" PRIu8,
              resetting ? "CHPP resetting" : "TX queue full", len, *handle);
    CHPP_FREE_AND_NULLIFY(buf);

  } else {
    success = true;
  }

  return success;
}

void chppEnqueueTxErrorDatagram(struct ChppTransportState *context,
                                enum ChppTransportErrorCode errorCode) {
  switch (errorCode) {
    case CHPP_TRANSPORT_ERROR_OOM: {
      CHPP_LOGD(
          "App layer is enqueueing a CHPP_TRANSPORT_ERROR_OOM error datagram");
      break;
    }
    case CHPP_TRANSPORT_ERROR_APPLAYER: {
      CHPP_LOGD(
          "App layer is enqueueing a CHPP_TRANSPORT_ERROR_APPLAYER error "
          "datagram");
      break;
    }
    default: {
      // App layer should not invoke any other errors
      CHPP_LOGE("App layer is enqueueing an invalid error (%" PRIu8
                ") datagram",
                errorCode);
      CHPP_DEBUG_ASSERT(false);
    }
  }
  chppEnqueueTxPacket(context, CHPP_TRANSPORT_GET_ERROR(errorCode));
}

void chppWorkThreadStart(struct ChppTransportState *context) {
  chppTransportSendReset(context, CHPP_TRANSPORT_ATTR_RESET);
  CHPP_LOGI("CHPP Work Thread started");

  while (true) {
    uint32_t signal = chppNotifierWait(&context->notifier);

    if (signal & CHPP_TRANSPORT_SIGNAL_EXIT) {
      CHPP_LOGI("CHPP Work Thread terminated");
      break;
    }

    if (signal & CHPP_TRANSPORT_SIGNAL_EVENT) {
      chppTransportDoWork(context);
    }
    if (signal & CHPP_TRANSPORT_SIGNAL_PLATFORM_MASK) {
      chppPlatformLinkDoWork(&context->linkParams,
                             signal & CHPP_TRANSPORT_SIGNAL_PLATFORM_MASK);
    }
  }
}

void chppWorkThreadStop(struct ChppTransportState *context) {
  chppNotifierSignal(&context->notifier, CHPP_TRANSPORT_SIGNAL_EXIT);
}

void chppLinkSendDoneCb(struct ChppPlatformLinkParameters *params,
                        enum ChppLinkErrorCode error) {
  if (error != CHPP_LINK_ERROR_NONE_SENT) {
    CHPP_LOGE(
        "chppLinkSendDoneCb() indicates async send failure with error %" PRIu8,
        error);
  }

  struct ChppTransportState *context =
      container_of(params, struct ChppTransportState, linkParams);

  chppMutexLock(&context->mutex);
  context->txStatus.linkBusy = false;
  if (context->txStatus.hasPacketsToSend) {
    chppNotifierSignal(&context->notifier, CHPP_TRANSPORT_SIGNAL_EVENT);
  }
  chppMutexUnlock(&context->mutex);
}

void chppAppProcessDoneCb(struct ChppTransportState *context, uint8_t *buf) {
  UNUSED_VAR(context);

  CHPP_FREE_AND_NULLIFY(buf);
}
