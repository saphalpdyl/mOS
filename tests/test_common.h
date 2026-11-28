/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#ifndef MOS_TEST_COMMON_H
#define MOS_TEST_COMMON_H

#include <stdio.h>

extern int failures;

/* Report a failure and keep going (unlike assert, which aborts). */
#define CHECK(cond)                                                                          \
	do {                                                                                 \
		if (!(cond)) {                                                               \
			fprintf(stderr, "%s:%d: %s: CHECK failed: %s\n", __FILE__, __LINE__, \
				__func__, #cond);                                           \
			failures++;                                                          \
		}                                                                            \
	} while (0)

struct __attribute__((aligned(16))) mtest {
	const char *test_name;
	const char *group;
	void (*test_func)(void);
};

#define TEST(func_name)                                                                   \
	static void func_name(void);                                                      \
	__attribute__((section("mtests"),                                             \
		       __used__)) static const struct mtest func_name##_entry = {     \
		.test_name = #func_name, .group = __FILE_NAME__, .test_func = func_name}; \
	static void func_name(void)

#endif /* MOS_TEST_COMMON_H */
