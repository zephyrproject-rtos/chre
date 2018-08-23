/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "gtest/gtest.h"

#include "chre/util/buffer.h"
#include "chre/util/macros.h"

using chre::Buffer;

void fillBufferWithSequentialValues(float *buffer, size_t size) {
  for (size_t i = 0; i < size; i++) {
    buffer[i] = static_cast<float>(i);
  }
}

TEST(Buffer, EmptyByDefault) {
  Buffer<float> byteBuffer;
  EXPECT_EQ(byteBuffer.data(), nullptr);
  EXPECT_EQ(byteBuffer.size(), 0);
}

TEST(Buffer, Wrap) {
  float buffer[128];
  fillBufferWithSequentialValues(buffer, ARRAY_SIZE(buffer));

  Buffer<float> byteBuffer;
  byteBuffer.wrap(buffer, ARRAY_SIZE(buffer));
  EXPECT_EQ(byteBuffer.data(), buffer);
  EXPECT_EQ(byteBuffer.size(), ARRAY_SIZE(buffer));
}

TEST(Buffer, CopyBuffer) {
  float buffer[128];
  fillBufferWithSequentialValues(buffer, ARRAY_SIZE(buffer));

  Buffer<float> byteBuffer;
  EXPECT_TRUE(byteBuffer.copy_array(buffer, ARRAY_SIZE(buffer)));
  EXPECT_EQ(byteBuffer.size(), ARRAY_SIZE(buffer));

  for (size_t i = 0; i < ARRAY_SIZE(buffer); i++) {
    EXPECT_EQ(byteBuffer.data()[i], static_cast<float>(i));
  }
}
