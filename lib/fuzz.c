// SPDX-License-Identifier: GPL-2.0-only

#include <fuzz.h>
#include <string.h>
#include <common.h>

#define for_each_fuzz_test(test) \
	for (test = &__barebox_fuzz_tests_start; \
	     test != &__barebox_fuzz_tests_end; test++)

int call_for_each_fuzz_test(int (*fn)(const struct fuzz_test *test, void *ctx),
			    void *ctx)
{
	const struct fuzz_test *test;
	int ret;

	for_each_fuzz_test(test) {
		ret = fn(test, ctx);
		if (ret)
			return ret;
	}

	return 0;
}

static int list_fuzz_test_one(const struct fuzz_test *test, void *ctx)
{
	int (*println)(const char *) = ctx;

	println(test->name);
	return 0;
}

void list_fuzz_tests(int (*println)(const char *))
{
	call_for_each_fuzz_test(list_fuzz_test_one, println);
}

#ifdef CONFIG_FUZZ_EXTERNAL
const u8 *fuzzer_get_data(size_t *len);
#else
static inline const u8 *fuzzer_get_data(size_t *len)
{
	return NULL;
}
#endif

extern int LLVMFuzzerRunDriver(int *argc, char ***argv,
			       int (*cb)(const uint8_t *, size_t));

static const struct fuzz_test *fuzz;
static int *saved_argc;
static char ***saved_argv;

static int fuzzer_run(const uint8_t *buf, size_t len)
{
	return fuzz_test_once(fuzz, buf, len);
}

/*
 * barebox supplies its own main() and enters libFuzzer via
 * LLVMFuzzerRunDriver(), so the fuzzing engine's own main() is never
 * executed. It is still linked in when an external engine (e.g.
 * OSS-Fuzz's LIB_FUZZING_ENGINE) is used, because the C runtime's
 * reference to main() extracts the engine's main object file before
 * --defsym=main=sandbox_main takes effect. Define the entry point it
 * references, so the link succeeds.
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	return fuzzer_run(data, size);
}

static int fuzz_main(void)
{
	int ret = -1;

	if (IS_ENABLED(CONFIG_FUZZ_EXTERNAL)) {
		ret = LLVMFuzzerRunDriver(saved_argc, saved_argv, fuzzer_run);
		pr_emerg("libfuzzer unexpectedly ended: %d\n", ret);
	} else {
		pr_emerg("libfuzzer not supported in this build\n");
	}

	return ret;
}

int setup_external_fuzz(const char *fuzz_name,
			int *argc, char ***argv)
{
	const struct fuzz_test *test;

	saved_argc = argc;
	saved_argv = argv;

	for_each_fuzz_test(test) {
		if (streq_ptr(test->name, fuzz_name)) {
			fuzz = test;
			barebox_main = fuzz_main;
			barebox_loglevel = MSG_CRIT;
			return 0;
		}
	}

	return -1;
}

bool fuzz_external_active(void)
{
	return fuzz != NULL;
}
