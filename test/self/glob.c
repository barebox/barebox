// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <fs.h>
#include <dirent.h>
#include <glob.h>
#include <libfile.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <bselftest.h>

BSELFTEST_GLOBALS();

#define expect(cond, fmt, ...) ({ \
	bool __cond = (cond); \
	total_tests++; \
	if (!__cond) { \
		failed_tests++; \
		printf("%s:%d failed: %s: " fmt "\n", \
		       __func__, __LINE__, #cond, ##__VA_ARGS__); \
	} \
	__cond; \
})

static bool glob_has(const glob_t *g, const char *path)
{
	for (size_t i = 0; i < g->gl_pathc; i++) {
		if (!strcmp(g->gl_pathv[i], path))
			return true;
	}

	return false;
}

/* The order of results is only defined with CONFIG_GLOB_SORT */
static bool glob_equal(const glob_t *g, const char * const expected[])
{
	size_t n = 0;

	for (; expected[n]; n++) {
		if (!glob_has(g, expected[n]))
			return false;
	}

	return g->gl_pathc == n;
}

static void glob_print(const glob_t *g)
{
	for (size_t i = 0; i < g->gl_pathc; i++)
		printf("  %s\n", g->gl_pathv[i]);
}

static void __expect_glob(int line, const char *pattern, int flags,
			  int expected_ret, const char * const expected[])
{
	glob_t g = {};
	int ret;

	total_tests++;

	ret = glob(pattern, flags, NULL, &g);
	if (ret != expected_ret || !glob_equal(&g, expected)) {
		failed_tests++;
		printf("%d: glob(\"%s\", 0x%x) = %d (expected %d) with:\n",
		       line, pattern, flags, ret, expected_ret);
		glob_print(&g);
	}

	globfree(&g);
}

/* An empty list of results is spelt NULL */
#define expect_glob(pattern, flags, ret, ...) \
	__expect_glob(__LINE__, pattern, flags, ret, \
		      (const char *[]){ __VA_ARGS__, NULL })

static char *errfunc_path;

static int errfunc(const char *path, int err)
{
	errfunc_path = xstrdup(path);
	return 0;
}

static void test_glob_tree(void)
{
	expect_glob("/globtest/*", 0, 0,
		    "/globtest/a", "/globtest/b", "/globtest/c",
		    "/globtest/file1", "/globtest/file2", "/globtest/file10");
	expect_glob("/globtest/file?", 0, 0,
		    "/globtest/file1", "/globtest/file2");
	expect_glob("/globtest/file[12]", 0, 0,
		    "/globtest/file1", "/globtest/file2");
	expect_glob("/globtest/file[!1]", 0, 0, "/globtest/file2");
	expect_glob("/globtest/file[0-9]*", 0, 0,
		    "/globtest/file1", "/globtest/file2", "/globtest/file10");
	expect_glob("/globtest/nope*", 0, GLOB_NOMATCH, NULL);
	expect_glob("/globtest/nope*", GLOB_NOCHECK, 0, "/globtest/nope*");

	/* A leading period must be matched explicitly */
	expect_glob("/globtest/*hidden", 0, GLOB_NOMATCH, NULL);
	expect_glob("/globtest/*hidden", GLOB_PERIOD, 0, "/globtest/.hidden");
	expect_glob("/globtest/.h*", 0, 0, "/globtest/.hidden");

	/* Wildcards in a directory component */
	expect_glob("/globtest/*/x", 0, 0, "/globtest/a/x", "/globtest/b/x");
	expect_glob("/globtest/?/x", 0, 0, "/globtest/a/x", "/globtest/b/x");
	expect_glob("/globtest/*/x", GLOB_ERR, 0,
		    "/globtest/a/x", "/globtest/b/x");
	expect_glob("/g*test/a/x", 0, 0, "/globtest/a/x");
	expect_glob("/globtest/*/*", 0, 0,
		    "/globtest/a/x", "/globtest/a/y", "/globtest/b/x");
	expect_glob("/globtest/*/nope", 0, GLOB_NOMATCH, NULL);
	expect_glob("/globtest/*/nope", GLOB_NOCHECK, 0, "/globtest/*/nope");

	/* No wildcards at all */
	expect_glob("/globtest/a/y", 0, 0, "/globtest/a/y");
	expect_glob("/globtest/a/y", GLOB_ERR, 0, "/globtest/a/y");
	expect_glob("/globtest/nope", 0, GLOB_NOMATCH, NULL);
	expect_glob("/globtest/nope", GLOB_NOCHECK, 0, "/globtest/nope");

	/* Escaped names */
	expect_glob("/globtest/a/\\y", 0, 0, "/globtest/a/y");
	expect_glob("/globtest/*/\\x", 0, 0, "/globtest/a/x", "/globtest/b/x");
	expect_glob("/globtest/*/\\x", GLOB_NOESCAPE, GLOB_NOMATCH, NULL);
	expect_glob("/globtest/file\\?", 0, GLOB_NOMATCH, NULL);

	/* Directories get a trailing slash */
	expect_glob("/globtest/[ab]", GLOB_MARK, 0,
		    "/globtest/a/", "/globtest/b/");
	expect_glob("/globtest/[ab]/", 0, 0, "/globtest/a/", "/globtest/b/");
	expect_glob("/globtest/file?", GLOB_MARK, 0,
		    "/globtest/file1", "/globtest/file2");
}

static void test_glob_relative(void)
{
	char *oldpwd;

	oldpwd = pushd("/globtest");
	if (!expect(oldpwd != NULL, "pushd: %m"))
		return;

	expect_glob("file?", 0, 0, "file1", "file2");
	expect_glob("*/x", 0, 0, "a/x", "b/x");
	expect_glob("a/y", 0, 0, "a/y");

	popd(oldpwd);
}

static void test_glob_flags(void)
{
	glob_t g = {};
	int ret;

	ret = glob("/globtest/file1", 0, NULL, &g);
	expect(ret == 0, "glob() = %d", ret);
	ret = glob("/globtest/a/*", GLOB_APPEND, NULL, &g);
	expect(ret == 0, "glob(GLOB_APPEND) = %d", ret);
	if (!expect(glob_equal(&g, (const char *[]){ "/globtest/file1",
			"/globtest/a/x", "/globtest/a/y", NULL }),
		    "GLOB_APPEND results"))
		glob_print(&g);
	globfree(&g);

	if (IS_ENABLED(CONFIG_GLOB_SORT)) {
		ret = glob("/globtest/file*", 0, NULL, &g);
		if (!expect(ret == 0 && g.gl_pathc == 3 &&
			    !strcmp(g.gl_pathv[0], "/globtest/file1") &&
			    !strcmp(g.gl_pathv[1], "/globtest/file10") &&
			    !strcmp(g.gl_pathv[2], "/globtest/file2"),
			    "sorted results"))
			glob_print(&g);
		globfree(&g);
	}

	ret = glob("/globtest/nodir/*", GLOB_ERR, NULL, &g);
	expect(ret == GLOB_ABORTED, "glob(GLOB_ERR) = %d", ret);
	globfree(&g);

	ret = glob("/globtest/nodir/*", 0, errfunc, &g);
	expect(ret == GLOB_NOMATCH, "glob(errfunc) = %d", ret);
	expect(!strcmp_ptr(errfunc_path, "/globtest/nodir"),
	       "errfunc path: %s", errfunc_path);
	free(errfunc_path);
	globfree(&g);
}

static void test_glob_symlink(void)
{
	int ret;

	ret = symlink("nowhere", "/globtest/c/dangling");
	if (!expect(ret == 0, "symlink() = %d", ret))
		return;

	expect_glob("/globtest/c/dangling", 0, 0, "/globtest/c/dangling");
	expect_glob("/globtest/*/dangling", 0, 0, "/globtest/c/dangling");
	expect_glob("/globtest/c/*", 0, 0, "/globtest/c/dangling");

	unlink("/globtest/c/dangling");
}

static void test_glob_automount(void)
{
	struct stat s;
	DIR *dir;
	int ret;

	if (!IS_ENABLED(CONFIG_FS_AUTOMOUNT) || !IS_ENABLED(CONFIG_CMD_MKDIR) ||
	    !IS_ENABLED(CONFIG_CMD_MOUNT)) {
		skipped_tests++;
		return;
	}

	ret = make_directory("/globtest/am");
	if (!expect(ret == 0, "make_directory() = %d", ret))
		return;

	ret = automount_add("/globtest/am", "mkdir /globtest/automounted; "
			    "mount -t ramfs none /globtest/am");
	if (!expect(ret == 0, "automount_add() = %d", ret))
		return;

	/* With GLOB_NOCHECK, a literal path isn't looked up */
	expect_glob("/globtest/am/x", GLOB_NOCHECK, 0, "/globtest/am/x");
	expect(lstat("/globtest/automounted", &s) != 0, "automount triggered");

	expect_glob("/globtest/am/*", GLOB_NOCHECK, 0, "/globtest/am/*");
	expect(lstat("/globtest/automounted", &s) == 0,
	       "automount not triggered");

	umount("/globtest/am");
	automount_remove("/globtest/am");

	dir = opendir("/globtest/am");
	if (expect(dir != NULL, "opendir() after automount_remove(): %m"))
		closedir(dir);
}

static int create_tree(void)
{
	static const char * const dirs[] = {
		"/globtest/a/x", "/globtest/b", "/globtest/c",
		"/globtest/.hidden",
	};
	static const char * const files[] = {
		"/globtest/a/y", "/globtest/b/x", "/globtest/.hidden/x",
		"/globtest/file1", "/globtest/file2", "/globtest/file10",
	};
	int ret;

	for (int i = 0; i < ARRAY_SIZE(dirs); i++) {
		ret = make_directory(dirs[i]);
		if (!expect(ret == 0, "make_directory(%s) = %d", dirs[i], ret))
			return ret;
	}

	for (int i = 0; i < ARRAY_SIZE(files); i++) {
		ret = write_file(files[i], "", 0);
		if (!expect(ret == 0, "write_file(%s) = %d", files[i], ret))
			return ret;
	}

	return 0;
}

static void test_glob(void)
{
	char *failedpath = NULL;
	int ret;

	if (!create_tree()) {
		test_glob_tree();
		test_glob_relative();
		test_glob_flags();
		test_glob_symlink();
		test_glob_automount();
	}

	ret = unlink_recursive("/globtest", &failedpath);
	expect(ret == 0, "unlink_recursive(%s) = %d", failedpath, ret);
}
bselftest(core, test_glob);
