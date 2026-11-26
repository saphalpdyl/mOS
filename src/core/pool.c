/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#include "mos/pool.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

int mos_pool_init(struct mos_pool *pool, uint32_t n_total)
{

	if (!pool)
		return -EINVAL;

	memset(pool, 0, sizeof(*pool));

	if (n_total == 0)
		return -EINVAL;

	pool->free_idxs = malloc(n_total * sizeof(*pool->free_idxs));
	if (!pool->free_idxs) {
		return -ENOMEM;
	}

	/* sizeof(struct mos_buffer) is a multiple of 64, as aligned_alloc requires. */
	pool->bufs = aligned_alloc(MOS_CACHE_LINE_SIZE, n_total * sizeof(*pool->bufs));
	if (!pool->bufs) {
		free(pool->free_idxs);
		pool->free_idxs = NULL;
		return -ENOMEM;
	}

	for (uint32_t i = 0; i < n_total; ++i)
		pool->free_idxs[i] = i;

	/*
	 * Pre-fault: touch every page now so the page faults happen at init,
	 * not while forwarding.
	 */
	memset(pool->bufs, 0, n_total * sizeof(*pool->bufs));

	pool->n_total = n_total;
	pool->n_free = n_total;

	return 0;
}

int mos_pool_destroy(struct mos_pool *pool)
{
	if (!pool)
		return -EINVAL;

	free(pool->free_idxs);
	free(pool->bufs);

	memset(pool, 0, sizeof(*pool));

	return 0;
}

int mos_pool_malloc(struct mos_pool *pool)
{
	if (!pool)
		return -EINVAL;

	if (pool->n_free == 0)
		return -ENOMEM;

	int idx = (int)pool->free_idxs[pool->n_free - 1];

	mos_buffer_init(&pool->bufs[idx]);

	pool->n_free--;

	return idx;
}

int mos_pool_free(struct mos_pool *pool, uint32_t idx)
{
	if (!pool)
		return -EINVAL;

	if (pool->n_free == pool->n_total)
		return -EINVAL;

	if (idx >= pool->n_total)
		return -EINVAL;

	pool->free_idxs[pool->n_free] = idx;
	pool->n_free++;

	return 0;
}