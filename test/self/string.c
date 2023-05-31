// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <string.h>

BSELFTEST_GLOBALS();

static const char *strverscmp_expect_str(int expect)
{
	switch (expect) {
	case -1: return "<";
	case  0: return "==";
	case  1: return ">";
	default: return "?!";
	}
}

static int strverscmp_assert_one(const char *lhs, const char *rhs, int expect)
{
	int actual;

	total_tests++;

	actual = strverscmp(lhs, rhs);
	if (actual != expect) {
		failed_tests++;
		printf("(%s %s %s), but (%s %s %s) expected\n",
		       lhs, strverscmp_expect_str(actual), rhs,
		       lhs, strverscmp_expect_str(expect), rhs);
	}

	return actual;
}

static int __strverscmp_assert(char *expr)
{
	const char *token, *tokens[3];
	int expect = -42;
	int i = 0;

	while ((token = strsep_unescaped(&expr, " "))) {
		if (i == 3) {
			pr_err("invalid expression\n");
			return -EILSEQ;
		}

		tokens[i++] = token;
	}

	if (!strcmp(tokens[1], "<"))
	    expect = -1;
	else if (!strcmp(tokens[1], "=="))
	    expect = 0;
	else if (!strcmp(tokens[1], ">"))
	    expect = 1;

	return strverscmp_assert_one(tokens[0], tokens[2], expect);
}

#define strverscmp_assert(expr) ({ \
	char __expr_mut[] = expr; \
	__strverscmp_assert(__expr_mut); \
})

static void test_strverscmp_spec_examples(void)
{
	/*
	 * Taken from specification at
	 * https://uapi-group.org/specifications/specs/version_format_specification/#examples
	 */
	strverscmp_assert("11 == 11");
	strverscmp_assert("systemd-123 == systemd-123");
	strverscmp_assert("bar-123 < foo-123");
	strverscmp_assert("123a > 123");
	strverscmp_assert("123.a > 123");
	strverscmp_assert("123.a < 123.b");
	strverscmp_assert("123a > 123.a");
	strverscmp_assert("11α == 11β");
	strverscmp_assert("A < a");
	strverscmp_assert_one("", "0", -1);
	strverscmp_assert("0. > 0");
	strverscmp_assert("0.0 > 0");
	strverscmp_assert("0 > ~");
	strverscmp_assert_one("", "~", 1);
	strverscmp_assert("1_ == 1");
	strverscmp_assert("_1 == 1");
	strverscmp_assert("1_ < 1.2");
	strverscmp_assert("1_2_3 > 1.3.3");
	strverscmp_assert("1+ == 1");
	strverscmp_assert("+1 == 1");
	strverscmp_assert("1+ < 1.2");
	strverscmp_assert("1+2+3 > 1.3.3");
}

static void test_strverscmp_one(const char *newer, const char *older)
{
        strverscmp_assert_one(newer, newer,  0);
        strverscmp_assert_one(newer, older,  1);
        strverscmp_assert_one(older, newer, -1);
        strverscmp_assert_one(older, older,  0);
}

static void test_strverscmp_spec_systemd(void)
{
	/*
	 * Taken from systemd tests at
	 * 87b7d9b6ff23ec10b66bf53efeabf16ad85d7ad8
	 */
        static const char * const versions[] = {
                "~1", "", "ab", "abb", "abc", "0001", "002", "12", "122", "122.9",
                "123~rc1", "123", "123-a", "123-a.1", "123-a1", "123-a1.1", "123-3",
                "123-3.1", "123^patch1", "123^1", "123.a-1" "123.1-1", "123a-1", "124",
                NULL,
        };
        const char * const *p, * const *q;

	for (p = versions; *p; p++)
		for (q = p + 1; *q; q++)
                        test_strverscmp_one(*q, *p);

        test_strverscmp_one("123.45-67.89", "123.45-67.88");
        test_strverscmp_one("123.45-67.89a", "123.45-67.89");
        test_strverscmp_one("123.45-67.89", "123.45-67.ab");
        test_strverscmp_one("123.45-67.89", "123.45-67.9");
        test_strverscmp_one("123.45-67.89", "123.45-67");
        test_strverscmp_one("123.45-67.89", "123.45-66.89");
        test_strverscmp_one("123.45-67.89", "123.45-9.99");
        test_strverscmp_one("123.45-67.89", "123.42-99.99");
        test_strverscmp_one("123.45-67.89", "123-99.99");

        /* '~' : pre-releases */
        test_strverscmp_one("123.45-67.89", "123~rc1-99.99");
        test_strverscmp_one("123-45.67.89", "123~rc1-99.99");
        test_strverscmp_one("123~rc2-67.89", "123~rc1-99.99");
        test_strverscmp_one("123^aa2-67.89", "123~rc1-99.99");
        test_strverscmp_one("123aa2-67.89", "123~rc1-99.99");

        /* '-' : separator between version and release. */
        test_strverscmp_one("123.45-67.89", "123-99.99");
        test_strverscmp_one("123^aa2-67.89", "123-99.99");
        test_strverscmp_one("123aa2-67.89", "123-99.99");

        /* '^' : patch releases */
        test_strverscmp_one("123.45-67.89", "123^45-67.89");
        test_strverscmp_one("123^aa2-67.89", "123^aa1-99.99");
        test_strverscmp_one("123aa2-67.89", "123^aa2-67.89");

        /* '.' : point release */
        test_strverscmp_one("123aa2-67.89", "123.aa2-67.89");
        test_strverscmp_one("123.ab2-67.89", "123.aa2-67.89");

        /* invalid characters */
        strverscmp_assert_one("123_aa2-67.89", "123aa+2-67.89", 0);
}

static void test_strverscmp(void)
{
	test_strverscmp_spec_examples();
	test_strverscmp_spec_systemd();

	/* and now some corner cases */
	strverscmp_assert_one(NULL, NULL, 0);
	strverscmp_assert_one(NULL, "", 0);
	strverscmp_assert_one("", NULL, 0);
	strverscmp_assert_one("", "", 0);
}

static void test_string(void)
{
	test_strverscmp();
}
bselftest(parser, test_string);
