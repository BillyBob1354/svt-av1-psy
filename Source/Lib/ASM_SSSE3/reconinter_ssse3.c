/*
 * Copyright (c) 2018, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at https://www.aomedia.org/license/software-license. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at https://www.aomedia.org/license/patent-license.
 */

#include "definitions.h"
#include "common_dsp_rtcd.h"
#include <tmmintrin.h>
#include "synonyms.h"

void svt_av1_build_compound_diffwtd_mask_highbd_ssse3(uint8_t *mask, DIFFWTD_MASK_TYPE mask_type, const uint8_t *src0,
                                                      int src0_stride, const uint8_t *src1, int src1_stride, int h,
                                                      int w, int bd) {
    if (w < 8) {
        svt_av1_build_compound_diffwtd_mask_highbd_c(mask, mask_type, src0, src0_stride, src1, src1_stride, h, w, bd);
    } else {
        assert(bd >= 8);
        assert((w % 8) == 0);
        assert(mask_type == DIFFWTD_38 || mask_type == DIFFWTD_38_INV);
        const __m128i   x0                        = _mm_setzero_si128();
        const __m128i   x_aom_blend_a64_max_alpha = _mm_set1_epi16(AOM_BLEND_A64_MAX_ALPHA);
        const int       mask_base                 = 38;
        const __m128i   xmask_base                = _mm_set1_epi16(mask_base);
        const uint16_t *ssrc0                     = (uint16_t *)(src0);
        const uint16_t *ssrc1                     = (uint16_t *)(src1);
        if (bd == 8) {
            if (mask_type == DIFFWTD_38_INV) {
                for (int i = 0; i < h; ++i) {
                    for (int j = 0; j < w; j += 8) {
                        __m128i s0   = _mm_loadu_si128((const __m128i *)&ssrc0[j]);
                        __m128i s1   = _mm_loadu_si128((const __m128i *)&ssrc1[j]);
                        __m128i diff = _mm_srai_epi16(_mm_abs_epi16(_mm_sub_epi16(s0, s1)), DIFF_FACTOR_LOG2);
                        __m128i m    = _mm_min_epi16(_mm_max_epi16(x0, _mm_add_epi16(diff, xmask_base)),
                                                  x_aom_blend_a64_max_alpha);
                        m            = _mm_sub_epi16(x_aom_blend_a64_max_alpha, m);
                        m            = _mm_packus_epi16(m, m);
                        _mm_storel_epi64((__m128i *)&mask[j], m);
                    }
                    ssrc0 += src0_stride;
                    ssrc1 += src1_stride;
                    mask += w;
                }
            } else {
                for (int i = 0; i < h; ++i) {
                    for (int j = 0; j < w; j += 8) {
                        __m128i s0   = _mm_loadu_si128((const __m128i *)&ssrc0[j]);
                        __m128i s1   = _mm_loadu_si128((const __m128i *)&ssrc1[j]);
                        __m128i diff = _mm_srai_epi16(_mm_abs_epi16(_mm_sub_epi16(s0, s1)), DIFF_FACTOR_LOG2);
                        __m128i m    = _mm_min_epi16(_mm_max_epi16(x0, _mm_add_epi16(diff, xmask_base)),
                                                  x_aom_blend_a64_max_alpha);
                        m            = _mm_packus_epi16(m, m);
                        _mm_storel_epi64((__m128i *)&mask[j], m);
                    }
                    ssrc0 += src0_stride;
                    ssrc1 += src1_stride;
                    mask += w;
                }
            }
        } else {
            const __m128i xshift = xx_set1_64_from_32i(bd - 8 + DIFF_FACTOR_LOG2);
            if (mask_type == DIFFWTD_38_INV) {
                for (int i = 0; i < h; ++i) {
                    for (int j = 0; j < w; j += 8) {
                        __m128i s0   = _mm_loadu_si128((const __m128i *)&ssrc0[j]);
                        __m128i s1   = _mm_loadu_si128((const __m128i *)&ssrc1[j]);
                        __m128i diff = _mm_sra_epi16(_mm_abs_epi16(_mm_sub_epi16(s0, s1)), xshift);
                        __m128i m    = _mm_min_epi16(_mm_max_epi16(x0, _mm_add_epi16(diff, xmask_base)),
                                                  x_aom_blend_a64_max_alpha);
                        m            = _mm_sub_epi16(x_aom_blend_a64_max_alpha, m);
                        m            = _mm_packus_epi16(m, m);
                        _mm_storel_epi64((__m128i *)&mask[j], m);
                    }
                    ssrc0 += src0_stride;
                    ssrc1 += src1_stride;
                    mask += w;
                }
            } else {
                for (int i = 0; i < h; ++i) {
                    for (int j = 0; j < w; j += 8) {
                        __m128i s0   = _mm_loadu_si128((const __m128i *)&ssrc0[j]);
                        __m128i s1   = _mm_loadu_si128((const __m128i *)&ssrc1[j]);
                        __m128i diff = _mm_sra_epi16(_mm_abs_epi16(_mm_sub_epi16(s0, s1)), xshift);
                        __m128i m    = _mm_min_epi16(_mm_max_epi16(x0, _mm_add_epi16(diff, xmask_base)),
                                                  x_aom_blend_a64_max_alpha);
                        m            = _mm_packus_epi16(m, m);
                        _mm_storel_epi64((__m128i *)&mask[j], m);
                    }
                    ssrc0 += src0_stride;
                    ssrc1 += src1_stride;
                    mask += w;
                }
            }
        }
    }
}
