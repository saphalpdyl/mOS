/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#include <stdio.h>

#include "mos/lib/core/pool.h"
#include "mos/lib/graph/frame.h"
#include "mos/lib/graph/node.h"
#include "mos/lib/graph/worker.h"

static uint32_t drop_process_frame(struct mlib_worker *, mlib_rte_handle_t, mlib_frame_handle_t);

MLIB_NODE_REGISTER(drop) = {
	.name = "drop",
	.type = MLIB_NODE_TYPE_INTERNAL,
	.process_frame = drop_process_frame,
};

static uint32_t drop_process_frame(struct mlib_worker *worker,
				   mlib_rte_handle_t this_node_rte_handle,
				   mlib_frame_handle_t frame_handle)
{
	(void)this_node_rte_handle;

	const struct mlib_frame *frame = mlib_get_frame(frame_handle);
	int failures = mlib_pool_free_batch(worker->pool, frame->bufs, frame->count);

	if (failures > 0)
		fprintf(stderr, "at least one of pool_free failed: %d failures\n", failures);

	return frame->count;
}
