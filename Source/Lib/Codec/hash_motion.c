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

#include "hash.h"
#include "hash_motion.h"
#include "pcs.h"

void             svt_aom_free(void *memblk);
static const int crc_bits        = 16;
static const int block_size_bits = 3;

static void hash_table_clear_all(HashTable *p_hash_table) {
    if (p_hash_table->p_lookup_table == NULL)
        return;
    int max_addr = 1 << (crc_bits + block_size_bits);
    for (int i = 0; i < max_addr; i++) {
        if (p_hash_table->p_lookup_table[i] != NULL) {
            svt_aom_vector_destroy(p_hash_table->p_lookup_table[i]);
            free(p_hash_table->p_lookup_table[i]);
            p_hash_table->p_lookup_table[i] = NULL;
        }
    }
}

static void get_pixels_in_1d_char_array_by_block_2x2(uint8_t *y_src, int stride, uint8_t *p_pixels_in1D) {
    uint8_t *p_pel = y_src;
    int      index = 0;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) p_pixels_in1D[index++] = p_pel[j];
        p_pel += stride;
    }
}

static void get_pixels_in_1d_short_array_by_block_2x2(uint16_t *y_src, int stride, uint16_t *p_pixels_in1D) {
    uint16_t *p_pel = y_src;
    int       index = 0;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) p_pixels_in1D[index++] = p_pel[j];
        p_pel += stride;
    }
}

static int is_block_2x2_row_same_value(uint8_t *p) {
    if (p[0] != p[1] || p[2] != p[3])
        return 0;
    return 1;
}

static int is_block16_2x2_row_same_value(uint16_t *p) {
    if (p[0] != p[1] || p[2] != p[3])
        return 0;
    return 1;
}

static int is_block_2x2_col_same_value(uint8_t *p) {
    if ((p[0] != p[2]) || (p[1] != p[3]))
        return 0;
    return 1;
}

static int is_block16_2x2_col_same_value(uint16_t *p) {
    if ((p[0] != p[2]) || (p[1] != p[3]))
        return 0;
    return 1;
}

// the hash value (hash_value1 consists two parts, the first 3 bits relate to
// the block size and the remaining 16 bits are the crc values. This fuction
// is used to get the first 3 bits.
static int hash_block_size_to_index(int block_size) {
    switch (block_size) {
    case 4: return 0;
    case 8: return 1;
    case 16: return 2;
    case 32: return 3;
    case 64: return 4;
    case 128: return 5;
    default: return -1;
    }
}

void svt_av1_hash_table_destroy(HashTable *p_hash_table) {
    hash_table_clear_all(p_hash_table);
    EB_FREE_ARRAY(p_hash_table->p_lookup_table);
    p_hash_table->p_lookup_table = NULL;
}

EbErrorType svt_aom_rtime_alloc_svt_av1_hash_table_create(HashTable *p_hash_table) {
    EbErrorType err_code = EB_ErrorNone;
    ;

    if (p_hash_table->p_lookup_table != NULL) {
        hash_table_clear_all(p_hash_table);
        return err_code;
    }
    const int max_addr = 1 << (crc_bits + block_size_bits);
    //p_hash_table->p_lookup_table = (Vector **)malloc(sizeof(p_hash_table->p_lookup_table[0]) * max_addr);
    EB_CALLOC_ARRAY(p_hash_table->p_lookup_table, max_addr);

    return err_code;
}

static void hash_table_add_to_table(HashTable *p_hash_table, uint32_t hash_value, BlockHash *curr_block_hash) {
    if (p_hash_table->p_lookup_table[hash_value] == NULL) {
        p_hash_table->p_lookup_table[hash_value] = malloc(sizeof(p_hash_table->p_lookup_table[0][0]));
        svt_aom_vector_setup(p_hash_table->p_lookup_table[hash_value], 10, sizeof(curr_block_hash[0]));
        svt_aom_vector_push_back(p_hash_table->p_lookup_table[hash_value], curr_block_hash);
    } else {
        svt_aom_vector_push_back(p_hash_table->p_lookup_table[hash_value], curr_block_hash);
    }
}

int32_t svt_av1_hash_table_count(const HashTable *p_hash_table, uint32_t hash_value) {
    if (p_hash_table->p_lookup_table[hash_value] == NULL) {
        return 0;
    } else
        return (int32_t)(p_hash_table->p_lookup_table[hash_value]->size);
}

Iterator svt_av1_hash_get_first_iterator(HashTable *p_hash_table, uint32_t hash_value) {
    assert(svt_av1_hash_table_count(p_hash_table, hash_value) > 0);
    return svt_aom_vector_begin(p_hash_table->p_lookup_table[hash_value]);
}

void svt_av1_generate_block_2x2_hash_value(const Yv12BufferConfig *picture, uint32_t *pic_block_hash[2],
                                           int8_t *pic_block_same_info[3], PictureControlSet *pcs) {
    const int width  = 2;
    const int height = 2;
    const int x_end  = picture->y_crop_width - width + 1;
    const int y_end  = picture->y_crop_height - height + 1;

    const int length = width * 2;
    if (picture->flags & YV12_FLAG_HIGHBITDEPTH) {
        uint16_t p[4];
        int      pos = 0;
        for (int y_pos = 0; y_pos < y_end; y_pos++) {
            for (int x_pos = 0; x_pos < x_end; x_pos++) {
                get_pixels_in_1d_short_array_by_block_2x2(
                    CONVERT_TO_SHORTPTR(picture->y_buffer) + y_pos * picture->y_stride + x_pos, picture->y_stride, p);
                pic_block_same_info[0][pos] = is_block16_2x2_row_same_value(p);
                pic_block_same_info[1][pos] = is_block16_2x2_col_same_value(p);

                pic_block_hash[0][pos] = svt_av1_get_crc_value(
                    &pcs->crc_calculator1, (uint8_t *)p, length * sizeof(p[0]));
                pic_block_hash[1][pos] = svt_av1_get_crc_value(
                    &pcs->crc_calculator2, (uint8_t *)p, length * sizeof(p[0]));
                pos++;
            }
            pos += width - 1;
        }
    } else {
        uint8_t p[4];
        int     pos = 0;
        for (int y_pos = 0; y_pos < y_end; y_pos++) {
            for (int x_pos = 0; x_pos < x_end; x_pos++) {
                get_pixels_in_1d_char_array_by_block_2x2(
                    picture->y_buffer + y_pos * picture->y_stride + x_pos, picture->y_stride, p);
                pic_block_same_info[0][pos] = is_block_2x2_row_same_value(p);
                pic_block_same_info[1][pos] = is_block_2x2_col_same_value(p);

                pic_block_hash[0][pos] = svt_av1_get_crc_value(&pcs->crc_calculator1, p, length * sizeof(p[0]));
                pic_block_hash[1][pos] = svt_av1_get_crc_value(&pcs->crc_calculator2, p, length * sizeof(p[0]));
                pos++;
            }
            pos += width - 1;
        }
    }
}

void svt_av1_generate_block_hash_value(const Yv12BufferConfig *picture, int block_size, uint32_t *src_pic_block_hash[2],
                                       uint32_t *dst_pic_block_hash[2], int8_t *src_pic_block_same_info[3],
                                       int8_t *dst_pic_block_same_info[3], PictureControlSet *pcs) {
    const int pic_width = picture->y_crop_width;
    const int x_end     = picture->y_crop_width - block_size + 1;
    const int y_end     = picture->y_crop_height - block_size + 1;

    const int src_size  = block_size >> 1;
    const int quad_size = block_size >> 2;

    uint32_t  p[4];
    const int length = sizeof(p);

    int pos = 0;
    for (int y_pos = 0; y_pos < y_end; y_pos++) {
        for (int x_pos = 0; x_pos < x_end; x_pos++) {
            p[0]                       = src_pic_block_hash[0][pos];
            p[1]                       = src_pic_block_hash[0][pos + src_size];
            p[2]                       = src_pic_block_hash[0][pos + src_size * pic_width];
            p[3]                       = src_pic_block_hash[0][pos + src_size * pic_width + src_size];
            dst_pic_block_hash[0][pos] = svt_av1_get_crc_value(&pcs->crc_calculator1, (uint8_t *)p, length);

            p[0]                       = src_pic_block_hash[1][pos];
            p[1]                       = src_pic_block_hash[1][pos + src_size];
            p[2]                       = src_pic_block_hash[1][pos + src_size * pic_width];
            p[3]                       = src_pic_block_hash[1][pos + src_size * pic_width + src_size];
            dst_pic_block_hash[1][pos] = svt_av1_get_crc_value(&pcs->crc_calculator2, (uint8_t *)p, length);

            dst_pic_block_same_info[0][pos] = src_pic_block_same_info[0][pos] &&
                src_pic_block_same_info[0][pos + quad_size] && src_pic_block_same_info[0][pos + src_size] &&
                src_pic_block_same_info[0][pos + src_size * pic_width] &&
                src_pic_block_same_info[0][pos + src_size * pic_width + quad_size] &&
                src_pic_block_same_info[0][pos + src_size * pic_width + src_size];

            dst_pic_block_same_info[1][pos] = src_pic_block_same_info[1][pos] &&
                src_pic_block_same_info[1][pos + src_size] && src_pic_block_same_info[1][pos + quad_size * pic_width] &&
                src_pic_block_same_info[1][pos + quad_size * pic_width + src_size] &&
                src_pic_block_same_info[1][pos + src_size * pic_width] &&
                src_pic_block_same_info[1][pos + src_size * pic_width + src_size];
            pos++;
        }
        pos += block_size - 1;
    }

    if (block_size >= 4) {
        const int size_minus_1 = block_size - 1;
        pos                    = 0;
        for (int y_pos = 0; y_pos < y_end; y_pos++) {
            for (int x_pos = 0; x_pos < x_end; x_pos++) {
                dst_pic_block_same_info[2][pos] = (!dst_pic_block_same_info[0][pos] &&
                                                   !dst_pic_block_same_info[1][pos]) ||
                    (((x_pos & size_minus_1) == 0) && ((y_pos & size_minus_1) == 0));
                pos++;
            }
            pos += block_size - 1;
        }
    }
}

void svt_aom_rtime_alloc_svt_av1_add_to_hash_map_by_row_with_precal_data(HashTable *p_hash_table, uint32_t *pic_hash[2],
                                                                         int8_t *pic_is_same, int pic_width,
                                                                         int pic_height, int block_size) {
    const int x_end = pic_width - block_size + 1;
    const int y_end = pic_height - block_size + 1;

    const int8_t   *src_is_added = pic_is_same;
    const uint32_t *src_hash[2]  = {pic_hash[0], pic_hash[1]};

    int add_value = hash_block_size_to_index(block_size);
    assert(add_value >= 0);
    add_value <<= crc_bits;
    const int crc_mask = (1 << crc_bits) - 1;

    for (int x_pos = 0; x_pos < x_end; x_pos++) {
        for (int y_pos = 0; y_pos < y_end; y_pos++) {
            const int pos = y_pos * pic_width + x_pos;
            // valid data
            if (src_is_added[pos]) {
                BlockHash curr_block_hash;
                curr_block_hash.x = x_pos;
                curr_block_hash.y = y_pos;

                const uint32_t hash_value1  = (src_hash[0][pos] & crc_mask) + add_value;
                curr_block_hash.hash_value2 = src_hash[1][pos];

                hash_table_add_to_table(p_hash_table, hash_value1, &curr_block_hash);
            }
        }
    }
}

void svt_av1_get_block_hash_value(uint8_t *y_src, int stride, int block_size, uint32_t *hash_value1,
                                  uint32_t *hash_value2, int use_highbitdepth, struct PictureControlSet *pcs,
                                  IntraBcContext *x) {
    UNUSED(pcs);
    uint32_t  to_hash[4];
    const int add_value = hash_block_size_to_index(block_size) << crc_bits;
    assert(add_value >= 0);
    const int crc_mask = (1 << crc_bits) - 1;

    // 2x2 subblock hash values in current CU
    int sub_block_in_width = (block_size >> 1);
    if (use_highbitdepth) {
        uint16_t  pixel_to_hash[4];
        uint16_t *y16_src = CONVERT_TO_SHORTPTR(y_src);
        for (int y_pos = 0; y_pos < block_size; y_pos += 2) {
            for (int x_pos = 0; x_pos < block_size; x_pos += 2) {
                int pos = (y_pos >> 1) * sub_block_in_width + (x_pos >> 1);
                get_pixels_in_1d_short_array_by_block_2x2(y16_src + y_pos * stride + x_pos, stride, pixel_to_hash);
                assert(pos < AOM_BUFFER_SIZE_FOR_BLOCK_HASH);
                x->hash_value_buffer[0][0][pos] = svt_av1_get_crc_value(
                    &x->crc_calculator1, (uint8_t *)pixel_to_hash, sizeof(pixel_to_hash));
                x->hash_value_buffer[1][0][pos] = svt_av1_get_crc_value(
                    &x->crc_calculator2, (uint8_t *)pixel_to_hash, sizeof(pixel_to_hash));
            }
        }
    } else {
        uint8_t pixel_to_hash[4];
        for (int y_pos = 0; y_pos < block_size; y_pos += 2) {
            for (int x_pos = 0; x_pos < block_size; x_pos += 2) {
                int pos = (y_pos >> 1) * sub_block_in_width + (x_pos >> 1);
                get_pixels_in_1d_char_array_by_block_2x2(y_src + y_pos * stride + x_pos, stride, pixel_to_hash);
                assert(pos < AOM_BUFFER_SIZE_FOR_BLOCK_HASH);
                x->hash_value_buffer[0][0][pos] = svt_av1_get_crc_value(
                    &x->crc_calculator1, pixel_to_hash, sizeof(pixel_to_hash));
                x->hash_value_buffer[1][0][pos] = svt_av1_get_crc_value(
                    &x->crc_calculator2, pixel_to_hash, sizeof(pixel_to_hash));
            }
        }
    }

    int src_sub_block_in_width = sub_block_in_width;
    sub_block_in_width >>= 1;

    int src_idx = 1;
    int dst_idx = 0;

    // 4x4 subblock hash values to current block hash values
    for (int sub_width = 4; sub_width <= block_size; sub_width *= 2) {
        src_idx = 1 - src_idx;
        dst_idx = 1 - dst_idx;

        int dst_pos = 0;
        for (int y_pos = 0; y_pos < sub_block_in_width; y_pos++) {
            for (int x_pos = 0; x_pos < sub_block_in_width; x_pos++) {
                int src_pos = (y_pos << 1) * src_sub_block_in_width + (x_pos << 1);

                assert(src_pos + 1 < AOM_BUFFER_SIZE_FOR_BLOCK_HASH);
                assert(src_pos + src_sub_block_in_width + 1 < AOM_BUFFER_SIZE_FOR_BLOCK_HASH);
                assert(dst_pos < AOM_BUFFER_SIZE_FOR_BLOCK_HASH);
                to_hash[0] = x->hash_value_buffer[0][src_idx][src_pos];
                to_hash[1] = x->hash_value_buffer[0][src_idx][src_pos + 1];
                to_hash[2] = x->hash_value_buffer[0][src_idx][src_pos + src_sub_block_in_width];
                to_hash[3] = x->hash_value_buffer[0][src_idx][src_pos + src_sub_block_in_width + 1];
                x->hash_value_buffer[0][dst_idx][dst_pos] = svt_av1_get_crc_value(
                    &x->crc_calculator1, (uint8_t *)to_hash, sizeof(to_hash));

                to_hash[0] = x->hash_value_buffer[1][src_idx][src_pos];
                to_hash[1] = x->hash_value_buffer[1][src_idx][src_pos + 1];
                to_hash[2] = x->hash_value_buffer[1][src_idx][src_pos + src_sub_block_in_width];
                to_hash[3] = x->hash_value_buffer[1][src_idx][src_pos + src_sub_block_in_width + 1];
                x->hash_value_buffer[1][dst_idx][dst_pos] = svt_av1_get_crc_value(
                    &x->crc_calculator2, (uint8_t *)to_hash, sizeof(to_hash));
                dst_pos++;
            }
        }

        src_sub_block_in_width = sub_block_in_width;
        sub_block_in_width >>= 1;
    }

    *hash_value1 = (x->hash_value_buffer[0][dst_idx][0] & crc_mask) + add_value;
    *hash_value2 = x->hash_value_buffer[1][dst_idx][0];
}
