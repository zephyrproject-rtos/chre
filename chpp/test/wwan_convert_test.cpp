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

#include <string.h>

#include "chpp/memory.h"
#include "chpp/services/wwan_types.h"

TEST(WwanConvert, EncodeErrorCode) {
  const chreWwanCellInfoResult chreResult = {
      .version = 200,  // ignored
      .errorCode = 2,
      .cellInfoCount = 0,
      .reserved = 3,         // ignored
      .cookie = (void *)-1,  // ignored
      .cells = nullptr,
  };

  ChppWwanCellInfoResultWithHeader *chppWithHeader = nullptr;
  size_t outputSize = 999;
  bool result =
      chppWwanCellInfoResultFromChre(&chreResult, &chppWithHeader, &outputSize);
  ASSERT_TRUE(result);
  ASSERT_NE(chppWithHeader, nullptr);
  EXPECT_EQ(outputSize, sizeof(ChppWwanCellInfoResultWithHeader));

  ChppWwanCellInfoResult *chpp = &chppWithHeader->payload;
  EXPECT_EQ(chpp->version, CHRE_WWAN_CELL_INFO_RESULT_VERSION);
  EXPECT_EQ(chpp->errorCode, chreResult.errorCode);
  EXPECT_EQ(chpp->cellInfoCount, chreResult.cellInfoCount);
  EXPECT_EQ(chpp->reserved, 0);
  EXPECT_EQ(chpp->cookie, 0);
  EXPECT_EQ(chpp->cells.offset, 0);
  EXPECT_EQ(chpp->cells.length, 0);

  chppFree(chppWithHeader);
}

TEST(WwanConvert, SingleCell) {
  // clang-format off
  const chreWwanCellInfo chreCell = {
    .timeStamp = 1234,
    .cellInfoType = CHRE_WWAN_CELL_INFO_TYPE_LTE,
    .timeStampType = CHRE_WWAN_CELL_TIMESTAMP_TYPE_MODEM,
    .registered = 1,
    .reserved = 111,  // ignored
    .CellInfo = {
      .lte = {
        .cellIdentityLte = {
          .mcc = 777,
          .mnc = 888,
          .ci = 4321,
          .pci = 333,
          .tac = 9876,
          .earfcn = 5432,
        },
        .signalStrengthLte = {
          .signalStrength = 27,
          .rsrp = 96,
          .rsrq = 18,
          .rssnr = 157,
          .cqi = 13,
          .timingAdvance = INT32_MAX,
        }
      }
    }
  };
  const chreWwanCellInfoResult chreResult = {
    .errorCode = 0,
    .cellInfoCount = 1,
    .cells = &chreCell,
  };
  // clang-format on

  ChppWwanCellInfoResultWithHeader *chppWithHeader = nullptr;
  size_t outputSize = 999;
  bool result =
      chppWwanCellInfoResultFromChre(&chreResult, &chppWithHeader, &outputSize);
  ASSERT_TRUE(result);
  ASSERT_NE(chppWithHeader, nullptr);
  EXPECT_EQ(outputSize, sizeof(ChppWwanCellInfoResultWithHeader) +
                            sizeof(ChppWwanCellInfo));

  ChppWwanCellInfoResult *chpp = &chppWithHeader->payload;
  EXPECT_EQ(chpp->errorCode, chreResult.errorCode);
  EXPECT_EQ(chpp->cellInfoCount, chreResult.cellInfoCount);
  EXPECT_EQ(chpp->cells.offset, sizeof(ChppWwanCellInfoResult));
  EXPECT_EQ(chpp->cells.length, sizeof(ChppWwanCellInfo));

  ChppWwanCellInfo *chppCell =
      (ChppWwanCellInfo *)((uint8_t *)chpp + chpp->cells.offset);
  EXPECT_EQ(chppCell->timeStamp, chreCell.timeStamp);
  EXPECT_EQ(chppCell->cellInfoType, chreCell.cellInfoType);
  EXPECT_EQ(chppCell->timeStampType, chreCell.timeStampType);
  EXPECT_EQ(chppCell->registered, chreCell.registered);
  EXPECT_EQ(chppCell->reserved, 0);

  EXPECT_EQ(chppCell->CellInfo.lte.cellIdentityLte.mcc,
            chreCell.CellInfo.lte.cellIdentityLte.mcc);
  EXPECT_EQ(chppCell->CellInfo.lte.cellIdentityLte.mnc,
            chreCell.CellInfo.lte.cellIdentityLte.mnc);
  EXPECT_EQ(chppCell->CellInfo.lte.cellIdentityLte.ci,
            chreCell.CellInfo.lte.cellIdentityLte.ci);
  EXPECT_EQ(chppCell->CellInfo.lte.cellIdentityLte.pci,
            chreCell.CellInfo.lte.cellIdentityLte.pci);
  EXPECT_EQ(chppCell->CellInfo.lte.cellIdentityLte.tac,
            chreCell.CellInfo.lte.cellIdentityLte.tac);
  EXPECT_EQ(chppCell->CellInfo.lte.cellIdentityLte.earfcn,
            chreCell.CellInfo.lte.cellIdentityLte.earfcn);

  EXPECT_EQ(chppCell->CellInfo.lte.signalStrengthLte.signalStrength,
            chreCell.CellInfo.lte.signalStrengthLte.signalStrength);
  EXPECT_EQ(chppCell->CellInfo.lte.signalStrengthLte.rsrp,
            chreCell.CellInfo.lte.signalStrengthLte.rsrp);
  EXPECT_EQ(chppCell->CellInfo.lte.signalStrengthLte.rsrq,
            chreCell.CellInfo.lte.signalStrengthLte.rsrq);
  EXPECT_EQ(chppCell->CellInfo.lte.signalStrengthLte.rssnr,
            chreCell.CellInfo.lte.signalStrengthLte.rssnr);
  EXPECT_EQ(chppCell->CellInfo.lte.signalStrengthLte.cqi,
            chreCell.CellInfo.lte.signalStrengthLte.cqi);
  EXPECT_EQ(chppCell->CellInfo.lte.signalStrengthLte.timingAdvance,
            chreCell.CellInfo.lte.signalStrengthLte.timingAdvance);

  chppFree(chppWithHeader);
}

TEST(WwanConvert, TwoCells) {
  // clang-format off
  const chreWwanCellInfo chreCells[] = {
    {
      .timeStamp = 1234,
      .cellInfoType = CHRE_WWAN_CELL_INFO_TYPE_LTE,
      .timeStampType = CHRE_WWAN_CELL_TIMESTAMP_TYPE_MODEM,
      .registered = 1,
      .reserved = 111,  // ignored
      .CellInfo = {
        .lte = {
          .cellIdentityLte = {
            .mcc = 777,
            .mnc = 888,
            .ci = 4321,
            .pci = 333,
            .tac = 9876,
            .earfcn = 5432,
          },
          .signalStrengthLte = {
            .signalStrength = 27,
            .rsrp = 96,
            .rsrq = 18,
            .rssnr = 157,
            .cqi = 13,
            .timingAdvance = INT32_MAX,
          }
        }
      }
    },
    {
      .timeStamp = 1235,
      .cellInfoType = CHRE_WWAN_CELL_INFO_TYPE_WCDMA,
      .timeStampType = CHRE_WWAN_CELL_TIMESTAMP_TYPE_ANTENNA,
      .registered = 0,
      .CellInfo = {
        .wcdma = {
          .cellIdentityWcdma = {
            .mcc = 123,
            .mnc = 456,
            .lac = 789,
            .cid = 012,
            .psc = 345,
            .uarfcn = 678,
          },
          .signalStrengthWcdma = {
            .signalStrength = 99,
            .bitErrorRate = INT32_MAX,
          }
        }
      }
    },
  };
  const chreWwanCellInfoResult chreResult = {
    .errorCode = 0,
    .cellInfoCount = 2,
    .cells = chreCells,
  };
  // clang-format on

  ChppWwanCellInfoResultWithHeader *chppWithHeader = nullptr;
  size_t outputSize = 999;
  bool result =
      chppWwanCellInfoResultFromChre(&chreResult, &chppWithHeader, &outputSize);
  ASSERT_TRUE(result);
  ASSERT_NE(chppWithHeader, nullptr);
  EXPECT_EQ(outputSize, sizeof(ChppWwanCellInfoResultWithHeader) +
                            2 * sizeof(ChppWwanCellInfo));

  ChppWwanCellInfoResult *chpp = &chppWithHeader->payload;
  EXPECT_EQ(chpp->errorCode, chreResult.errorCode);
  EXPECT_EQ(chpp->cellInfoCount, chreResult.cellInfoCount);
  EXPECT_EQ(chpp->cells.offset, sizeof(ChppWwanCellInfoResult));
  EXPECT_EQ(chpp->cells.length, 2 * sizeof(ChppWwanCellInfo));

  ChppWwanCellInfo *chppCells =
      (ChppWwanCellInfo *)((uint8_t *)chpp + chpp->cells.offset);
  EXPECT_EQ(chppCells[0].timeStamp, chreCells[0].timeStamp);
  EXPECT_EQ(chppCells[0].cellInfoType, chreCells[0].cellInfoType);
  EXPECT_EQ(chppCells[0].timeStampType, chreCells[0].timeStampType);
  EXPECT_EQ(chppCells[0].registered, chreCells[0].registered);
  EXPECT_EQ(chppCells[0].reserved, 0);

  EXPECT_EQ(chppCells[0].CellInfo.lte.cellIdentityLte.mcc,
            chreCells[0].CellInfo.lte.cellIdentityLte.mcc);
  EXPECT_EQ(chppCells[0].CellInfo.lte.cellIdentityLte.mnc,
            chreCells[0].CellInfo.lte.cellIdentityLte.mnc);
  EXPECT_EQ(chppCells[0].CellInfo.lte.cellIdentityLte.ci,
            chreCells[0].CellInfo.lte.cellIdentityLte.ci);
  EXPECT_EQ(chppCells[0].CellInfo.lte.cellIdentityLte.pci,
            chreCells[0].CellInfo.lte.cellIdentityLte.pci);
  EXPECT_EQ(chppCells[0].CellInfo.lte.cellIdentityLte.tac,
            chreCells[0].CellInfo.lte.cellIdentityLte.tac);
  EXPECT_EQ(chppCells[0].CellInfo.lte.cellIdentityLte.earfcn,
            chreCells[0].CellInfo.lte.cellIdentityLte.earfcn);

  EXPECT_EQ(chppCells[0].CellInfo.lte.signalStrengthLte.signalStrength,
            chreCells[0].CellInfo.lte.signalStrengthLte.signalStrength);
  EXPECT_EQ(chppCells[0].CellInfo.lte.signalStrengthLte.rsrp,
            chreCells[0].CellInfo.lte.signalStrengthLte.rsrp);
  EXPECT_EQ(chppCells[0].CellInfo.lte.signalStrengthLte.rsrq,
            chreCells[0].CellInfo.lte.signalStrengthLte.rsrq);
  EXPECT_EQ(chppCells[0].CellInfo.lte.signalStrengthLte.rssnr,
            chreCells[0].CellInfo.lte.signalStrengthLte.rssnr);
  EXPECT_EQ(chppCells[0].CellInfo.lte.signalStrengthLte.cqi,
            chreCells[0].CellInfo.lte.signalStrengthLte.cqi);
  EXPECT_EQ(chppCells[0].CellInfo.lte.signalStrengthLte.timingAdvance,
            chreCells[0].CellInfo.lte.signalStrengthLte.timingAdvance);

  EXPECT_EQ(chppCells[1].timeStamp, chreCells[1].timeStamp);
  EXPECT_EQ(chppCells[1].cellInfoType, chreCells[1].cellInfoType);
  EXPECT_EQ(chppCells[1].timeStampType, chreCells[1].timeStampType);
  EXPECT_EQ(chppCells[1].registered, chreCells[1].registered);
  EXPECT_EQ(chppCells[1].reserved, 0);

  EXPECT_EQ(chppCells[1].CellInfo.wcdma.cellIdentityWcdma.mcc,
            chreCells[1].CellInfo.wcdma.cellIdentityWcdma.mcc);
  EXPECT_EQ(chppCells[1].CellInfo.wcdma.cellIdentityWcdma.mnc,
            chreCells[1].CellInfo.wcdma.cellIdentityWcdma.mnc);
  EXPECT_EQ(chppCells[1].CellInfo.wcdma.cellIdentityWcdma.lac,
            chreCells[1].CellInfo.wcdma.cellIdentityWcdma.lac);
  EXPECT_EQ(chppCells[1].CellInfo.wcdma.cellIdentityWcdma.cid,
            chreCells[1].CellInfo.wcdma.cellIdentityWcdma.cid);
  EXPECT_EQ(chppCells[1].CellInfo.wcdma.cellIdentityWcdma.psc,
            chreCells[1].CellInfo.wcdma.cellIdentityWcdma.psc);
  EXPECT_EQ(chppCells[1].CellInfo.wcdma.cellIdentityWcdma.uarfcn,
            chreCells[1].CellInfo.wcdma.cellIdentityWcdma.uarfcn);

  EXPECT_EQ(chppCells[1].CellInfo.wcdma.signalStrengthWcdma.signalStrength,
            chreCells[1].CellInfo.wcdma.signalStrengthWcdma.signalStrength);
  EXPECT_EQ(chppCells[1].CellInfo.wcdma.signalStrengthWcdma.bitErrorRate,
            chreCells[1].CellInfo.wcdma.signalStrengthWcdma.bitErrorRate);

  // Ensure unused bytes in the union are zeroed out
  uint8_t *pastEnd =
      (uint8_t *)&chppCells[1].CellInfo.wcdma.signalStrengthWcdma.bitErrorRate +
      sizeof(chppCells[1].CellInfo.wcdma.signalStrengthWcdma.bitErrorRate);
  size_t sizePastEnd = sizeof(chreWwanCellInfo::chreWwanCellInfoPerRat) -
                       sizeof(chreWwanCellInfoWcdma);
  uint8_t zeros[sizePastEnd];
  memset(zeros, 0, sizePastEnd);
  EXPECT_EQ(memcmp(pastEnd, zeros, sizeof(zeros)), 0);

  chppFree(chppWithHeader);
}
