/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#ifndef MOS_NODE_H
#define MOS_NODE_H
#include <stdint.h>

#include "mos/common.h"

/* Only used as a pointer here; worker.h includes this header, not the other way round. */
struct mlib_worker;

#define MLIB_NODE_MAX_NEXT 32

enum mlib_node_type {
	MLIB_NODE_TYPE_INPUT,
	MLIB_NODE_TYPE_INTERNAL,
};

typedef uint32_t (*mlib_node_fn_t)(struct mlib_worker *worker,
				   mlib_rte_handle_t this_node_rte_handle,
				   mlib_frame_handle_t frame_handle);

struct __attribute__((aligned(MLIB_CACHE_LINE_SIZE))) mlib_node {
	const char *name;
	enum mlib_node_type type;

	const char *nexts[MLIB_NODE_MAX_NEXT];
	mlib_node_fn_t process_frame;
};

_Static_assert(__alignof(struct mlib_node) == MLIB_CACHE_LINE_SIZE,
	       "mlib_node is required to be aligned to cache line");
_Static_assert(sizeof(struct mlib_node) % __alignof(struct mlib_node) == 0,
	       "sizeof mlib_node has to be a multiple of its alignment");

struct mlib_rte {
	mlib_node_handle_t this_node_reg;
	uint64_t counter_calls;
	uint64_t counter_packets;

	uint32_t n_nexts;
	mlib_rte_handle_t nexts[MLIB_NODE_MAX_NEXT];
	mlib_frame_handle_t open_output_frames[MLIB_NODE_MAX_NEXT];
};

#define MLIB_NODE_REGISTER(name) \
	__attribute__((section("mlib_nodes"), __used__)) static const struct mlib_node name##_node

#endif // MOS_NODE_H
