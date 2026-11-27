/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#ifndef MOS_WORKER_H
#define MOS_WORKER_H

#include "mos/common.h"
#include "mos/core/fifo.h"
#include "mos/nodes/node.h"

#define MLIB_WORKER_MAX_RTES 256

struct mlib_pending_entry {
	mlib_frame_handle_t frame;
	mlib_rte_handle_t node;
};

MLIB_FIFO_DEFINE(worker_pending_entries, struct mlib_pending_entry, MLIB_FIFO_MAX);

struct mlib_worker {
	struct worker_pending_entries_fifo pending_entries;

	struct mlib_rte rtes[MLIB_WORKER_MAX_RTES];
	uint32_t n_rtes;
};

int mlib_worker_init(struct mlib_worker *worker);

const struct mlib_rte *mlib_get_rte(mlib_rte_handle_t handle);
const struct mlib_node *mlib_get_node(mlib_node_handle_t handle);
mlib_rte_handle_t mlib_worker_create_rte_handle(struct mlib_rte *rte);
mlib_node_handle_t mlib_worker_create_node_handle(const struct mlib_node *node);

void mlib_worker_free(struct mlib_worker *worker);

#endif // MOS_WORKER_H