/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mos/pool.h"

static int failures;

#define CHECK(cond)                                                                          \
	do {                                                                                 \
		if (!(cond)) {                                                               \
			fprintf(stderr, "%s:%d: %s: CHECK failed: %s\n", __FILE__, __LINE__, \
				__func__, #cond);                                            \
			failures++;                                                          \
		}                                                                            \
	} while (0)

#define RUN(test)                                                                  \
	do {                                                                       \
		int before = failures;                                             \
		test();                                                            \
		printf("%-40s %s\n", #test, failures == before ? "ok" : "FAILED"); \
	} while (0)

#define N 8 /* small pool, so states are easy to reason about */

static void pool_setup(struct mos_pool *pool)
{
	int res = mos_pool_init(pool, N);

	CHECK(res == 0);
}

/* ---------------------------------------------------------------- init / destroy */

static void test_init_rejects_bad_args(void)
{
	struct mos_pool pool;

	CHECK(mos_pool_init(NULL, N) == -EINVAL);

	/* Garbage in the struct, as with an uninitialized local. */
	memset(&pool, 0xAB, sizeof(pool));
	CHECK(mos_pool_init(&pool, 0) == -EINVAL);

	/* pool.h promises a failed init leaves the pool zeroed (safe to destroy). */
	CHECK(pool.bufs == NULL);
	CHECK(pool.free_idxs == NULL);
	CHECK(pool.n_total == 0);
	CHECK(pool.n_free == 0);
}

static void test_init_state(void)
{
	struct mos_pool pool;

	pool_setup(&pool);

	CHECK(pool.n_total == N);
	CHECK(pool.n_free == N);
	for (uint32_t i = 0; i < N; i++)
		CHECK(pool.free_idxs[i] == i);

	/* aligned_alloc must really have given us cache-line-aligned memory */
	CHECK((uintptr_t)pool.bufs % MOS_CACHE_LINE_SIZE == 0);

	mos_pool_destroy(&pool);
}

static void test_destroy_twice_is_safe(void)
{
	struct mos_pool pool;

	pool_setup(&pool);
	CHECK(mos_pool_destroy(&pool) == 0);
	CHECK(pool.bufs == NULL);
	CHECK(pool.free_idxs == NULL);
	CHECK(mos_pool_destroy(&pool) == 0); /* ASan would flag a double free */
}

/* ---------------------------------------------------------------- alloc */

static void test_alloc_is_lifo(void)
{
	struct mos_pool pool;

	pool_setup(&pool);

	/* free_idxs = [0..N-1], top is N-1 */
	CHECK(mos_pool_malloc(&pool) == N - 1);
	CHECK(mos_pool_malloc(&pool) == N - 2);
	CHECK(pool.n_free == N - 2);

	mos_pool_destroy(&pool);
}

static void test_alloc_until_empty(void)
{
	struct mos_pool pool;

	pool_setup(&pool);

	for (int i = 0; i < N; i++)
		CHECK(mos_pool_malloc(&pool) >= 0);
	CHECK(pool.n_free == 0);

	CHECK(mos_pool_malloc(&pool) == -ENOMEM);
	CHECK(pool.n_free == 0); /* a failed alloc changes nothing */

	mos_pool_destroy(&pool);
}

/* ---------------------------------------------------------------- free */

/*
 * The out-of-sequence case: two buffers go out, the one allocated FIRST comes
 * back first. The stale index left above n_free must not be handed out again
 * while its buffer is still in use.
 */
static void test_free_out_of_order(void)
{
	struct mos_pool pool;

	pool_setup(&pool);

	int a = mos_pool_malloc(&pool); /* N-1 */
	int b = mos_pool_malloc(&pool); /* N-2 */
	CHECK(a == N - 1);
	CHECK(b == N - 2);

	/* a is tx'd before b */
	CHECK(mos_pool_free(&pool, a) == 0);
	CHECK(pool.n_free == N - 1);

	/* next alloc must give back a, NOT b (b is still in flight) */
	int c = mos_pool_malloc(&pool);
	CHECK(c == a);
	CHECK(c != b);

	CHECK(mos_pool_free(&pool, b) == 0);
	CHECK(mos_pool_free(&pool, c) == 0);
	CHECK(pool.n_free == N);

	mos_pool_destroy(&pool);
}

/*
 * Free everything in a scrambled order, then allocate everything again:
 * every index must come out exactly once (no duplicates, none lost).
 */
static void test_scrambled_free_then_realloc_all(void)
{
	static const uint32_t order[N] = {5, 0, 7, 2, 6, 1, 4, 3};
	int seen[N] = {0};
	struct mos_pool pool;

	pool_setup(&pool);

	for (int i = 0; i < N; i++)
		mos_pool_malloc(&pool);
	for (int i = 0; i < N; i++)
		CHECK(mos_pool_free(&pool, order[i]) == 0);
	CHECK(pool.n_free == N);

	for (int i = 0; i < N; i++) {
		int idx = mos_pool_malloc(&pool);

		CHECK(idx >= 0 && idx < N);
		if (idx >= 0 && idx < N)
			seen[idx]++;
	}
	for (int i = 0; i < N; i++)
		CHECK(seen[i] == 1);

	mos_pool_destroy(&pool);
}

static void test_free_into_full_pool_is_rejected(void)
{
	struct mos_pool pool;

	pool_setup(&pool);

	/* nothing is allocated, so nothing can be returned */
	CHECK(mos_pool_free(&pool, 0) != 0);
	CHECK(pool.n_free == N);

	mos_pool_destroy(&pool);
}

static void test_free_invalid_index_is_rejected(void)
{
	struct mos_pool pool;

	pool_setup(&pool);
	mos_pool_malloc(&pool);

	CHECK(mos_pool_free(&pool, N) != 0); /* one past the end */
	CHECK(mos_pool_free(&pool, UINT32_MAX) != 0);
	CHECK(pool.n_free == N - 1); /* rejected frees change nothing */

	mos_pool_destroy(&pool);
}

/* ---------------------------------------------------------------- buffer state */

/* A reused buffer must not keep the previous packet's offset/length. */
static void test_alloc_resets_buffer(void)
{
	struct mos_pool pool;

	pool_setup(&pool);

	int idx = mos_pool_malloc(&pool);
	CHECK(idx >= 0);
	if (idx < 0)
		goto out;

	/* pretend a packet used it: stripped headers, set a length */
	pool.bufs[idx].data_off = 7;
	pool.bufs[idx].data_len = 99;
	mos_pool_free(&pool, idx);

	int again = mos_pool_malloc(&pool);
	CHECK(again == idx); /* LIFO: same buffer comes back */
	CHECK(pool.bufs[again].data_off == MOS_BUF_HEADROOM_SIZE);
	CHECK(pool.bufs[again].data_len == 0);
out:
	mos_pool_destroy(&pool);
}

int main(void)
{
	RUN(test_init_rejects_bad_args);
	RUN(test_init_state);
	RUN(test_destroy_twice_is_safe);
	RUN(test_alloc_is_lifo);
	RUN(test_alloc_until_empty);
	RUN(test_free_out_of_order);
	RUN(test_scrambled_free_then_realloc_all);
	RUN(test_free_into_full_pool_is_rejected);
	RUN(test_free_invalid_index_is_rejected);
	RUN(test_alloc_resets_buffer);

	if (failures)
		fprintf(stderr, "\n%d check(s) failed\n", failures);
	return failures ? 1 : 0;
}
