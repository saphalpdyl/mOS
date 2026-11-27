/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */
#include "mos/nodes/worker.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "mos/nodes/frame.h"
#include "mos/nodes/node.h"

extern const struct mlib_node __start_mlib_nodes[]
	__attribute__((weak)); // NOLINT(bugprone-reserved-identifier)
extern const struct mlib_node __stop_mlib_nodes[]
	__attribute__((weak)); // NOLINT(bugprone-reserved-identifier)

/* Linear search; only used at init, so speed doesn't matter */
static mlib_rte_handle_t find_rte_by_name(struct mlib_worker *worker, const char *name)
{
	for (uint32_t i = 0; i < worker->n_rtes; i++) {
		const struct mlib_node *reg = mlib_get_node(worker->rtes[i].this_node_reg);

		if (strcmp(reg->name, name) == 0)
			return mlib_worker_create_rte_handle(&worker->rtes[i]);
	}

	return MLIB_RTE_HANDLE_NONE;
}

int mlib_worker_init(struct mlib_worker *worker)
{
	if (!worker)
		return -EINVAL;

	memset(worker, 0, sizeof(*worker));

	/* Pass 1: one runtime per registered node, built in place in worker->rtes. */
	for (const struct mlib_node *reg = __start_mlib_nodes; reg < __stop_mlib_nodes; reg++) {
		if (!reg->name) {
			fprintf(stderr, "mlib: a registered node has no name\n");
			return -EINVAL;
		}
		if (worker->n_rtes == MLIB_WORKER_MAX_RTES) {
			fprintf(stderr, "mlib: more than %d nodes registered\n",
				MLIB_WORKER_MAX_RTES);
			return -ENOSPC;
		}
		if (reg->n_nexts > MLIB_NODE_MAX_NEXT) {
			fprintf(stderr, "mlib: node '%s': n_nexts %u > max %d\n", reg->name,
				reg->n_nexts, MLIB_NODE_MAX_NEXT);
			return -EINVAL;
		}
		if (find_rte_by_name(worker, reg->name) != MLIB_RTE_HANDLE_NONE) {
			fprintf(stderr, "mlib: node '%s' registered twice\n", reg->name);
			return -EINVAL;
		}

		struct mlib_rte *rte = &worker->rtes[worker->n_rtes];

		rte->this_node_reg = mlib_worker_create_node_handle(reg);
		rte->n_nexts = reg->n_nexts;
		worker->n_rtes++;
	}

	/*
	 * Link pass
	 */
	for (uint32_t i = 0; i < worker->n_rtes; i++) {
		struct mlib_rte *rte = &worker->rtes[i];
		const struct mlib_node *reg = mlib_get_node(rte->this_node_reg);

		for (uint32_t k = 0; k < rte->n_nexts; k++) {
			const char *next_name = reg->nexts[k];

			if (!next_name) {
				fprintf(stderr, "mlib: node '%s': next %u has no name\n", reg->name,
					k);
				return -EINVAL;
			}

			mlib_rte_handle_t next = find_rte_by_name(worker, next_name);

			if (next == MLIB_RTE_HANDLE_NONE) {
				fprintf(stderr, "mlib: node '%s': unknown next '%s'\n", reg->name,
					next_name);
				return -EINVAL;
			}

			rte->nexts[k] = next;
		}
	}

	if (find_rte_by_name(worker, "drop") == MLIB_RTE_HANDLE_NONE) {
		fprintf(stderr, "mlib: no 'drop' node registered\n");
		return -EINVAL;
	}

	return 0;
}

void mlib_worker_free(struct mlib_worker *worker)
{
	if (!worker)
		return;

	for (uint32_t i = 0; i < worker->n_rtes; i++) {
		struct mlib_rte *rte = &worker->rtes[i];

		for (uint32_t k = 0; k < rte->n_nexts; k++) {
			if (rte->open_output_frames[k])
				mlib_frame_free(&rte->open_output_frames[k]);
		}
	}

	struct mlib_pending_entry e;

	while (worker_pending_entries_pop(&worker->pending_entries, &e) == 0)
		mlib_frame_free(&e.frame);

	memset(worker, 0, sizeof(*worker));
}

/*
 * These create/get rte/node handles look weird because they are to be replaced
 * with fixed-allocators in the future. mlib_*_handle_t just defines struct mlib_* * (a pointer)
 * but later will be represented by uint32_t. These function serve as abstractions so that we don't
 * have to change things in a lot of places.
 */
mlib_rte_handle_t mlib_worker_create_rte_handle(struct mlib_rte *rte)
{
	return rte;
}

mlib_node_handle_t mlib_worker_create_node_handle(const struct mlib_node *node)
{
	return node;
}

const struct mlib_rte *mlib_get_rte(mlib_rte_handle_t handle)
{
	return handle;
}
const struct mlib_node *mlib_get_node(mlib_node_handle_t handle)
{
	return handle;
}
