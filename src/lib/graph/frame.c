/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#include "mos/lib/graph/frame.h"

#include <errno.h>
#include <stdlib.h>

int mlib_frame_alloc(struct mlib_frame **frame)
{
	if (!frame)
		return -EINVAL;

	*frame = malloc(sizeof(**frame));
	if (!*frame)
		return -ENOMEM;

	(*frame)->count = 0;

	return 0;
}

int mlib_frame_free(struct mlib_frame **frame)
{
	if (!frame)
		return -EINVAL;

	free(*frame);
	return 0;
}