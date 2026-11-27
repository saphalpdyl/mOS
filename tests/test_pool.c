/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mos/core/pool.h"

#include "test_common.h"

#define N 8 /* small pool, so states are easy to reason about */

static void pool_setup(struct mlib_pool *pool)
{
	int res = mlib_pool_init(pool, N);

	CHECK(res == 0);
}

/* ---------------------------------------------------------------- init / destroy */

TEST(test_init_rejects_bad_args)
{
	struct mlib_pool pool;

	CHECK(mlib_pool_init(NULL, N) == -EINVAL);

	/* Garbage in the struct, as with an uninitialized local. */
	memset(&pool, 0xAB, sizeof(pool));
	CHECK(mlib_pool_init(&pool, 0) == -EINVAL);

	/* pool.h promises a failed init leaves the pool zeroed (safe to destroy). */
	CHECK(pool.bufs == NULL);
	CHECK(pool.free_idxs == NULL);
	CHECK(pool.n_total == 0);
	CHECK(pool.n_free == 0);
}

TEST(test_init_state)
{
	struct mlib_pool pool;

	pool_setup(&pool);

	CHECK(pool.n_total == N);
	CHECK(pool.n_free == N);
	for (uint32_t i = 0; i < N; i++)
		CHECK(pool.free_idxs[i] == i);

	/* aligned_alloc must really have given us cache-line-aligned memory */
	CHECK((uintptr_t)pool.bufs % MLIB_CACHE_LINE_SIZE == 0);

	mlib_pool_destroy(&pool);
}

TEST(test_destroy_twice_is_safe)
{
	struct mlib_pool pool;

	pool_setup(&pool);
	CHECK(mlib_pool_destroy(&pool) == 0);
	CHECK(pool.bufs == NULL);
	CHECK(pool.free_idxs == NULL);
	CHECK(mlib_pool_destroy(&pool) == 0); /* ASan would flag a double free */
}

/* ---------------------------------------------------------------- alloc */

TEST(test_alloc_is_lifo)
{
	struct mlib_pool pool;

	pool_setup(&pool);

	/* free_idxs = [0..N-1], top is N-1 */
	CHECK(mlib_pool_alloc(&pool) == N - 1);
	CHECK(mlib_pool_alloc(&pool) == N - 2);
	CHECK(pool.n_free == N - 2);

	mlib_pool_destroy(&pool);
}

TEST(test_alloc_until_empty)
{
	struct mlib_pool pool;

	pool_setup(&pool);

	for (int i = 0; i < N; i++)
		CHECK(mlib_pool_alloc(&pool) >= 0);
	CHECK(pool.n_free == 0);

	CHECK(mlib_pool_alloc(&pool) == -ENOMEM);
	CHECK(pool.n_free == 0); /* a failed alloc changes nothing */

	mlib_pool_destroy(&pool);
}

TEST(test_alloc_batch)
{
	struct mlib_pool pool;
	pool_setup(&pool);

	uint32_t *allocated_idx = calloc(MLIB_FRAME_MAX_SIZE, sizeof(uint32_t));
	int allocated_count = mlib_pool_alloc_batch(&pool, allocated_idx, N);

	CHECK(allocated_count == N);
	CHECK(allocated_idx[N - 1] == 0);

	free(allocated_idx);
	CHECK(mlib_pool_destroy(&pool) == 0);
}

TEST(test_alloc_batch_more_than_available)
{
	struct mlib_pool pool;
	pool_setup(&pool);

	uint32_t *allocated_idx = calloc(MLIB_FRAME_MAX_SIZE, sizeof(uint32_t));
	int allocated_count = mlib_pool_alloc_batch(&pool, allocated_idx, N + 5);

	CHECK(allocated_count < N + 5);
	CHECK(allocated_idx[N - 1] == 0);

	free(allocated_idx);
	CHECK(mlib_pool_destroy(&pool) == 0);
}

/* ---------------------------------------------------------------- free */

/*
 * The out-of-sequence case: two buffers go out, the one allocated FIRST comes
 * back first. The stale index left above n_free must not be handed out again
 * while its buffer is still in use.
 */
TEST(test_free_out_of_order)
{
	struct mlib_pool pool;

	pool_setup(&pool);

	int a = mlib_pool_alloc(&pool); /* N-1 */
	int b = mlib_pool_alloc(&pool); /* N-2 */
	CHECK(a == N - 1);
	CHECK(b == N - 2);

	/* a is tx'd before b */
	CHECK(mlib_pool_free(&pool, a) == 0);
	CHECK(pool.n_free == N - 1);

	/* next alloc must give back a, NOT b (b is still in flight) */
	int c = mlib_pool_alloc(&pool);
	CHECK(c == a);
	CHECK(c != b);

	CHECK(mlib_pool_free(&pool, b) == 0);
	CHECK(mlib_pool_free(&pool, c) == 0);
	CHECK(pool.n_free == N);

	mlib_pool_destroy(&pool);
}

/*
 * Free everything in a scrambled order, then allocate everything again:
 * every index must come out exactly once (no duplicates, none lost).
 */
TEST(test_scrambled_free_then_realloc_all)
{
	static const uint32_t order[N] = {5, 0, 7, 2, 6, 1, 4, 3};
	int seen[N] = {0};
	struct mlib_pool pool;

	pool_setup(&pool);

	for (int i = 0; i < N; i++)
		mlib_pool_alloc(&pool);
	for (int i = 0; i < N; i++)
		CHECK(mlib_pool_free(&pool, order[i]) == 0);
	CHECK(pool.n_free == N);

	for (int i = 0; i < N; i++) {
		int idx = mlib_pool_alloc(&pool);

		CHECK(idx >= 0 && idx < N);
		if (idx >= 0 && idx < N)
			seen[idx]++;
	}
	for (int i = 0; i < N; i++)
		CHECK(seen[i] == 1);

	mlib_pool_destroy(&pool);
}

TEST(test_free_into_full_pool_is_rejected)
{
	struct mlib_pool pool;

	pool_setup(&pool);

	/* nothing is allocated, so nothing can be returned */
	CHECK(mlib_pool_free(&pool, 0) != 0);
	CHECK(pool.n_free == N);

	mlib_pool_destroy(&pool);
}

TEST(test_free_invalid_index_is_rejected)
{
	struct mlib_pool pool;

	pool_setup(&pool);
	mlib_pool_alloc(&pool);

	CHECK(mlib_pool_free(&pool, N) != 0); /* one past the end */
	CHECK(mlib_pool_free(&pool, UINT32_MAX) != 0);
	CHECK(pool.n_free == N - 1); /* rejected frees change nothing */

	mlib_pool_destroy(&pool);
}

/* ---------------------------------------------------------------- free batch */

/*
 * Batch-free everything in a scrambled order, then allocate everything again:
 * every index must come out exactly once. Also checks the batch really frees
 * the indices it was given (LIFO: the last one freed is the next one out).
 */
TEST(test_free_batch_scrambled_then_realloc_all)
{
	static const uint32_t order[N] = {5, 0, 7, 2, 6, 1, 4, 3};
	uint32_t idxs[N];
	int seen[N] = {0};
	struct mlib_pool pool;

	pool_setup(&pool);

	CHECK(mlib_pool_alloc_batch(&pool, idxs, N) == N);
	CHECK(pool.n_free == 0);

	memcpy(idxs, order, sizeof(idxs));
	CHECK(mlib_pool_free_batch(&pool, idxs, N) == 0);
	CHECK(pool.n_free == N);

	/* the free stack now holds `order` bottom to top */
	CHECK(pool.free_idxs[N - 1] == order[N - 1]);

	for (int i = 0; i < N; i++) {
		int idx = mlib_pool_alloc(&pool);

		CHECK(idx >= 0 && idx < N);
		if (idx >= 0 && idx < N)
			seen[idx]++;
	}
	for (int i = 0; i < N; i++)
		CHECK(seen[i] == 1);

	mlib_pool_destroy(&pool);
}

/*
 * One bad index in the middle: it is skipped and counted, the valid ones
 * around it are still freed.
 */
TEST(test_free_batch_skips_invalid_index)
{
	uint32_t idxs[3];
	struct mlib_pool pool;

	pool_setup(&pool);

	CHECK(mlib_pool_alloc_batch(&pool, idxs, 3) == 3); /* N-1, N-2, N-3 */
	CHECK(pool.n_free == N - 3);

	idxs[1] = N; /* one past the end */
	CHECK(mlib_pool_free_batch(&pool, idxs, 3) == 1);
	CHECK(pool.n_free == N - 1);

	/* the last valid free is on top */
	CHECK(mlib_pool_alloc(&pool) == (int)idxs[2]);

	mlib_pool_destroy(&pool);
}

/* Nothing is allocated, so every free in the batch must be rejected. */
TEST(test_free_batch_into_full_pool_is_rejected)
{
	uint32_t idxs[3] = {0, 1, 2};
	struct mlib_pool pool;

	pool_setup(&pool);

	CHECK(mlib_pool_free_batch(&pool, idxs, 3) == 3);
	CHECK(pool.n_free == N);

	mlib_pool_destroy(&pool);
}

TEST(test_free_batch_empty_is_noop)
{
	uint32_t idx;
	struct mlib_pool pool;

	pool_setup(&pool);
	mlib_pool_alloc(&pool);

	CHECK(mlib_pool_free_batch(&pool, &idx, 0) == 0);
	CHECK(pool.n_free == N - 1);

	mlib_pool_destroy(&pool);
}

/* ---------------------------------------------------------------- buffer state */

/* A reused buffer must not keep the previous packet's offset/length. */
TEST(test_alloc_resets_buffer)
{
	struct mlib_pool pool;

	pool_setup(&pool);

	int idx = mlib_pool_alloc(&pool);
	CHECK(idx >= 0);
	if (idx < 0)
		goto out;

	/* pretend a packet used it: stripped headers, set a length */
	pool.bufs[idx].data_off = 7;
	pool.bufs[idx].data_len = 99;
	mlib_pool_free(&pool, idx);

	int again = mlib_pool_alloc(&pool);
	CHECK(again == idx); /* LIFO: same buffer comes back */
	CHECK(pool.bufs[again].data_off == MLIB_BUF_HEADROOM_SIZE);
	CHECK(pool.bufs[again].data_len == 0);
out:
	mlib_pool_destroy(&pool);
}
