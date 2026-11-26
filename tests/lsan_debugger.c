/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

/*
 * LeakSanitizer needs ptrace to scan memory at exit, and it can't get it while
 * a debugger (gdb, CLion) is already attached. It then dies with exit code 1
 * even when nothing leaked. LeakSanitizer calls this hook at exit: returning 1
 * skips the leak check, so we do that only when a debugger is attached.
 * Normal runs and ctest still check for leaks.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int __lsan_is_turned_off(void)
{
	char line[256];
	int traced = 0;
	FILE *f = fopen("/proc/self/status", "r");

	if (!f)
		return 0;
	while (fgets(line, sizeof(line), f)) {
		if (strncmp(line, "TracerPid:", 10) == 0) {
			/* "TracerPid:\t0" means no debugger attached */
			traced = atoi(line + 10) != 0;
			break;
		}
	}
	fclose(f);
	return traced;
}
