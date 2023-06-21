/*
* Copyright(c) 2020 Intel Corporation
*
* This source code is subject to the terms of the BSD 2 Clause License and
* the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
* was not distributed with this source code in the LICENSE file, you can
* obtain it at https://www.aomedia.org/license/software-license. If the Alliance for Open
* Media Patent License 1.0 was not distributed with this source code in the
* PATENTS file, you can obtain it at https://www.aomedia.org/license/patent-license.
*/

/*
* This file contains only debug macros that are used during the development
* and are supposed to be cleaned up every tag cycle
* all macros must have the following format:
* - adding a new feature should be prefixed by FTR_
* - tuning a feature should be prefixed by TUNE_
* - enabling a feature should be prefixed by EN_
* - disabling a feature should be prefixed by DIS_
* - bug fixes should be prefixed by FIX_
* - code refactors should be prefixed by RFCTR_
* - code cleanups should be prefixed by CLN_
* - optimizations should be prefixed by OPT_
* - all macros must have a coherent comment explaining what the MACRO is doing
* - #if 0 / #if 1 are not to be used
*/

#ifndef EbDebugMacros_h
#define EbDebugMacros_h

// clang-format off

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define TUNE_SSIM                   1
#if TUNE_SSIM
#define TUNE_SSIM_LIBAOM_APPROACH   1  // 16x16 block variance based lambda adjustment, ported from libaom
#define TUNE_SSIM_FULL_SPACIAL_DIST 1  // replace SSD with SSIM based distortion functions
#else
#define TUNE_SSIM_LIBAOM_APPROACH   0
#define TUNE_SSIM_FULL_SPACIAL_DIST 0
#endif

#define OPT_INTRA_M5                              1 // Use TPL info in M5 intra level selection
#define FIX_TPL_LVLS                              1 // Fix TPL behaviour when certains levels are used
#define OPT_NEW_TPL_LVL                           1 // New TPL/intra levels for M4/M5
#define OPT_TPL_LAY1_5L                           1 // Enable QPS/QPM for layer 1 in 5L and for layer 2 in 6L
#define OPT_DEPTH_LVLS                            1 // Optimize depth refinement
#define CLN_TPL_FUNCS                             1 // Align certain TPL-related functions with libaom versions

#define OPT_P_CPLX                                1 // Optimize nsq parent complexity
#define OPT_RPS_40m30                             1 // Optimize rps list. remove ref 30 for pic 40. //applicable for all 4:3 presets
#define OPT_RPS_ADD                               1 // Optimize rps list. add more pictures on top of 4:3 case

#define FIX_NSQ_LVL22                             1 // Fix NSQ lvl22
#define OPT_HV_NON_HV                             1 // Apply an offset to non_HV_split_rate_th
#define OPT_MRP                                   1 // Preset tuning for MRP

#define TF_PACKAGE                                1 // tf package
#if TF_PACKAGE
#define MCTF_FIX_BUILD                            1 // Fix the build of the MCTF pcs queue
#define SHUT_MCTF_PEN                             1 // Remove the bias towards MV (0,0) for HME of MCTF
#define MCTF_FULL_SAD                             1 // FULL_SAD @ HME/ME of MCTF
#define FIX_LINUX_MISMATCH                        1 // Fix a linux vs windows mimatch
#define FIX_SCENE_TRANSITION                      1 // Improve the scene-transition
#define MCTF_OPT_REFS_MODULATION                  1 // Use the filtered-to-unfiltered distortion divided by the noise-level for the ref-frame(s) modulation instead of only filtered-to-unfiltered
#define MCTF_ON_THE_FLY_PRUNING                   1 // Use ahd-error to central/avg to identify/skip outlier ref-frame(s)
#define MCTF_OPT_HME_LEVEL                        1 // Increase the HME-Level0 when a high active region is detected

#endif

#define OPT_ENABLE_2L_INCOMP                      1 // Enable 2L RA pred structure for incomplete MGs
#define OPT_BEST_REF                              1 // Optimize use  best me references for mrp

//FOR DEBUGGING - Do not remove
#define OPT_LD_LATENCY2         0 // Latency optimization for low delay
#define LOG_ENC_DONE            0 // log encoder job one
#define NO_ENCDEC               0 // bypass encDec to test cmpliance of MD. complained achieved when skip_flag is OFF. Port sample code from VCI-SW_AV1_Candidate1 branch
#define DEBUG_TPL               0 // Prints to debug TPL
#define DETAILED_FRAME_OUTPUT   0 // Prints detailed frame output from the library for debugging
#define TUNE_CHROMA_SSIM        0 // Allows for Chroma and SSIM BDR-based Tuning
#define TUNE_CQP_CHROMA_SSIM    0 // Tune CQP qp scaling towards improved chroma and SSIM BDR

#define MIN_PIC_PARALLELIZATION 0 // Use the minimum amount of picture parallelization
#define SRM_REPORT              0 // Report SRM status
#define LAD_MG_PRINT            0 // Report LAD
#define RC_NO_R2R               0 // This is a debugging flag for RC and makes encoder to run with no R2R in RC mode
                                  // Note that the speed might impacted significantly
#if RC_NO_R2R
#define REMOVE_LP1_LPN_DIFF     1 // Disallow single-thread/multi-thread differences
#else
#define REMOVE_LP1_LPN_DIFF     0 // Disallow single-thread/multi-thread differences
#endif
// Super-resolution debugging code
#define DEBUG_SCALING           0
#define DEBUG_TF                0
#define DEBUG_UPSCALING         0
#define DEBUG_SUPERRES_RECODE   0
#define DEBUG_SUPERRES_ENERGY   0
#define DEBUG_RC_CAP_LOG        0 // Prints for RC cap

// Switch frame debugging code
#define DEBUG_SFRAME            0
// Quantization matrices
#define DEBUG_QM_LEVEL          0
#define DEBUG_STARTUP_MG_SIZE   0
#define DEBUG_SEGMENT_QP        0
#define DEBUG_ROI               0
#ifdef __cplusplus
}
#endif // __cplusplus

// clang-format on

#endif // EbDebugMacros_h
