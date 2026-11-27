/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#ifndef MOS_BUFFER_H
#define MOS_BUFFER_H

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#include "mos/common.h"

#define MLIB_BUF_SCRATCH_SIZE  56
#define MLIB_BUF_DATA_SIZE     2048
#define MLIB_BUF_HEADROOM_SIZE 128

struct __attribute__((aligned(MLIB_CACHE_LINE_SIZE))) mlib_buffer {
	uint32_t data_off;
	uint32_t data_len;
	uint8_t scratch[MLIB_BUF_SCRATCH_SIZE];

	/* CACHE_LINE_BOUNDARY */
	uint8_t data[MLIB_BUF_HEADROOM_SIZE + MLIB_BUF_DATA_SIZE];
};

_Static_assert(sizeof(struct mlib_buffer) == 2240,
	       "mlib_buffer size is not expected, something changed?");
_Static_assert(alignof(struct mlib_buffer) == MLIB_CACHE_LINE_SIZE,
	       "mlib_buffer is not aligned to MLIB_CACHE_LINE_SIZE, will thrash "
	       "the cache lines!");
_Static_assert(offsetof(struct mlib_buffer, data) == MLIB_CACHE_LINE_SIZE,
	       "data should start at the cache line boundary i.e "
	       "{MLIB_CACHE_LINE_SIZE}B after the "
	       "metadata");
_Static_assert(((offsetof(struct mlib_buffer, data) + MLIB_BUF_HEADROOM_SIZE) %
		MLIB_CACHE_LINE_SIZE) == 0,
	       "packet start (data + headroom) must be cache-line aligned");

/*
 * Initializes a buffer with default values
 */
static inline void mlib_buffer_init(struct mlib_buffer *buf)
{
	buf->data_off = MLIB_BUF_HEADROOM_SIZE;
	buf->data_len = 0;
}

/*
 * Returns pointer to data at current offset
 */
static inline void *mlib_buffer_get_data(struct mlib_buffer *buf)
{
	return &buf->data[buf->data_off];
}

/*
 * Bring the data_offset by n; returns a pointer to the start of the prepended packet
 * Usage:
 *	struct ip6_hdr *hdr;
 *	hdr = mlib_buffer_prepend_bytes(buf, sizeof(*hdr));
 *	if (!hdr) {
 *		// Not enough space
 *	}
 *
 * NOTE: if called after _strip_bytes(), make sure to overwrite the prepended section completely
 */
void *mlib_buffer_prepend_bytes(struct mlib_buffer *buf, size_t n);

/*
 * Moves the offset ptr forward by n; does not remove data;
 */
int mlib_buffer_strip_bytes(struct mlib_buffer *buf, size_t n);

#endif // MOS_BUFFER_H
