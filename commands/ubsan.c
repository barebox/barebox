// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <command.h>
#include <abort.h>

struct test_ubsan {
	const char *cmd;
	void(*fun)(void);
};

static void test_ubsan_add_overflow(void)
{
	volatile int val = INT_MAX;

	val += 2;
}

static void test_ubsan_sub_overflow(void)
{
	volatile int val = INT_MIN;
	volatile int val2 = 2;

	val -= val2;
}

static void test_ubsan_mul_overflow(void)
{
	volatile int val = INT_MAX / 2;

	val *= 3;
}

static void test_ubsan_negate_overflow(void)
{
	volatile int val = INT_MIN;

	val = -val;
}

static void test_ubsan_divrem_overflow(void)
{
	volatile int val = 16;
	volatile int val2 = 0;

	val /= val2;
}

static void test_ubsan_shift_out_of_bounds(void)
{
	volatile int val = -1;
	int val2 = 10;

	val2 <<= val;
}

static void test_ubsan_out_of_bounds(void)
{
	volatile int i = 4, j = 5;
	volatile int arr[4];

	arr[j] = i;
}

static void test_ubsan_load_invalid_value(void)
{
	volatile char *dst, *src;
	bool val, val2, *ptr;
	char c = 4;

	dst = (char *)&val;
	src = &c;
	*dst = *src;

	ptr = &val2;
	val2 = val;
}

static void test_ubsan_null_ptr_deref(void)
{
	volatile int *ptr = NULL;
	int val;

	data_abort_mask();
	val = *ptr;
	data_abort_unmask();
}

static void test_ubsan_misaligned_access(void)
{
	volatile char arr[5] __aligned(4) = {1, 2, 3, 4, 5};
	volatile int *ptr, val = 6;

	ptr = (int *)(arr + 1);
	*ptr = val;
}

static void test_ubsan_object_size_mismatch(void)
{
	/* "((aligned(8)))" helps this not into be misaligned for ptr-access. */
	volatile int val __aligned(8) = 4;
	volatile long long *ptr, val2;

	ptr = (long long *)&val;
	OPTIMIZER_HIDE_VAR(ptr);

	val2 = *ptr;
}

static const struct test_ubsan test_ubsan_array[] = {
	{ .cmd = "add",   .fun = test_ubsan_add_overflow },
	{ .cmd = "sub",   .fun = test_ubsan_sub_overflow },
	{ .cmd = "mul",   .fun = test_ubsan_mul_overflow },
	{ .cmd = "neg",   .fun = test_ubsan_negate_overflow },
	{ .cmd = "div",   .fun = test_ubsan_divrem_overflow },
	{ .cmd = "shift", .fun = test_ubsan_shift_out_of_bounds },
	{ .cmd = "oob",   .fun = test_ubsan_out_of_bounds },
	{ .cmd = "trap",  .fun = test_ubsan_load_invalid_value },
	{ .cmd = "null",  .fun = test_ubsan_null_ptr_deref },
	{ .cmd = "align", .fun = test_ubsan_misaligned_access },
	{ .cmd = "size",  .fun = test_ubsan_object_size_mismatch },
	{ /* sentinel */ }
};

static int do_ubsan(int argc, char *argv[])
{
	const struct test_ubsan *test;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	for (test = test_ubsan_array; test->cmd; test++) {
		if (strcmp(test->cmd, argv[1]) == 0) {
			test->fun();
			return 0;
		}
	}

	return COMMAND_ERROR_USAGE;
}

BAREBOX_CMD_HELP_START(ubsan)
BAREBOX_CMD_HELP_TEXT("trigger undefined behavior for UBSAN to detect")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Functions:")
BAREBOX_CMD_HELP_TEXT("add, sub, mul, neg, div, shift, oob, trap,")
BAREBOX_CMD_HELP_TEXT("null, align, size")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ubsan)
	.cmd		= do_ubsan,
	BAREBOX_CMD_DESC("trigger undefined behavior for UBSAN to detect")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_ubsan_help)
BAREBOX_CMD_END
