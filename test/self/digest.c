// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <bselftest.h>
#include <clock.h>
#include <digest.h>

BSELFTEST_GLOBALS();

struct digest_test_case {
	const char *name;
	const void *buf;
	size_t buf_size;
	const void *digest_str;
	u64 time_ns;
};

#define TEST_CASE(buf, digest_str) \
	{ #buf, buf, sizeof(buf), digest_str }

#define test_digest(option, algo, ...) do { \
	struct digest_test_case *t, cases[] = { __VA_ARGS__, { /* sentinel */ } }; \
	for (t = cases; t->buf; t++) \
		__test_digest((option), (algo), t, __func__, __LINE__); \
	if (!__is_defined(DEBUG)) \
		break; \
	printf("%s:\t", algo); \
	for (t = cases; t->buf; t++) \
		printf(" digest(%zu bytes) = %10lluns", t->buf_size, t->time_ns); \
	printf("\n"); \
} while (0)

static inline const char *digest_suffix(const char *str, const char *suffix)
{
	static char buf[32];

	if (!*suffix)
		return str;

	WARN_ON(snprintf(buf, sizeof(buf), "%s-%s", str, suffix) >= sizeof(buf));
	return buf;
}

static void __test_digest(bool option,
			  const char *algo, struct digest_test_case *t,
			  const char *func, int line)
{
	unsigned char *output = NULL, *digest = NULL;
	struct digest *d;
	int hash_len, digest_len;
	u64 start;
	int ret;

	total_tests++;

	if (!option) {
		skipped_tests++;
		return;
	}

	d = digest_alloc(algo);
	if (!d) {
		printf("%s:%d: failed to allocate %s digest\n", func, line, algo);
		goto fail;
	}

	hash_len = digest_length(d);
	digest_len = strlen(t->digest_str) / 2;
	if (hash_len != digest_len) {
		printf("%s:%d: %s digests have length %u, but %u expected\n",
		       func, line, algo, hash_len, digest_len);
		goto fail;
	}

	output = calloc(hash_len, 1);
	if (WARN_ON(!output))
		goto fail;

	digest = calloc(digest_len, 1);
	if (WARN_ON(!digest))
		goto fail;

	ret = hex2bin(digest, t->digest_str, digest_len);
	if (WARN_ON(ret))
		goto fail;

	start = get_time_ns();

	ret = digest_digest(d, t->buf, t->buf_size, output);
	if (ret) {
		printf("%s:%d: error calculating %s(%s): %pe\n",
		       func, line, algo, t->name, ERR_PTR(ret));
		goto fail;
	}

	t->time_ns = get_time_ns() - start;

	if (memcmp(output, digest, hash_len)) {
		printf("%s:%d: mismatch calculating %s(%s):\n\tgot: %*phN\n\tbut: %*phN expected\n",
		       func, line, algo, t->name, hash_len, output, hash_len, digest);
		goto fail;
	}

	digest_free(d);
	free(digest);
	free(output);
	return;
fail:
	digest_free(d);
	free(digest);
	free(output);
	failed_tests++;
}

static const u8 zeroes7[7] = {};
static const u8 one32[32] = { 1 };
static u8 inc4097[4097];

static void test_digest_md5(const char *suffix)
{
	bool cond;

	cond = !strcmp(suffix, "generic") ? IS_ENABLED(CONFIG_DIGEST_MD5_GENERIC) :
	       IS_ENABLED(CONFIG_HAVE_DIGEST_MD5);

	test_digest(cond, digest_suffix("md5", suffix),
		TEST_CASE(zeroes7, "d310a40483f9399dd7ed1712e0fdd702"),
		TEST_CASE(one32,   "b39ac6e2aa7e375c38ba7ae921b5ba89"),
		TEST_CASE(inc4097, "70410aad262cd11e63ae854804c8024b"));
}

static void test_digests_sha12(const char *suffix)
{
	bool cond;

	cond = !strcmp(suffix, "generic") ? IS_ENABLED(CONFIG_DIGEST_SHA1_GENERIC) :
	       !strcmp(suffix, "asm") ? IS_ENABLED(CONFIG_DIGEST_SHA1_ARM) :
	       IS_ENABLED(CONFIG_HAVE_DIGEST_SHA1);

	test_digest(cond, digest_suffix("sha1", suffix),
		TEST_CASE(zeroes7, "77ce0377defbd11b77b1f4ad54ca40ea5ef28490"),
		TEST_CASE(one32,   "cbd9cbfc20182e4b71e593e7ad598fc383cc6058"),
		TEST_CASE(inc4097, "c627e736efd8bb0dff1778335c9c79cb1f27e396"));


	cond = !strcmp(suffix, "generic") ? IS_ENABLED(CONFIG_DIGEST_SHA224_GENERIC) :
	       !strcmp(suffix, "asm") ? IS_ENABLED(CONFIG_DIGEST_SHA256_ARM) :
	       !strcmp(suffix, "ce")  ? IS_ENABLED(CONFIG_DIGEST_SHA256_ARM64_CE) :
	       IS_ENABLED(CONFIG_HAVE_DIGEST_SHA224);

	test_digest(cond, digest_suffix("sha224", suffix),
		TEST_CASE(zeroes7, "fbf6df85218ac5632461a8a17c6f294e6f35264cbfc0a9774a4f665b"),
		TEST_CASE(one32,   "343cb3950305e6e6331e294b0a4925739d09ecbd2b43a2fc87c09941"),
		TEST_CASE(inc4097, "6596b5dcfbd857f4246d6b94508b8a1a5b715a4f644a0c1e7d54c4f7"));


	cond = !strcmp(suffix, "generic") ? IS_ENABLED(CONFIG_DIGEST_SHA256_GENERIC) :
	       !strcmp(suffix, "asm") ? IS_ENABLED(CONFIG_DIGEST_SHA256_ARM) :
	       !strcmp(suffix, "ce")  ? IS_ENABLED(CONFIG_DIGEST_SHA256_ARM64_CE) :
	       IS_ENABLED(CONFIG_HAVE_DIGEST_SHA256);

	test_digest(cond, digest_suffix("sha256", suffix),
		TEST_CASE(zeroes7, "837885c8f8091aeaeb9ec3c3f85a6ff470a415e610b8ba3e49f9b33c9cf9d619"),
		TEST_CASE(one32,   "01d0fabd251fcbbe2b93b4b927b26ad2a1a99077152e45ded1e678afa45dbec5"),
		TEST_CASE(inc4097, "1e973d029df2b2c66cb42a942c5edb45966f02abaff29fe99410e44d271d0efc"));
}


static void test_digests_sha35(const char *suffix)
{
	bool cond;

	cond = !strcmp(suffix, "generic") ? IS_ENABLED(CONFIG_DIGEST_SHA384_GENERIC) :
	       IS_ENABLED(CONFIG_HAVE_DIGEST_SHA384);

	test_digest(cond, digest_suffix("sha384", suffix),
		TEST_CASE(zeroes7, "b56705a73cf280f06d3a6b482c441a3d280c930d0c44b04f364dcdcedcfbc47c"
				   "f3645a71da7b97f9e5d3a0924f6b9634"),
		TEST_CASE(one32,   "dd606b49d7658a5eae905d593271c280819f92eb1a9a4986057aedc0a5f2eaea"
				   "99052904718f6d83f16ad209d793f253"),
		TEST_CASE(inc4097, "f76046b90890f20ae94066a3ad33010f5b3b2fd46977414636bbc634898b06fd"
				   "4cb8f85e0926e8817e518300a930529e"));


	cond = !strcmp(suffix, "generic") ? IS_ENABLED(CONFIG_DIGEST_SHA512_GENERIC) :
	       IS_ENABLED(CONFIG_HAVE_DIGEST_SHA512);

	test_digest(cond, digest_suffix("sha512", suffix),
		TEST_CASE(zeroes7, "76afca18a9b81ffb967ffcf0460ed221c3605d3820057214d785fa88259bb5cb"
				   "729576178e6edb0134f645d2e2e92cbabf1333462f3b9058692c950f51c64a92"),
		TEST_CASE(one32,   "ce0c265ecc82dd8cee6e56ce44e45dafd7a0c5750df914b253a1fb7a8af66ddb"
				   "99763607f0a85d0bd43669194a3a40577a528af395f4f17e06f1defcc6deb2a5"),
		TEST_CASE(inc4097, "42eb09aca460d79b0c0aeac28187ed055a92e33602b69428461697680ff9f48f"
				   "60a5a68aa0017e3446433349b42592b74713d7787628a58e400b7f588b9bd69b"));
}

static void test_digests(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(inc4097); i++)
		inc4097[i] = i;

	test_digest_md5("generic");

	test_digests_sha12("generic");
	if (IS_ENABLED(CONFIG_ARM32))
		test_digests_sha12("asm");

	test_digests_sha35("generic");

	test_digest_md5("");
	test_digests_sha12("");
	test_digests_sha35("");

}
bselftest(core, test_digests);
