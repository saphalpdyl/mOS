/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#include <stdio.h>

#include "mos/lib/core/pool.h"
#include "mos/lib/graph/frame.h"
#include "mos/lib/graph/node.h"
#include "mos/lib/graph/worker.h"

static uint32_t send_process_frame(struct mlib_worker *, mlib_rte_handle_t, mlib_frame_handle_t);

enum { NODE_NEXT_DROP };

/* Test node: creates custom jargon packets and sends to drop*/
MLIB_NODE_REGISTER(send) = {
	.name = "send",
	.type = MLIB_NODE_TYPE_INPUT,
	.nexts = {[NODE_NEXT_DROP] = "drop"},
	.process_frame = send_process_frame,
};

static uint32_t send_process_frame(struct mlib_worker *worker, mlib_rte_handle_t rte_handle,
				   mlib_frame_handle_t frame_handle)
{
	(void)frame_handle; /* INPUT nodes never have a input frames for e.g. rx*/

	// Create bufs
#define TEST_BUFS_COUNT 290
	uint32_t bufs[TEST_BUFS_COUNT];
	int allocated = mlib_pool_alloc_batch(worker->pool, bufs, TEST_BUFS_COUNT);
	if (allocated != TEST_BUFS_COUNT) {
		fprintf(stderr,
			"batch alloc bufs failed for send, could only create %d bufs instead of "
			"the required %d\n",
			allocated, TEST_BUFS_COUNT);

		if (mlib_pool_free_batch(worker->pool, bufs, allocated) != 0) {
			fprintf(stderr, "failed to batch free buffers on error");
		}
		return 0;
	}

	uint32_t sent = 0;
	for (uint32_t i = 0; i < TEST_BUFS_COUNT; i++) {
		// TODO: per-packet node send is not efficient with the NULL checks and all
		// We will batch the send altogether
		int res = mlib_worker_node_send(worker, rte_handle, NODE_NEXT_DROP, bufs[i]);
		if (res != 0) {
			fprintf(stderr, "node_send failed with %d error", res);
			continue;
		}

		sent++;
	}

	return sent;
}
