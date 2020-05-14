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

#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>
#include <thread>

#include "chpp/app.h"
#include "chpp/services/discovery.h"
#include "chpp/transport.h"

#include "transport_test.h"

namespace {

// Preamble as separate bytes for testing
constexpr uint8_t kChppPreamble0 = 0x68;
constexpr uint8_t kChppPreamble1 = 0x43;

// Max size of payload sent to chppRxDataCb (bytes)
constexpr size_t kMaxChunkSize = 20000;

constexpr size_t kMaxPacketSize = kMaxChunkSize + CHPP_PREAMBLE_LEN_BYTES +
                                  sizeof(ChppTransportHeader) +
                                  sizeof(ChppTransportFooter);

// Input sizes to test the entire range of sizes with a few tests
constexpr int kChunkSizes[] = {0,  1,   2,   3,    4,     5,    6,
                               7,  8,   10,  16,   20,    30,   40,
                               51, 100, 201, 1000, 10001, 20000};

/*
 * Test suite for the CHPP Transport Layer
 */
class TransportTests : public testing::TestWithParam<int> {
 protected:
  void SetUp() override {
    chppTransportInit(&transportContext, &appContext);
    chppAppInit(&appContext, &transportContext);

    transportContext.linkParams.index = 1;
    transportContext.linkParams.sync = true;

    // Make sure CHPP has a correct count of the number of registered services
    // on this platform, (in this case, 1,) as registered in the function
    // chppRegisterCommonServices().
    ASSERT_EQ(appContext.registeredServiceCount, 1);
  }

  void TearDown() override {
    chppAppDeinit(&appContext);
    chppTransportDeinit(&transportContext);
  }

  /**
   * Wait for chppTransportDoWork() to finish after it is notified by
   * chppEnqueueTxPacket to run.
   *
   * TODO: Explore better ways to synchronize test with transport
   */
  void WaitForTransport(struct ChppTransportState *transportContext) {
    volatile uint16_t k = 1;
    while (transportContext->txStatus.hasPacketsToSend || k == 0) {
      k++;
    }
    ASSERT_FALSE(transportContext->txStatus.hasPacketsToSend);  // timeout
  }

  ChppTransportState transportContext = {};
  ChppAppState appContext = {};
  uint8_t buf[kMaxPacketSize] = {};
};

/**
 * A series of zeros shouldn't change state from CHPP_STATE_PREAMBLE
 */
TEST_P(TransportTests, ZeroNoPreambleInput) {
  size_t len = static_cast<size_t>(GetParam());
  if (len <= kMaxChunkSize) {
    EXPECT_TRUE(chppRxDataCb(&transportContext, buf, len));
    EXPECT_EQ(transportContext.rxStatus.state, CHPP_STATE_PREAMBLE);
  }
}

/**
 * A preamble after a series of zeros input should change state from
 * CHPP_STATE_PREAMBLE to CHPP_STATE_HEADER
 */
TEST_P(TransportTests, ZeroThenPreambleInput) {
  size_t len = static_cast<size_t>(GetParam());

  if (len <= kMaxChunkSize) {
    // Add preamble at the end of buf, as individual bytes instead of using
    // chppAddPreamble(&buf[preambleLoc])
    size_t preambleLoc = MAX(0, len - CHPP_PREAMBLE_LEN_BYTES);
    buf[preambleLoc] = kChppPreamble0;
    buf[preambleLoc + 1] = kChppPreamble1;

    if (len >= CHPP_PREAMBLE_LEN_BYTES) {
      EXPECT_FALSE(chppRxDataCb(&transportContext, buf, len));
      EXPECT_EQ(transportContext.rxStatus.state, CHPP_STATE_HEADER);
    } else {
      EXPECT_TRUE(chppRxDataCb(&transportContext, buf, len));
      EXPECT_EQ(transportContext.rxStatus.state, CHPP_STATE_PREAMBLE);
    }
  }
}

/**
 * Rx Testing with various length payloads of zeros
 */
TEST_P(TransportTests, RxPayloadOfZeros) {
  transportContext.rxStatus.state = CHPP_STATE_HEADER;
  size_t len = static_cast<size_t>(GetParam());
  std::thread t1(chppWorkThreadStart, &transportContext);

  if (len <= kMaxChunkSize) {
    ChppTransportHeader header{};
    header.flags = 0;
    header.errorCode = 0;
    header.ackSeq = 1;
    header.seq = 0;
    header.length = len;

    memcpy(buf, &header, sizeof(header));

    // Send header and check for correct state
    EXPECT_FALSE(
        chppRxDataCb(&transportContext, buf, sizeof(ChppTransportHeader)));
    if (len > 0) {
      EXPECT_EQ(transportContext.rxStatus.state, CHPP_STATE_PAYLOAD);
    } else {
      EXPECT_EQ(transportContext.rxStatus.state, CHPP_STATE_FOOTER);
    }

    // Correct decoding of packet length
    EXPECT_EQ(transportContext.rxHeader.length, len);
    EXPECT_EQ(transportContext.rxStatus.locInDatagram, 0);
    EXPECT_EQ(transportContext.rxDatagram.length, len);

    // Send payload if any and check for correct state
    if (len > 0) {
      EXPECT_FALSE(chppRxDataCb(&transportContext,
                                &buf[sizeof(ChppTransportHeader)], len));
      EXPECT_EQ(transportContext.rxStatus.state, CHPP_STATE_FOOTER);
    }

    // Should have complete packet payload by now
    EXPECT_EQ(transportContext.rxStatus.locInDatagram, len);

    // But no ACK yet
    EXPECT_FALSE(transportContext.txStatus.hasPacketsToSend);
    EXPECT_EQ(transportContext.txStatus.errorCodeToSend,
              CHPP_TRANSPORT_ERROR_NONE);
    EXPECT_EQ(transportContext.rxStatus.expectedSeq, header.seq);

    // Send footer
    EXPECT_TRUE(chppRxDataCb(&transportContext,
                             &buf[sizeof(ChppTransportHeader) + len],
                             sizeof(ChppTransportFooter)));

    // The next expected packet sequence # should incremented only if the
    // received packet is payload-bearing.
    uint8_t nextSeq = header.seq + ((len > 0) ? 1 : 0);
    EXPECT_EQ(transportContext.rxStatus.expectedSeq, nextSeq);

    // Check for correct ACK crafting if applicable (i.e. if the received packet
    // is payload-bearing).
    if (len > 0) {
      // TODO: Remove later as can cause flaky tests
      // These are expected to change shortly afterwards, as chppTransportDoWork
      // is run
      // EXPECT_TRUE(transportContext.txStatus.hasPacketsToSend);
      EXPECT_EQ(transportContext.txStatus.errorCodeToSend,
                CHPP_TRANSPORT_ERROR_NONE);
      EXPECT_EQ(transportContext.txDatagramQueue.pending, 0);

      WaitForTransport(&transportContext);

      // Check response packet fields
      struct ChppTransportHeader *txHeader =
          (struct ChppTransportHeader *)&transportContext.pendingTxPacket
              .payload[CHPP_PREAMBLE_LEN_BYTES];
      EXPECT_EQ(txHeader->flags, CHPP_TRANSPORT_FLAG_FINISHED_DATAGRAM);
      EXPECT_EQ(txHeader->errorCode, CHPP_TRANSPORT_ERROR_NONE);
      EXPECT_EQ(txHeader->ackSeq, nextSeq);
      EXPECT_EQ(txHeader->length, 0);

      // Check outgoing packet length
      EXPECT_EQ(transportContext.pendingTxPacket.length,
                CHPP_PREAMBLE_LEN_BYTES + sizeof(struct ChppTransportHeader) +
                    sizeof(struct ChppTransportFooter));
    }

    // Check for correct state
    EXPECT_EQ(transportContext.rxStatus.state, CHPP_STATE_PREAMBLE);

    // Should have reset loc and length for next packet / datagram
    EXPECT_EQ(transportContext.rxStatus.locInDatagram, 0);
    EXPECT_EQ(transportContext.rxDatagram.length, 0);
  }

  chppWorkThreadStop(&transportContext);
  t1.join();
}

TEST_P(TransportTests, EnqueueDatagrams) {
  size_t len = static_cast<size_t>(GetParam());

  if (len <= CHPP_TX_DATAGRAM_QUEUE_LEN) {
    // Add (len) datagrams of various length to queue

    int fr = 0;

    for (int j = 0; j == CHPP_TX_DATAGRAM_QUEUE_LEN; j++) {
      for (size_t i = 1; i <= len; i++) {
        uint8_t *buf = (uint8_t *)chppMalloc(i + 100);
        EXPECT_TRUE(
            chppEnqueueTxDatagramOrFail(&transportContext, buf, i + 100));

        EXPECT_EQ(transportContext.txDatagramQueue.pending, i);
        EXPECT_EQ(transportContext.txDatagramQueue.front, fr);
        EXPECT_EQ(transportContext.txDatagramQueue
                      .datagram[(i - 1 + fr) % CHPP_TX_DATAGRAM_QUEUE_LEN]
                      .length,
                  i + 100);
      }

      if (transportContext.txDatagramQueue.pending ==
          CHPP_TX_DATAGRAM_QUEUE_LEN) {
        uint8_t *buf = (uint8_t *)chppMalloc(100);
        EXPECT_FALSE(chppEnqueueTxDatagramOrFail(&transportContext, buf, 100));
        chppFree(buf);
      }

      for (size_t i = len; i > 0; i--) {
        fr++;
        fr %= CHPP_TX_DATAGRAM_QUEUE_LEN;

        EXPECT_TRUE(chppDequeueTxDatagram(&transportContext));

        EXPECT_EQ(transportContext.txDatagramQueue.front, fr);
        EXPECT_EQ(transportContext.txDatagramQueue.pending, i - 1);
      }

      EXPECT_FALSE(chppDequeueTxDatagram(&transportContext));

      EXPECT_EQ(transportContext.txDatagramQueue.front, fr);
      EXPECT_EQ(transportContext.txDatagramQueue.pending, 0);
    }
  }
}

/**
 * Loopback testing with various length payloads of zeros
 */
TEST_P(TransportTests, LoopbackPayloadOfZeros) {
  transportContext.rxStatus.state = CHPP_STATE_HEADER;
  size_t len = static_cast<size_t>(GetParam());
  std::thread t1(chppWorkThreadStart, &transportContext);

  if (len <= kMaxChunkSize) {
    ChppTransportHeader header{};
    header.flags = 0;
    header.errorCode = 0;
    header.ackSeq = 1;
    header.seq = 0;
    header.length = len;

    memcpy(buf, &header, sizeof(header));

    buf[sizeof(ChppTransportHeader)] = CHPP_HANDLE_LOOPBACK;
    buf[sizeof(ChppTransportHeader) + 1] = CHPP_MESSAGE_TYPE_CLIENT_REQUEST;

    // TODO: Add checksum

    // Send header + payload (if any) + footer
    EXPECT_TRUE(chppRxDataCb(
        &transportContext, buf,
        sizeof(ChppTransportHeader) + len + sizeof(ChppTransportFooter)));

    // Check for correct state
    EXPECT_EQ(transportContext.rxStatus.state, CHPP_STATE_PREAMBLE);

    // The next expected packet sequence # should incremented only if the
    // received packet is payload-bearing.
    uint8_t nextSeq = header.seq + ((len > 0) ? 1 : 0);
    EXPECT_EQ(transportContext.rxStatus.expectedSeq, nextSeq);

    WaitForTransport(&transportContext);

    // Check for correct response packet crafting if applicable
    if (len > 0) {
      // Check response packet fields
      struct ChppTransportHeader *txHeader =
          (struct ChppTransportHeader *)&transportContext.pendingTxPacket
              .payload[CHPP_PREAMBLE_LEN_BYTES];

      // If datagram is larger than Tx MTU, the response packet should be the
      // first fragment
      size_t mtu_len = MIN(len, CHPP_TRANSPORT_TX_MTU_BYTES);
      uint8_t flags = (mtu_len == len)
                          ? CHPP_TRANSPORT_FLAG_FINISHED_DATAGRAM
                          : CHPP_TRANSPORT_FLAG_UNFINISHED_DATAGRAM;

      // Correct loopback command requires min of 2 bytes payload
      if (len < 2) {
        mtu_len = 0;
      }

      // Check response packet parameters
      EXPECT_EQ(txHeader->flags, flags);
      EXPECT_EQ(txHeader->errorCode, CHPP_TRANSPORT_ERROR_NONE);
      EXPECT_EQ(txHeader->ackSeq, nextSeq);
      EXPECT_EQ(txHeader->length, mtu_len);

      // Check response packet length
      EXPECT_EQ(transportContext.pendingTxPacket.length,
                CHPP_PREAMBLE_LEN_BYTES + sizeof(struct ChppTransportHeader) +
                    mtu_len + sizeof(struct ChppTransportFooter));

      // Check response packet payload
      if (len >= 2) {
        EXPECT_EQ(transportContext.pendingTxPacket
                      .payload[CHPP_PREAMBLE_LEN_BYTES +
                               sizeof(struct ChppTransportHeader)],
                  CHPP_HANDLE_LOOPBACK);
        EXPECT_EQ(transportContext.pendingTxPacket
                      .payload[CHPP_PREAMBLE_LEN_BYTES +
                               sizeof(struct ChppTransportHeader) + 1],
                  CHPP_MESSAGE_TYPE_SERVICE_RESPONSE);
      }
    }

    // Should have reset loc and length for next packet / datagram
    EXPECT_EQ(transportContext.rxStatus.locInDatagram, 0);
    EXPECT_EQ(transportContext.rxDatagram.length, 0);
  }

  chppWorkThreadStop(&transportContext);
  t1.join();
}

/**
 * Discovery service
 */
TEST_F(TransportTests, DiscoveryService) {
  transportContext.rxStatus.state = CHPP_STATE_HEADER;
  size_t len = 0;
  std::thread t1(chppWorkThreadStart, &transportContext);

  ChppTransportHeader transHeader{};
  transHeader.flags = 0;
  transHeader.errorCode = 0;
  transHeader.ackSeq = 1;
  transHeader.seq = 0;
  transHeader.length = sizeof(ChppAppHeader);

  memcpy(&buf[len], &transHeader, sizeof(transHeader));
  len += sizeof(transHeader);

  ChppAppHeader appHeader{};

  appHeader.handle = CHPP_HANDLE_DISCOVERY;
  appHeader.type = CHPP_MESSAGE_TYPE_CLIENT_REQUEST;
  appHeader.transaction = 1;
  appHeader.command = CHPP_DISCOVERY_COMMAND_DISCOVER_ALL;

  memcpy(&buf[len], &appHeader, sizeof(appHeader));
  len += sizeof(appHeader);

  // TODO: Add checksum
  len += sizeof(ChppTransportFooter);

  // Send header + payload (if any) + footer
  EXPECT_TRUE(chppRxDataCb(&transportContext, buf, len));

  // Check for correct state
  EXPECT_EQ(transportContext.rxStatus.state, CHPP_STATE_PREAMBLE);

  // The next expected packet sequence # should incremented
  uint8_t nextSeq = transHeader.seq + 1;
  EXPECT_EQ(transportContext.rxStatus.expectedSeq, nextSeq);

  // Wait for response
  WaitForTransport(&transportContext);

  // Check response packet fields
  struct ChppTransportHeader *txHeader =
      (struct ChppTransportHeader *)&transportContext.pendingTxPacket
          .payload[CHPP_PREAMBLE_LEN_BYTES];

  // Check response packet parameters
  EXPECT_EQ(txHeader->errorCode, CHPP_TRANSPORT_ERROR_NONE);
  EXPECT_EQ(txHeader->ackSeq, nextSeq);

  // TODO: more tests

  // Check response packet payload
  EXPECT_EQ(transportContext.pendingTxPacket
                .payload[CHPP_PREAMBLE_LEN_BYTES +
                         sizeof(struct ChppTransportHeader)],
            CHPP_HANDLE_DISCOVERY);
  EXPECT_EQ(transportContext.pendingTxPacket
                .payload[CHPP_PREAMBLE_LEN_BYTES +
                         sizeof(struct ChppTransportHeader) + 1],
            CHPP_MESSAGE_TYPE_SERVICE_RESPONSE);

  // Should have reset loc and length for next packet / datagram
  EXPECT_EQ(transportContext.rxStatus.locInDatagram, 0);
  EXPECT_EQ(transportContext.rxDatagram.length, 0);

  chppWorkThreadStop(&transportContext);
  t1.join();
}

INSTANTIATE_TEST_SUITE_P(TransportTestRange, TransportTests,
                         testing::ValuesIn(kChunkSizes));

}  // namespace
