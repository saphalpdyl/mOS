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
				__func__, #cond);                                            \
			failures++;                                                          \
		}                                                                            \
	} while (0)

#define RUN(test)                                                                  \
	do {                                                                       \
		int before = failures;                                             \
		test();                                                            \
		printf("%-40s %s\n", #test, failures == before ? "ok" : "FAILED"); \
	} while (0)

#endif /* MOS_TEST_COMMON_H */
