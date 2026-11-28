/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */
#include "mos/lib/graph/worker.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "mos/lib/graph/frame.h"
#include "mos/lib/graph/node.h"

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

int mlib_worker_init(struct mlib_worker *worker, struct mlib_pool *pool)
{
	if (!worker)
		return -EINVAL;

	memset(worker, 0, sizeof(*worker));

	worker->pool = pool;

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

		if (!reg->process_frame) {
			fprintf(stderr,
				"mlib: node is missing process_frame; MUST have process_frame");
			return -EINVAL;
		}

		uint32_t n_nexts;
		for (n_nexts = 0; n_nexts < MLIB_NODE_MAX_NEXT; n_nexts++) {
			if (reg->nexts[n_nexts] == NULL)
				break;
		}

		for (uint32_t i = n_nexts; i < MLIB_NODE_MAX_NEXT; i++) {
			if (reg->nexts[i] != NULL) {
				fprintf(stderr, "INVALID REGISTRATION: nexts has NULL gaps, all "
						"nexts should be an arr without NULL gaps");
				return -EINVAL;
			}
		}

		if (find_rte_by_name(worker, reg->name) != MLIB_RTE_HANDLE_NONE) {
			fprintf(stderr, "mlib: node '%s' registered twice\n", reg->name);
			return -EINVAL;
		}

		struct mlib_rte *rte = &worker->rtes[worker->n_rtes];

		rte->this_node_reg = mlib_worker_create_node_handle(reg);
		rte->n_nexts = n_nexts;
		worker->n_rtes++;
	}

	/*
	 * Link pass
	 */
	for (uint32_t i = 0; i < worker->n_rtes; i++) {
		struct mlib_rte *rte = &worker->rtes[i];
		const struct mlib_node *reg = mlib_get_node(rte->this_node_reg);

		uint32_t k = 0;
		for (k = 0; k < rte->n_nexts; k++) {
			const char *next_name = reg->nexts[k];

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

int mlib_worker_node_send(struct mlib_worker *worker, mlib_rte_handle_t rte_handle,
			  uint32_t next_index, uint32_t buf_idx)
{

	struct mlib_rte *this_rte = mlib_get_rte(rte_handle);

	if (next_index >= this_rte->n_nexts)
		return -EINVAL;

	mlib_frame_handle_t frame_handle = this_rte->open_output_frames[next_index];
	struct mlib_frame *frame = mlib_get_frame(frame_handle);
	if (!frame) {
		int res = mlib_frame_alloc(&this_rte->open_output_frames[next_index]);
		frame = this_rte->open_output_frames[next_index];
		frame_handle = mlib_worker_create_frame_handle(frame);
		if (res != 0) {
			fprintf(stderr, "failed to allocate frame: err = %d", res);
			return res;
		}
	}

	// This step should never really happen, since the previous send should push the frame
	// to FIFO queue. A guard nonetheless.
	if (frame->count == MLIB_FRAME_MAX_SIZE) {
		return -ENOBUFS;
	}

	frame->bufs[frame->count] = buf_idx;
	frame->count++;

	if (frame->count == MLIB_FRAME_MAX_SIZE) {
		struct mlib_pending_entry entry = {
			.frame = frame_handle,
			.node = this_rte->nexts[next_index],
		};

		int res = worker_pending_entries_push(&worker->pending_entries, entry);
		if (res != 0) {
			fprintf(stderr, "failed to push entries error: err = %d", res);
			return res;
		}

		this_rte->open_output_frames[next_index] = NULL;
	}

	return 0;
}

int mlib_worker_flush_rte_frames(struct mlib_worker *worker, mlib_rte_handle_t rte_handle)
{
	struct mlib_rte *rte = mlib_get_rte(rte_handle);
	if (!rte)
		return -EINVAL;

	for (uint32_t i = 0; i < rte->n_nexts; i++) {
		mlib_frame_handle_t frame_handle = rte->open_output_frames[i];
		struct mlib_frame *frame = mlib_get_frame(frame_handle);

		if (!frame) {
			continue;
		}

		struct mlib_pending_entry entry = {
			.frame = frame_handle,
			.node = rte->nexts[i],
		};

		int res = worker_pending_entries_push(&worker->pending_entries, entry);
		if (res != 0) {
			fprintf(stderr, "failed to push entries error: err = %d", res);
			return res;
		}

		// Clear frame pointer from the local context of the rte/node
		rte->open_output_frames[i] = NULL;
	}

	return 0;
}

int mlib_worker_run_once(struct mlib_worker *worker)
{
	uint32_t rte_idx;
	for (rte_idx = 0; rte_idx < worker->n_rtes; rte_idx++) {
		struct mlib_rte *rte = &worker->rtes[rte_idx];
		if (!rte)
			continue;
		mlib_rte_handle_t rte_handle = mlib_worker_create_rte_handle(rte);

		const struct mlib_node *node = mlib_get_node(rte->this_node_reg);
		if (node->type != MLIB_NODE_TYPE_INPUT) {
			continue;
		}

		uint32_t processed_packets =
			node->process_frame(worker, rte_handle, MLIB_FRAME_HANDLE_NONE);
		rte->counter_packets += processed_packets;
		rte->counter_calls++;

		mlib_worker_flush_rte_frames(worker, rte_handle);
	}

	while (worker_pending_entries_count(&worker->pending_entries) != 0) {
		struct mlib_pending_entry entry;
		if (worker_pending_entries_pop(&worker->pending_entries, &entry) != 0) {
			fprintf(stderr, "failed to pop from pending entries");
			break;
		}

		struct mlib_rte *rte = mlib_get_rte(entry.node);
		if (!rte)
			continue;
		const struct mlib_node *node = mlib_get_node(rte->this_node_reg);

		uint32_t processed_packets = node->process_frame(worker, entry.node, entry.frame);
		rte->counter_packets += processed_packets;
		rte->counter_calls++;

		if (mlib_worker_flush_rte_frames(worker, entry.node) != 0) {
			fprintf(stderr, "failed to flush rte frames");
		}

		struct mlib_frame *frame = mlib_get_frame(entry.frame);
		mlib_frame_free(&frame);
	}

	return 0;
}

/*
 * These create/get rte/node handles look weird because they are to be replaced
 * with fixed-allocators in the future. mlib_*_handle_t just defines struct mlib_* * (a
 * pointer) but later will be represented by uint32_t. These function serve as abstractions
 * so that we don't have to change things in a lot of places.
 */
mlib_rte_handle_t mlib_worker_create_rte_handle(struct mlib_rte *rte)
{
	return rte;
}

mlib_node_handle_t mlib_worker_create_node_handle(const struct mlib_node *node)
{
	return node;
}

mlib_frame_handle_t mlib_worker_create_frame_handle(struct mlib_frame *frame)
{
	return frame;
}

struct mlib_rte *mlib_get_rte(mlib_rte_handle_t handle)
{
	return handle;
}
const struct mlib_node *mlib_get_node(mlib_node_handle_t handle)
{
	return handle;
}

struct mlib_frame *mlib_get_frame(mlib_frame_handle_t handle)
{
	return handle;
}
