/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#ifndef MOS_FRAME_H
#define MOS_FRAME_H

#include <stddef.h>
#include <stdint.h>

#include "mos/common.h"

struct mlib_frame {
	uint32_t count;
	// Leaving 60B hole for metadata in the future

	__attribute__((aligned(MLIB_CACHE_LINE_SIZE))) uint32_t bufs[MLIB_FRAME_MAX_SIZE];
};

_Static_assert((offsetof(struct mlib_frame, bufs) % MLIB_CACHE_LINE_SIZE) == 0,
	       "bufs array must start and fit in a cache line");
_Static_assert(offsetof(struct mlib_frame, bufs) == MLIB_CACHE_LINE_SIZE,
	       "bufs array must start in the second cache line");

/* 0 / -EINVAL (frame is NULL) / -ENOMEM. A new frame has count == 0. */
int mlib_frame_alloc(struct mlib_frame **frame);
int mlib_frame_free(struct mlib_frame **frame);

#endif // MOS_FRAME_H
