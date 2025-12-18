// SPDX-License-Identifier: GPL-2.0-only
/*
 * Test cases for printf facility.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <linux/kernel.h>
#include <module.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/string.h>
#include <errno.h>
#include <efi/guid.h>

#include <linux/bitmap.h>

#define BUF_SIZE 256
#define PAD_SIZE 16
#define FILL_CHAR '$'

BSELFTEST_GLOBALS();

static char *test_buffer __initdata;
static char *alloced_buffer __initdata;

static int __printf(4, 0) __init
do_test(int bufsize, const char *expect, int elen,
	const char *fmt, va_list ap)
{
	va_list aq;
	int ret, written;

	total_tests++;

	memset(alloced_buffer, FILL_CHAR, BUF_SIZE + 2*PAD_SIZE);
	va_copy(aq, ap);
	ret = vsnprintf(test_buffer, bufsize, fmt, aq);
	va_end(aq);

	if (ret != elen) {
		pr_warn("vsnprintf(buf, %d, \"%s\", ...) returned %d, expected %d\n",
			bufsize, fmt, ret, elen);
		pr_warn("vsnprintf(buf, %d, \"%s\", ...) wrote '%s', expected '%.*s'\n",
			bufsize, fmt, test_buffer, ret, expect);
		return 1;
	}

	if (memchr_inv(alloced_buffer, FILL_CHAR, PAD_SIZE)) {
		pr_warn("vsnprintf(buf, %d, \"%s\", ...) wrote before buffer\n", bufsize, fmt);
		return 1;
	}

	if (!bufsize) {
		if (memchr_inv(test_buffer, FILL_CHAR, BUF_SIZE + PAD_SIZE)) {
			pr_warn("vsnprintf(buf, 0, \"%s\", ...) wrote to buffer\n",
				fmt);
			return 1;
		}
		return 0;
	}

	written = min(bufsize-1, elen);
	if (test_buffer[written]) {
		pr_warn("vsnprintf(buf, %d, \"%s\", ...) did not nul-terminate buffer\n",
			bufsize, fmt);
		return 1;
	}

	if (memchr_inv(test_buffer + written + 1, FILL_CHAR, BUF_SIZE + PAD_SIZE - (written + 1))) {
		pr_warn("vsnprintf(buf, %d, \"%s\", ...) wrote beyond the nul-terminator\n",
			bufsize, fmt);
		return 1;
	}

	if (memcmp(test_buffer, expect, written)) {
		pr_warn("vsnprintf(buf, %d, \"%s\", ...) wrote '%s', expected '%.*s'\n",
			bufsize, fmt, test_buffer, written, expect);
		return 1;
	}
	return 0;
}

static void __printf(3, 4) __init
__test(const char *expect, int elen, const char *fmt, ...)
{
	va_list ap;
	int rand;
	char *p;

	if (elen >= BUF_SIZE) {
		pr_err("error in test suite: expected output length %d too long. Format was '%s'.\n",
		       elen, fmt);
		failed_tests++;
		return;
	}

	va_start(ap, fmt);

	/*
	 * Every fmt+args is subjected to four tests: Three where we
	 * tell vsnprintf varying buffer sizes (plenty, not quite
	 * enough and 0), and then we also test that bvasprintf would
	 * be able to print it as expected.
	 */
	failed_tests += do_test(BUF_SIZE, expect, elen, fmt, ap);
	rand = 1 + prandom_u32_max(elen+1);
	/* Since elen < BUF_SIZE, we have 1 <= rand <= BUF_SIZE. */
	failed_tests += do_test(rand, expect, elen, fmt, ap);
	failed_tests += do_test(0, expect, elen, fmt, ap);

	p = bvasprintf(fmt, ap);
	if (p) {
		total_tests++;
		if (memcmp(p, expect, elen+1)) {
			pr_warn("bvasprintf(..., \"%s\", ...) returned '%s', expected '%s'\n",
				fmt, p, expect);
			failed_tests++;
		}
		kfree(p);
	}
	va_end(ap);
}

#define test(expect, fmt, ...)					\
	__test(expect, strlen(expect), fmt, ##__VA_ARGS__)

static void __init
test_basic(void)
{
	/* Work around annoying "warning: zero-length gnu_printf format string". */
	char nul = '\0';

	test("", &nul);
	test("100%", "100%%");
	test("xxx%yyy", "xxx%cyyy", '%');
	__test("xxx\0yyy", 7, "xxx%cyyy", '\0');
}

static void __init
test_number(void)
{
	signed char val;

	test("0x1234abcd  ", "%#-12x", 0x1234abcd);
	test("  0x1234abcd", "%#12x", 0x1234abcd);
	test("0|001| 12|+123| 1234|-123|-1234", "%d|%03d|%3d|%+d|% d|%+d|% d", 0, 1, 12, 123, 1234, -123, -1234);
	test("0|1|1|32768|65535", "%hu|%hu|%hu|%hu|%hu", 0, 1, 65537, 32768, -1);
	test("0|1|1|-32768|-1", "%hd|%hd|%hd|%hd|%hd", 0, 1, 65537, 32768, -1);
	test("2015122420151225", "%ho%ho%#ho", 1037, 5282, -11627);

	test("2015122420151225", "%ho%ho%#ho", 1037, 5282, -11627);

	/*
	 * POSIX/C99: »The result of converting zero with an explicit
	 * precision of zero shall be no characters.« Hence the output
	 * from the below test should really be "00|0||| ". However,
	 * the kernel's printf also produces a single 0 in that
	 * case. This test case simply documents the current
	 * behaviour.
	 */
	test("00|0|0|0|0", "%.2d|%.1d|%.0d|%.*d|%1.0d", 0, 0, 0, 0, 0, 0);

	val = -16;
	test("0xfffffff0|0xf0|0xf0", "%#02x|%#02x|%#02x", val, val & 0xff, (u8)val);

	/* On some platforms, test failure here indicates a misaligned stack */
	test("0x0807060504030201", "0x%016llx", 0x0807060504030201ULL);
}

static void __init
test_string(void)
{
	test("", "%s%.0s", "", "123");
	test("ABCD|abc|123", "%s|%.3s|%.*s", "ABCD", "abcdef", 3, "123456");
	test("1  |  2|3  |  4|5  ", "%-3s|%3s|%-*s|%*s|%*s", "1", "2", 3, "3", 3, "4", -3, "5");
	test("1234      ", "%-10.4s", "123456");
	test("      1234", "%10.4s", "123456");
}

static void __init
test_wstring(void)
{
	if (!IS_ENABLED(CONFIG_PRINTF_WCHAR))
		return;

	test("", "%ls%.0ls", L"", L"123");
	test("ABCD|abc|123", "%ls|%.3ls|%.*ls", L"ABCD", L"abcdef", 3, L"123456");
	test("1  |  2|3  |  4|5  ", "%-3ls|%3ls|%-*ls|%*ls|%*ls",
	     L"1", L"2", 3, L"3", 3, L"4", -3, L"5");
	test("1234      ", "%-10.4ls", L"123456");
	test("      1234", "%10.4ls", L"123456");
}

#if BITS_PER_LONG == 64

#define PTR_WIDTH 16
#define PTR ((void *)0xffff0123456789abUL)
#define PTR_STR "ffff0123456789ab"
#define PTR_VAL_NO_CRNG "(____ptrval____)"
#define ZEROS "00000000"	/* hex 32 zero bits */
#define ONES "ffffffff"		/* hex 32 one bits */

#else

#define PTR_WIDTH 8
#define PTR ((void *)0x456789ab)
#define PTR_STR "456789ab"
#define PTR_VAL_NO_CRNG "(ptrval)"
#define ZEROS ""
#define ONES ""

#endif	/* BITS_PER_LONG == 64 */

/*
 * NULL pointers aren't hashed.
 */
static void __init
null_pointer(void)
{
	test(ZEROS "00000000", "%p", NULL);
	test(ZEROS "00000000", "%px", NULL);
}

/*
 * Error pointers aren't hashed.
 */
static void __init
error_pointer(void)
{
	test(ONES "fffffff5", "%p", ERR_PTR(-11));
	test(ONES "fffffff5", "%px", ERR_PTR(-11));
}

#define PTR_INVALID ((void *)0x000000ab)

static void __init
invalid_pointer(void)
{
	test(ZEROS "000000ab", "%px", PTR_INVALID);
}

static void __init
ip4(void)
{
	IPaddr_t ip = cpu_to_be32(0x7f000001);

	test("127.0.0.1", "%pI4", &ip);
}

static void __init
ip(void)
{
	ip4();
}

static void __init
uuid(void)
{
	const char uuid[16] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
			       0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
	efi_guid_t protocol_guid = EFI_LOAD_FILE2_PROTOCOL_GUID;

	if (!IS_ENABLED(CONFIG_PRINTF_UUID)) {
		pr_info("skipping UUID tests: disabled in config\n");
		skipped_tests += 4;
		return;
	}

	test("00010203-0405-0607-0809-0a0b0c0d0e0f", "%pUb", uuid);
	test("00010203-0405-0607-0809-0A0B0C0D0E0F", "%pUB", uuid);
	test("03020100-0504-0706-0809-0a0b0c0d0e0f", "%pUl", uuid);
	test("03020100-0504-0706-0809-0A0B0C0D0E0F", "%pUL", uuid);
	if (IS_ENABLED(CONFIG_EFI_GUID))
		test("EFI Load File 2 Protocol", "%pUs", &protocol_guid);
}

static void __init
errptr(void)
{
	test("error 1234", "%pe", ERR_PTR(-1234));
	test(sizeof(void *) == 8 ? "00000000000004d2" : "000004d2", "%pe", ERR_PTR(1234));

	/* Check that %pe with a non-ERR_PTR gets treated as ordinary %p. */
	BUILD_BUG_ON(IS_ERR(PTR));

	if (!IS_ENABLED(CONFIG_ERRNO_MESSAGES)) {
		pr_info("skipping errno messages tests: disabled in config\n");
		skipped_tests += 2;
		return;
	}
	test("(Operation not permitted)", "(%pe)", ERR_PTR(-EPERM));
	test("Requested probe deferral", "%pe", ERR_PTR(-EPROBE_DEFER));
}

static void __init
test_hexstr(void)
{
	u8 buf[4] = { 0, 1, 2, 3 };

	if (!IS_ENABLED(CONFIG_PRINTF_HEXSTR)) {
		pr_info("skipping hexstr tests: disabled in config\n");
		skipped_tests += 4;
		return;
	}

	test("[00 01 02 03]", "[%*ph]",  (int)sizeof(buf), buf);
	test("[00:01:02:03]", "[%*phC]", (int)sizeof(buf), buf);
	test("[00-01-02-03]", "[%*phD]", (int)sizeof(buf), buf);
	test("[00010203]",    "[%*phN]", (int)sizeof(buf), buf);
}

static void __init
test_pointer(void)
{
	null_pointer();
	error_pointer();
	invalid_pointer();
	ip();
	uuid();
	errptr();
}

static void __init
test_jsonpath(void)
{
	if (!IS_ENABLED(CONFIG_JSMN)) {
		pr_info("skipping jsonpath tests: CONFIG_JSMN disabled in config\n");
		skipped_tests += 5;
		return;
	}

	test("<NULL>", "%pJP",  NULL);
	test("$", "%pJP",  &(char *[]){ NULL });
	test("$.1", "%pJP",  &(char *[]){ "1", NULL });
	test("$.1.23", "%pJP",  &(char *[]){ "1", "23", NULL });
	test("$.1.23.456", "%pJP",  &(char *[]){ "1", "23", "456", NULL });
}

static void __init test_printf(void)
{
	alloced_buffer = malloc(BUF_SIZE + 2*PAD_SIZE);
	if (!alloced_buffer)
		return;
	test_buffer = alloced_buffer + PAD_SIZE;

	test_basic();
	test_number();
	test_string();
	test_wstring();
	test_pointer();
	test_hexstr();
	test_jsonpath();

	free(alloced_buffer);
}

bselftest(core, test_printf);
MODULE_AUTHOR("Rasmus Villemoes <linux@rasmusvillemoes.dk>");
MODULE_LICENSE("GPL");
