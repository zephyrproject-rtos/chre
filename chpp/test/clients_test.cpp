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

#include <gtest/gtest.h>

#include "chpp/clients.h"
#include "chpp/services.h"

class ClientsTest : public testing::Test {
 protected:
  struct ChppClientState mClientState;
  struct ChppRequestResponseState mRRState;
};

TEST_F(ClientsTest, RequestResponseTimestampValid) {
  struct ChppAppHeader *reqHeader =
      chppAllocClientRequestCommand(&mClientState, 0 /* command */);
  chppClientTimestampRequest(&mRRState, reqHeader);

  struct ChppAppHeader *respHeader =
      chppAllocServiceResponse(reqHeader, sizeof(*reqHeader));
  ASSERT_TRUE(chppClientTimestampResponse(&mRRState, respHeader));

  chppFree(reqHeader);
  chppFree(respHeader);
}

TEST_F(ClientsTest, RequestResponseTimestampDuplicate) {
  struct ChppAppHeader *reqHeader =
      chppAllocClientRequestCommand(&mClientState, 0 /* command */);
  chppClientTimestampRequest(&mRRState, reqHeader);

  struct ChppAppHeader *respHeader =
      chppAllocServiceResponse(reqHeader, sizeof(*reqHeader));
  ASSERT_TRUE(chppClientTimestampResponse(&mRRState, respHeader));
  ASSERT_FALSE(chppClientTimestampResponse(&mRRState, respHeader));

  chppFree(reqHeader);
  chppFree(respHeader);
}

TEST_F(ClientsTest, RequestResponseTimestampInvalidId) {
  struct ChppAppHeader *reqHeader =
      chppAllocClientRequestCommand(&mClientState, 0 /* command */);
  chppClientTimestampRequest(&mRRState, reqHeader);

  struct ChppAppHeader *newReqHeader =
      chppAllocClientRequestCommand(&mClientState, 0 /* command */);
  struct ChppAppHeader *respHeader =
      chppAllocServiceResponse(newReqHeader, sizeof(*reqHeader));
  ASSERT_FALSE(chppClientTimestampResponse(&mRRState, respHeader));

  chppFree(reqHeader);
  chppFree(newReqHeader);
  chppFree(respHeader);
}