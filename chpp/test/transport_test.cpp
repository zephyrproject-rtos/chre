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

#include "chpp/transport.h"

namespace {

// Max size of payload sent to chppRxData (bytes)
constexpr size_t kMaxChunkSize = 20000;

constexpr size_t kMaxPacketSize = kMaxChunkSize + CHPP_PREAMBLE_LEN_BYTES +
                                  sizeof(ChppTransportHeader) +
                                  sizeof(ChppTransportFooter);

// Input sizes to test the entire range of sizes with a few tests
constexpr int kChunkSizes[] = {0,  1,   2,   3,    4,     5,    6,
                               7,  8,   9,   10,   20,    30,   40,
                               51, 100, 201, 1000, 10001, 20000};

/**
 * Adds a CHPP preamble to the specified location of buf
 *
 * @param buf The CHPP preamble will be added to buf
 * @param loc Location of buf where the CHPP preamble will be added
 */
void chppAddPreamble(uint8_t *buf, size_t loc) {
  for (size_t i = 0; i < CHPP_PREAMBLE_LEN_BYTES; i++) {
    buf[loc + i] = static_cast<uint8_t>(
        CHPP_PREAMBLE_DATA >> (CHPP_PREAMBLE_LEN_BYTES - 1 - i) & 0xff);
  }
}

/*
 * Test suite for the CHPP Transport Layer
 */
class TransportTests : public testing::TestWithParam<int> {
 protected:
  ChppTransportState context = {};
  uint8_t buf[kMaxPacketSize] = {};
};

/**
 * A series of zeros shouldn't change state from CHPP_STATE_PREAMBLE
 */
TEST_P(TransportTests, ZeroNoPreambleInput) {
  size_t len = static_cast<size_t>(GetParam());
  if (len <= kMaxChunkSize) {
    EXPECT_TRUE(chppRxData(&context, buf, len));
    EXPECT_EQ(context.rxStatus.state, CHPP_STATE_PREAMBLE);
  }
}

/**
 * A preamble after a series of zeros input should change state from
 * CHPP_STATE_PREAMBLE to CHPP_STATE_HEADER
 */
TEST_P(TransportTests, ZeroThenPreambleInput) {
  size_t len = static_cast<size_t>(GetParam());

  if (len <= kMaxChunkSize) {
    // Add preamble at the end of buf
    chppAddPreamble(buf, MAX(0, len - CHPP_PREAMBLE_LEN_BYTES));

    if (len >= CHPP_PREAMBLE_LEN_BYTES) {
      EXPECT_FALSE(chppRxData(&context, buf, len));
      EXPECT_EQ(context.rxStatus.state, CHPP_STATE_HEADER);
    } else {
      EXPECT_TRUE(chppRxData(&context, buf, len));
      EXPECT_EQ(context.rxStatus.state, CHPP_STATE_PREAMBLE);
    }
  }
}

/**
 * Rx Testing with various length payloads of zeros
 */
TEST_P(TransportTests, RxPayloadOfZeros) {
  context.rxStatus.state = CHPP_STATE_HEADER;
  size_t len = static_cast<size_t>(GetParam());

  if (len <= kMaxChunkSize) {
    ChppTransportHeader header{};
    header.flags = 0;
    header.errorCode = 0;
    header.ackSeq = 1;
    header.seq = 0;
    header.length = len;

    memcpy(buf, &header, sizeof(header));

    // Send header and check for correct state
    EXPECT_FALSE(chppRxData(&context, buf, sizeof(ChppTransportHeader)));
    if (len > 0) {
      EXPECT_EQ(context.rxStatus.state, CHPP_STATE_PAYLOAD);
    } else {
      EXPECT_EQ(context.rxStatus.state, CHPP_STATE_FOOTER);
    }

    // Correct decoding of packet length
    EXPECT_EQ(context.rxHeader.length, len);
    EXPECT_EQ(context.rxDatagram.loc, 0);
    EXPECT_EQ(context.rxDatagram.length, len);

    // Send payload if any and check for correct state
    if (len > 0) {
      EXPECT_FALSE(
          chppRxData(&context, &buf[sizeof(ChppTransportHeader)], len));
      EXPECT_EQ(context.rxStatus.state, CHPP_STATE_FOOTER);
    }

    // Should have complete packet payload by now
    EXPECT_EQ(context.rxDatagram.loc, len);

    // Send footer and check for correct state
    EXPECT_TRUE(chppRxData(&context, &buf[sizeof(ChppTransportHeader) + len],
                           sizeof(ChppTransportFooter)));
    EXPECT_EQ(context.rxStatus.state, CHPP_STATE_PREAMBLE);

    // Should have reset loc and length for next packet / datagram
    EXPECT_EQ(context.rxDatagram.loc, 0);
    EXPECT_EQ(context.rxDatagram.length, 0);

    // If payload packet, expect next packet with incremented sequence #
    uint8_t seq = header.seq + ((len > 0) ? 1 : 0);

    EXPECT_EQ(context.rxStatus.expectedSeq, seq);
    EXPECT_EQ(context.txHeader.ackSeq, seq);
  }
}

INSTANTIATE_TEST_SUITE_P(TransportTestRange, TransportTests,
                         testing::ValuesIn(kChunkSizes));

}  // namespace
