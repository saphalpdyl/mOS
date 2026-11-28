/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2026 saphalpdyl.
 */

#include <string.h>

#include "mos/lib/core/buffer.h"

#include "test_common.h"

#define TEST_BUFFER_PKT_SIZE  10
#define TEST_BUFFER_STRIP_N   5
#define TEST_BUFFER_PREPEND_N 5

static void test_buffer_setup(struct mlib_buffer *buf)
{
	mlib_buffer_init(buf);
}

static void test_buffer_setup_with_pkt(struct mlib_buffer *buf)
{
	test_buffer_setup(buf);

	memset(&buf->data[buf->data_off], 3, TEST_BUFFER_PKT_SIZE);
	buf->data_len = TEST_BUFFER_PKT_SIZE;
}

TEST(test_buffer_strip)
{
	struct mlib_buffer buf;
	test_buffer_setup_with_pkt(&buf);

	CHECK(mlib_buffer_strip_bytes(&buf, TEST_BUFFER_STRIP_N) == 0);

	CHECK(buf.data_len == (TEST_BUFFER_PKT_SIZE - TEST_BUFFER_STRIP_N));
	CHECK(buf.data_off == MLIB_BUF_HEADROOM_SIZE + TEST_BUFFER_STRIP_N);
}

TEST(test_buffer_strip_invalid)
{
	struct mlib_buffer buf;
	test_buffer_setup_with_pkt(&buf);

	CHECK(mlib_buffer_strip_bytes(&buf, TEST_BUFFER_PKT_SIZE + 5) != 0);
}

TEST(test_buffer_prepend)
{
	struct mlib_buffer buf;
	test_buffer_setup_with_pkt(&buf);

	void *ptr = mlib_buffer_prepend_bytes(&buf, TEST_BUFFER_PREPEND_N);
	CHECK(ptr != NULL);

	CHECK(buf.data_off == (MLIB_BUF_HEADROOM_SIZE - TEST_BUFFER_PREPEND_N));
	CHECK(buf.data_len == TEST_BUFFER_PREPEND_N + TEST_BUFFER_PKT_SIZE);

#ifndef NDEBUG
	int mem_expected[TEST_BUFFER_PREPEND_N + 3] = {0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 3, 3, 3};
#else
	int mem_expected[TEST_BUFFER_PREPEND_N + 3] = {0, 0, 0, 0, 0, 3, 3, 3};
#endif
	uint8_t matched = 1;
	for (int i = 0; i < TEST_BUFFER_PREPEND_N + 3; ++i) {
		if (buf.data[buf.data_off + i] != mem_expected[i]) {
			matched = 0;
			break;
		}
	}

	CHECK(matched == 1);
}

TEST(test_buffer_prepend_invalid)
{
	struct mlib_buffer buf;
	test_buffer_setup_with_pkt(&buf);

	uint32_t prev_off = buf.data_off;
	uint32_t prev_len = buf.data_len;

	void *ptr = mlib_buffer_prepend_bytes(&buf, MLIB_BUF_HEADROOM_SIZE + 1);
	CHECK(ptr == NULL);

	CHECK(prev_off == buf.data_off);
	CHECK(prev_len == buf.data_len);
}

TEST(test_buffer_strip_then_prepend)
{
	struct mlib_buffer buf;
	test_buffer_setup_with_pkt(&buf);
	/* length = 10; offset = 0*/

	CHECK(mlib_buffer_strip_bytes(&buf, 5) == 0);
	/* length should decrease by 5, offset increase by 5*/

	void *ptr = mlib_buffer_prepend_bytes(&buf, 7);
	CHECK(ptr != NULL);

	/*length should increase by 7; offset should decrease by 7*/

	/* delta length = increase by 2; delta offset = decrease by 2*/
	/* length = 12; offset = 0*/

	CHECK(buf.data_len == 12);
	CHECK(buf.data_off == MLIB_BUF_HEADROOM_SIZE - 2);
}

TEST(test_buffer_prepend_then_strip)
{
	struct mlib_buffer buf;
	test_buffer_setup_with_pkt(&buf);
	/* length = 10; offset = 0*/

	void *ptr = mlib_buffer_prepend_bytes(&buf, 7);
	CHECK(ptr != NULL);

	/*length should increase by 7; offset should decrease by 7*/

	CHECK(mlib_buffer_strip_bytes(&buf, 5) == 0);
	/* length should decrease by 5, offset increase by 5*/

	/* delta length = increase by 2; delta offset = decrease by 2*/
	/* length = 12; offset = 0*/

	CHECK(buf.data_len == 12);
	CHECK(buf.data_off == MLIB_BUF_HEADROOM_SIZE - 2);
}
