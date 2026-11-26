/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#include "mos/buffer.h"

#include <errno.h>
#include <string.h>

int mos_buffer_strip_bytes(struct mos_buffer *buf, size_t n)
{
	if (!buf || n > buf->data_len)
		return -EINVAL;

	buf->data_off += n;
	buf->data_len -= n;

	return 0;
}

void *mos_buffer_prepend_bytes(struct mos_buffer *buf, size_t n)
{
	if (!buf || n > buf->data_off)
		return NULL;

	buf->data_off -= n;
	buf->data_len += n;

#ifndef NDEBUG
	/*
	 * Poisoning the prepended section to make sure that stale data does not make
	 * tests pass by luck
	 */
	memset(&buf->data[buf->data_off], 0xA5, n);
#endif

	return &buf->data[buf->data_off];
}