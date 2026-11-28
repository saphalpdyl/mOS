/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#include <string.h>

#include "mos/core/pool.h"
#include "mos/nodes/frame.h"
#include "mos/nodes/worker.h"

#include "test_common.h"

TEST(test_graph_run_once_with_send)
{
	struct mlib_pool pool;
	int res = mlib_pool_init(&pool, MLIB_POOL_N);

	CHECK(res == 0);
	CHECK(pool.n_free == MLIB_POOL_N);

	struct mlib_worker worker;
	res = mlib_worker_init(&worker, &pool);

	CHECK(res == 0);

	mlib_worker_run_once(&worker);
	CHECK(worker.n_rtes > 0);

	uint32_t rte_idx = 0;
	for (rte_idx = 0; rte_idx < worker.n_rtes; rte_idx++) {
		struct mlib_rte *rte = &worker.rtes[rte_idx];
		const struct mlib_node *node = mlib_get_node(rte->this_node_reg);

		if (strcmp(node->name, "drop") == 0)
			CHECK(rte->counter_calls == 2);

		if (strcmp(node->name, "send") == 0)
			CHECK(rte->counter_calls == 1);

		CHECK(rte->counter_packets == 290);
	}

	CHECK(pool.n_free == MLIB_POOL_N);
	CHECK(worker_pending_entries_count(&worker.pending_entries) == 0);

	mlib_worker_free(&worker);
	mlib_pool_destroy(&pool);
}