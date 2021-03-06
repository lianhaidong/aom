/*
 * Copyright (c) 2017, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <altivec.h>

#include "config/av1_rtcd.h"

#include "av1/common/cfl.h"

#define OFF_0 0
#define OFF_1 16
#define OFF_2 32
#define OFF_3 48
#define CFL_BUF_LINE_BYTES 64
#define CFL_LINE_1 64
#define CFL_LINE_2 128
#define CFL_LINE_3 192

typedef vector int8_t int8x16_t;
typedef vector uint8_t uint8x16_t;
typedef vector int16_t int16x8_t;
typedef vector uint16_t uint16x8_t;
typedef vector int32_t int32x4_t;
typedef vector uint32_t uint32x4_t;
typedef vector uint64_t uint64x2_t;

static INLINE void subtract_average_vsx(int16_t *pred_buf, int width,
                                        int height, int round_offset,
                                        int num_pel_log2) {
  const int16_t *end = pred_buf + height * CFL_BUF_LINE;
  const int16_t *sum_buf = pred_buf;
  const uint32x4_t div_shift = vec_splats((uint32_t)num_pel_log2);
  const uint8x16_t mask_64 = { 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                               0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
  const uint8x16_t mask_32 = { 0x14, 0x15, 0x16, 0x17, 0x00, 0x01, 0x02, 0x03,
                               0x1C, 0x1D, 0x1E, 0x1F, 0x08, 0x09, 0x0A, 0x0B };

  int32x4_t sum_32x4_0 = { 0, 0, 0, round_offset };
  int32x4_t sum_32x4_1 = { 0, 0, 0, 0 };
  do {
    sum_32x4_0 = vec_sum4s(vec_vsx_ld(OFF_0, sum_buf), sum_32x4_0);
    sum_32x4_1 = vec_sum4s(vec_vsx_ld(OFF_0 + CFL_LINE_1, sum_buf), sum_32x4_1);
    if (width >= 16) {
      sum_32x4_0 = vec_sum4s(vec_vsx_ld(OFF_1, sum_buf), sum_32x4_0);
      sum_32x4_1 =
          vec_sum4s(vec_vsx_ld(OFF_1 + CFL_LINE_1, sum_buf), sum_32x4_1);
    }
    if (width == 32) {
      sum_32x4_0 = vec_sum4s(vec_vsx_ld(OFF_2, sum_buf), sum_32x4_0);
      sum_32x4_1 =
          vec_sum4s(vec_vsx_ld(OFF_2 + CFL_LINE_1, sum_buf), sum_32x4_1);
      sum_32x4_0 = vec_sum4s(vec_vsx_ld(OFF_3, sum_buf), sum_32x4_0);
      sum_32x4_1 =
          vec_sum4s(vec_vsx_ld(OFF_3 + CFL_LINE_1, sum_buf), sum_32x4_1);
    }
  } while ((sum_buf += (CFL_BUF_LINE * 2)) < end);
  int32x4_t sum_32x4 = vec_add(sum_32x4_0, sum_32x4_1);

  const int32x4_t perm_64 = vec_perm(sum_32x4, sum_32x4, mask_64);
  sum_32x4 = vec_add(sum_32x4, perm_64);
  const int32x4_t perm_32 = vec_perm(sum_32x4, sum_32x4, mask_32);
  sum_32x4 = vec_add(sum_32x4, perm_32);
  const int32x4_t avg = vec_sr(sum_32x4, div_shift);
  const int16x8_t vec_avg = vec_pack(avg, avg);
  do {
    vec_vsx_st(vec_sub(vec_vsx_ld(OFF_0, pred_buf), vec_avg), OFF_0, pred_buf);
    vec_vsx_st(vec_sub(vec_vsx_ld(OFF_0 + CFL_LINE_1, pred_buf), vec_avg),
               OFF_0 + CFL_BUF_LINE_BYTES, pred_buf);
    vec_vsx_st(vec_sub(vec_vsx_ld(OFF_0 + CFL_LINE_2, pred_buf), vec_avg),
               OFF_0 + CFL_LINE_2, pred_buf);
    vec_vsx_st(vec_sub(vec_vsx_ld(OFF_0 + CFL_LINE_3, pred_buf), vec_avg),
               OFF_0 + CFL_LINE_3, pred_buf);
    if (width >= 16) {
      vec_vsx_st(vec_sub(vec_vsx_ld(OFF_1, pred_buf), vec_avg), OFF_1,
                 pred_buf);
      vec_vsx_st(vec_sub(vec_vsx_ld(OFF_1 + CFL_LINE_1, pred_buf), vec_avg),
                 OFF_1 + CFL_LINE_1, pred_buf);
      vec_vsx_st(vec_sub(vec_vsx_ld(OFF_1 + CFL_LINE_2, pred_buf), vec_avg),
                 OFF_1 + CFL_LINE_2, pred_buf);
      vec_vsx_st(vec_sub(vec_vsx_ld(OFF_1 + CFL_LINE_3, pred_buf), vec_avg),
                 OFF_1 + CFL_LINE_3, pred_buf);
    }
    if (width == 32) {
      vec_vsx_st(vec_sub(vec_vsx_ld(OFF_2, pred_buf), vec_avg), OFF_2,
                 pred_buf);
      vec_vsx_st(vec_sub(vec_vsx_ld(OFF_2 + CFL_LINE_1, pred_buf), vec_avg),
                 OFF_2 + CFL_LINE_1, pred_buf);
      vec_vsx_st(vec_sub(vec_vsx_ld(OFF_2 + CFL_LINE_2, pred_buf), vec_avg),
                 OFF_2 + CFL_LINE_2, pred_buf);
      vec_vsx_st(vec_sub(vec_vsx_ld(OFF_2 + CFL_LINE_3, pred_buf), vec_avg),
                 OFF_2 + CFL_LINE_3, pred_buf);

      vec_vsx_st(vec_sub(vec_vsx_ld(OFF_3, pred_buf), vec_avg), OFF_3,
                 pred_buf);
      vec_vsx_st(vec_sub(vec_vsx_ld(OFF_3 + CFL_LINE_1, pred_buf), vec_avg),
                 OFF_3 + CFL_LINE_1, pred_buf);
      vec_vsx_st(vec_sub(vec_vsx_ld(OFF_3 + CFL_LINE_2, pred_buf), vec_avg),
                 OFF_3 + CFL_LINE_2, pred_buf);
      vec_vsx_st(vec_sub(vec_vsx_ld(OFF_3 + CFL_LINE_3, pred_buf), vec_avg),
                 OFF_3 + CFL_LINE_3, pred_buf);
    }
  } while ((pred_buf += CFL_BUF_LINE * 4) < end);
}

// Declare wrappers for VSX sizes
CFL_SUB_AVG_X(vsx, 8, 4, 16, 5)
CFL_SUB_AVG_X(vsx, 8, 8, 32, 6)
CFL_SUB_AVG_X(vsx, 8, 16, 64, 7)
CFL_SUB_AVG_X(vsx, 8, 32, 128, 8)
CFL_SUB_AVG_X(vsx, 16, 4, 32, 6)
CFL_SUB_AVG_X(vsx, 16, 8, 64, 7)
CFL_SUB_AVG_X(vsx, 16, 16, 128, 8)
CFL_SUB_AVG_X(vsx, 16, 32, 256, 9)
CFL_SUB_AVG_X(vsx, 32, 8, 128, 8)
CFL_SUB_AVG_X(vsx, 32, 16, 256, 9)
CFL_SUB_AVG_X(vsx, 32, 32, 512, 10)

// Based on observation, for small blocks VSX does not outperform C (no 64bit
// load and store intrinsics). So we call the C code for block widths 4.
cfl_subtract_average_fn get_subtract_average_fn_vsx(TX_SIZE tx_size) {
  static const cfl_subtract_average_fn sub_avg[TX_SIZES_ALL] = {
    subtract_average_4x4_c,     /* 4x4 */
    subtract_average_8x8_vsx,   /* 8x8 */
    subtract_average_16x16_vsx, /* 16x16 */
    subtract_average_32x32_vsx, /* 32x32 */
    cfl_subtract_average_null,  /* 64x64 (invalid CFL size) */
    subtract_average_4x8_c,     /* 4x8 */
    subtract_average_8x4_vsx,   /* 8x4 */
    subtract_average_8x16_vsx,  /* 8x16 */
    subtract_average_16x8_vsx,  /* 16x8 */
    subtract_average_16x32_vsx, /* 16x32 */
    subtract_average_32x16_vsx, /* 32x16 */
    cfl_subtract_average_null,  /* 32x64 (invalid CFL size) */
    cfl_subtract_average_null,  /* 64x32 (invalid CFL size) */
    subtract_average_4x16_c,    /* 4x16 */
    subtract_average_16x4_vsx,  /* 16x4 */
    subtract_average_8x32_vsx,  /* 8x32 */
    subtract_average_32x8_vsx,  /* 32x8 */
    cfl_subtract_average_null,  /* 16x64 (invalid CFL size) */
    cfl_subtract_average_null,  /* 64x16 (invalid CFL size) */
  };
  // Modulo TX_SIZES_ALL to ensure that an attacker won't be able to
  // index the function pointer array out of bounds.
  return sub_avg[tx_size % TX_SIZES_ALL];
}
