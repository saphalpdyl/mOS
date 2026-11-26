/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#include <stdio.h>

#include "test_common.h"
#include "tests.h"

int main(void)
{
	/* pool */
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

	/* buffer */
	RUN(test_buffer_strip);
	RUN(test_buffer_strip_invalid);
	RUN(test_buffer_prepend);
	RUN(test_buffer_prepend_invalid);
	RUN(test_buffer_prepend_then_strip);
	RUN(test_buffer_strip_then_prepend);

	if (failures)
		fprintf(stderr, "\n%d check(s) failed\n", failures);
	return failures ? 1 : 0;
}
