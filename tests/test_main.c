/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#include <stdio.h>
#include <string.h>

#include "test_common.h"

extern const struct mtest __start_mtests[];
extern const struct mtest __stop_mtests[];

int main(void)
{
	const struct mtest *t = __start_mtests;
	const char *prev_group = NULL;
	int group_failures = 0;
	int group_items = 0;

#define PRINT_GROUP_END()                                                                      \
	printf("\nGROUP %s: %d OK | %d FAILED \n\n", prev_group, group_items - group_failures, \
	       group_failures)

	for (; t < __stop_mtests; t++) {
		if (prev_group == NULL || strcmp(prev_group, t->group) != 0) {
			if (prev_group != NULL) {
				PRINT_GROUP_END();
			}
			prev_group = t->group;
			printf("GROUP: %s \n", t->group);
			printf("================\n");
			group_failures = 0;
			group_items = 0;
		}

		int before = failures;
		t->test_func();
		group_items++;
		if (failures != before) {
			group_failures++;
			printf("%s | FAILED\n", t->test_name);
			continue;
		}
		printf("%s | OK\n", t->test_name);
	}
	if (prev_group != NULL) {
		PRINT_GROUP_END();
	}

	if (failures)
		fprintf(stderr, "\n%d check(s) failed\n", failures);
	return failures ? 1 : 0;
}
