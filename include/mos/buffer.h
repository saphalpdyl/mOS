/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#ifndef MOS_BUFFER_H
#define MOS_BUFFER_H

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#define MOS_BUF_MTD_SIZE_BYTES	    56
#define MOS_BUF_SIZE_BYTES	    2048
#define MOS_BUF_HEADROOM_SIZE_BYTES 128
#define MOS_CACHE_LINE_SIZE	    64

struct __attribute__((aligned(MOS_CACHE_LINE_SIZE))) mos_buffer {
	uint32_t curr_doffset;
	uint32_t curr_len;
	uint8_t scratch[MOS_BUF_MTD_SIZE_BYTES];

	/* CACHE_LINE_BOUNDARY */
	uint8_t data[MOS_BUF_HEADROOM_SIZE_BYTES + MOS_BUF_SIZE_BYTES];
};

_Static_assert(sizeof(struct mos_buffer) == 2240,
	       "mos_buffer size is not expected, something changed?");
_Static_assert(alignof(struct mos_buffer) == MOS_CACHE_LINE_SIZE,
	       "mos_buffer is not aligned to MOS_CACHE_LINE_SIZE, will thrash "
	       "the cache lines!");
_Static_assert(offsetof(struct mos_buffer, data) == MOS_CACHE_LINE_SIZE,
	       "data should start at the cache line boundary i.e "
	       "{MOS_CACHE_LINE_SIZE}B after the "
	       "metadata");
_Static_assert(((offsetof(struct mos_buffer, data) + MOS_BUF_HEADROOM_SIZE_BYTES) %
		MOS_CACHE_LINE_SIZE) == 0,
	       "packet start (data + headroom) must be cache-line aligned");

#endif // MOS_BUFFER_H
