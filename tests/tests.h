/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#ifndef MOS_TESTS_H
#define MOS_TESTS_H

/* Every test function, so test_main.c can call them. Add new ones here. */

/* test_pool.c */
void test_init_rejects_bad_args(void);
void test_init_state(void);
void test_destroy_twice_is_safe(void);
void test_alloc_is_lifo(void);
void test_alloc_until_empty(void);
void test_free_out_of_order(void);
void test_scrambled_free_then_realloc_all(void);
void test_free_into_full_pool_is_rejected(void);
void test_free_invalid_index_is_rejected(void);
void test_alloc_resets_buffer(void);

/* test_buffer.c */
void test_buffer_strip(void);
void test_buffer_strip_invalid(void);
void test_buffer_prepend(void);
void test_buffer_prepend_invalid(void);
void test_buffer_strip_then_prepend(void);
void test_buffer_prepend_then_strip(void);

#endif /* MOS_TESTS_H */
