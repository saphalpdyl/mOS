/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#ifndef MOS_POOL_H
#define MOS_POOL_H

#include "mos/buffer.h"
#include "mos/common.h"

#define MOS_POOL_N 16384

struct __attribute__((aligned(MOS_CACHE_LINE_SIZE))) mos_pool {
	struct mos_buffer *bufs;
	uint32_t *free_idxs;
	uint32_t n_total;
	uint32_t n_free;
};

/*
 * Allocate n_total buffers and an all-free index stack into a caller-owned
 * pool. Returns 0, -EINVAL (bad args) or -ENOMEM. On failure the pool is left
 * zeroed, so destroying it is safe.
 */
int mos_pool_init(struct mos_pool *pool, uint32_t n_total);

/*
 * Free all buffers in the pool and clean up
 */
int mos_pool_destroy(struct mos_pool *pool);

/*
 * Allocate a mos_buffer; returns an idx >= 0 if success, otherwise failure
 */
int mos_pool_malloc(struct mos_pool *pool);

/*
 * Free a mos_buffer; returns 0 if success, otherwise failure
 */
int mos_pool_free(struct mos_pool *pool, uint32_t idx);

#endif // MOS_POOL_H
