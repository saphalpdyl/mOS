/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#ifndef MOS_POOL_H
#define MOS_POOL_H

#include "mos/common.h"

#include "mos/lib/core/buffer.h"

#define MLIB_POOL_N 16384

struct __attribute__((aligned(MLIB_CACHE_LINE_SIZE))) mlib_pool {
	struct mlib_buffer *bufs;
	uint32_t *free_idxs;
	uint32_t n_total;
	uint32_t n_free;
};

/*
 * Allocate n_total buffers and an all-free index stack into a caller-owned
 * pool. Returns 0, -EINVAL (bad args) or -ENOMEM. On failure the pool is left
 * zeroed, so destroying it is safe.
 */
int mlib_pool_init(struct mlib_pool *pool, uint32_t n_total);

/*
 * Free all buffers in the pool and clean up
 */
int mlib_pool_destroy(struct mlib_pool *pool);

/*
 * Allocate a mlib_buffer; returns an idx >= 0 if success, otherwise failure
 */
int mlib_pool_alloc(struct mlib_pool *pool);

/*
 * Free a mlib_buffer; returns 0 if success, otherwise failure
 */
int mlib_pool_free(struct mlib_pool *pool, uint32_t idx);

/*
 * Allocates a batch of {count} mlib_buffers in mlib_pool;
 * Returns the number of buffers allocated
 */
int mlib_pool_alloc_batch(struct mlib_pool *pool, uint32_t *idx_buf, size_t count);

/*
 * Frees a batch of {count} mlib_buffers in mlib_pool;
 * Returns the number of failures;
 * Usage:
 *	int failures = mlib_pool_free_batch(pool, (uint32_t*)to_free_idxs, count);
 * 	if (failures != 0)
 * 		// HANDLE ERROR
 */
int mlib_pool_free_batch(struct mlib_pool *pool, const uint32_t *idx_buf, size_t count);

#endif // MOS_POOL_H
