/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#ifndef MOS_COMMON_H
#define MOS_COMMON_H

#include <stddef.h>

#define MLIB_CACHE_LINE_SIZE 64
#define MLIB_FRAME_MAX_SIZE  256

typedef struct mlib_frame *mlib_frame_handle_t;
typedef struct mlib_node const *mlib_node_handle_t;
typedef struct mlib_rte *mlib_rte_handle_t;

#define MLIB_RTE_HANDLE_NONE   NULL
#define MLIB_FRAME_HANDLE_NONE NULL

#endif // MOS_COMMON_H
