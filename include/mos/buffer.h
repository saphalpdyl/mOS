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

#define MOS_BUF_SCRATCH_SIZE  56
#define MOS_BUF_DATA_SIZE     2048
#define MOS_BUF_HEADROOM_SIZE 128

struct __attribute__((aligned(MOS_CACHE_LINE_SIZE))) mos_buffer {
	uint32_t data_off;
	uint32_t data_len;
	uint8_t scratch[MOS_BUF_SCRATCH_SIZE];

	/* CACHE_LINE_BOUNDARY */
	uint8_t data[MOS_BUF_HEADROOM_SIZE + MOS_BUF_DATA_SIZE];
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
_Static_assert(((offsetof(struct mos_buffer, data) + MOS_BUF_HEADROOM_SIZE) %
		MOS_CACHE_LINE_SIZE) == 0,
	       "packet start (data + headroom) must be cache-line aligned");

/*
 * Initializes a buffer with default values
 */
static inline void mos_buffer_init(struct mos_buffer *buf)
{
	buf->data_off = MOS_BUF_HEADROOM_SIZE;
	buf->data_len = 0;
}

/*
 * Returns pointer to data at current offset
 */
static inline void *mos_buffer_get_data(struct mos_buffer *buf)
{
	return &buf->data[buf->data_off];
}

/*
 * Bring the data_offset by n; returns a pointer to the start of the prepended packet
 * Usage:
 *	struct ip6_hdr *hdr;
 *	hdr = mos_buffer_prepend_bytes(buf, sizeof(*hdr));
 *	if (!hdr) {
 *		// Not enough space
 *	}
 *
 * NOTE: if called after _strip_bytes(), make sure to overwrite the prepended section completely
 */
void *mos_buffer_prepend_bytes(struct mos_buffer *buf, size_t n);

/*
 * Moves the offset ptr forward by n; does not remove data;
 */
int mos_buffer_strip_bytes(struct mos_buffer *buf, size_t n);

#endif // MOS_BUFFER_H
