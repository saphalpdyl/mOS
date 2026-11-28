/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#ifndef MOS_FIFO_H
#define MOS_FIFO_H

#include <errno.h>
#include <stdint.h>

#define MLIB_FIFO_MAX	     256
#define MLIB_FIFO_TYPE(name) struct name##_fifo

/*
 * Type-generic FIFO ring with free-running counters.
 *
 * MLIB_FIFO_DEFINE(name, entry_type, size) must be used at file scope.
 * Usage:
 *	MLIB_FIFO_DEFINE(pending, struct mlib_pending_entry, MLIB_FIFO_MAX)
 *
 *	struct pending_fifo q = {0};	// {0} == empty, no init needed
 *	pending_push(&q, e);
 *	while (pending_pop(&q, &e) == 0)
 *		...
 */
#define MLIB_FIFO_DEFINE(name, entry_type, size)                                                  \
	_Static_assert((size) > 0 && ((size) & ((size) - 1)) == 0,                                \
		       #name " fifo size must be a power of two");                                \
                                                                                                  \
	MLIB_FIFO_TYPE(name)                                                                      \
	{                                                                                         \
		uint32_t head;                                                                    \
		uint32_t tail;                                                                    \
		entry_type q[size];                                                               \
	};                                                                                        \
                                                                                                  \
	static inline __attribute__((used)) uint32_t name##_count(const MLIB_FIFO_TYPE(name) * f) \
	{                                                                                         \
		return f->tail - f->head;                                                         \
	}                                                                                         \
                                                                                                  \
	static inline __attribute__((used)) int name##_push(MLIB_FIFO_TYPE(name) * f,             \
							    entry_type entry)                     \
	{                                                                                         \
		if (name##_count(f) == (size))                                                    \
			return -ENOBUFS;                                                          \
                                                                                                  \
		f->q[f->tail & ((size) - 1)] = entry;                                             \
		f->tail++;                                                                        \
		return 0;                                                                         \
	}                                                                                         \
                                                                                                  \
	static inline __attribute__((used)) int name##_pop(MLIB_FIFO_TYPE(name) * f,              \
							   entry_type * out)                      \
	{                                                                                         \
		if (name##_count(f) == 0)                                                         \
			return -EAGAIN;                                                           \
                                                                                                  \
		*out = f->q[f->head & ((size) - 1)];                                              \
		f->head++;                                                                        \
		return 0;                                                                         \
	}

#endif // MOS_FIFO_H
