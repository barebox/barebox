// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <fcntl.h>
#include <fs.h>
#include <string.h>
#include <linux/bitfield.h>
#include <unistd.h>
#include <bselftest.h>
#include <libfile.h>

BSELFTEST_GLOBALS();

#define expect(cond, res, fmt, ...) ({ \
	int __cond = (cond); \
	int __res = (res); \
	total_tests++; \
	if (__cond != __res) { \
		failed_tests++; \
		printf("%s:%d failed: %s == %d: " fmt "\n", \
		       __func__, __LINE__, #cond, __cond, ##__VA_ARGS__); \
	} \
	__cond == __res; \
})

static void check_statat(const char *at, int dirfd, const char *prefix, unsigned expected)
{
	static const char *paths[] = { ".", "..", "testfile", "dirfdtest" };
	struct stat s;

	for (int i = 0; i < ARRAY_SIZE(paths); i++) {
		const char *path = paths[i];
		char *fullpath = NULL, *testpath = basprintf("%s%s", prefix, path);
		struct fs_device *fsdev1, *fsdev2;
		int ret;

		ret = statat(dirfd, testpath, &s);
		if (!expect(ret == 0, FIELD_GET(BIT(2), expected),
			    "statat(%s, %s): %m", at, testpath))
			goto next;

		fullpath = canonicalize_path(dirfd, testpath);
		if (!expect(fullpath != NULL, FIELD_GET(BIT(1), expected),
			    "canonicalize_path(%s, %s): %m", at, testpath))
			goto next;

		if (!fullpath)
			goto next;

		fsdev1 = get_fsdevice_by_path(AT_FDCWD, fullpath);
		if (!expect(IS_ERR_OR_NULL(fsdev1), false, "get_fsdevice_by_path(AT_FDCWD, %s)",
			    fullpath))
			goto next;

		fsdev2 = get_fsdevice_by_path(dirfd, testpath);
		if (!expect(IS_ERR_OR_NULL(fsdev1), false, "get_fsdevice_by_path(%s, %s)",
			    at, testpath))
			goto next;

		if (!expect(fsdev1 == fsdev2, true,
			    "get_fsdevice_by_path(%s, %s) != get_fsdevice_by_path(AT_FDCWD, %s)",
			    fullpath, at, testpath))
			goto next;

		ret = strcmp_ptr(fsdev1->path, "/dirfdtest");
		if (!expect(ret == 0, FIELD_GET(BIT(0), expected),
			    "fsdev_of(%s)->path = %s != /dirfdtest", fullpath, fsdev1->path))
			goto next;

next:
		expected >>= 3;
		free(testpath);
		free(fullpath);
	}
}

static void do_test_dirfd(const char *at, int dirfd,
			  unsigned expected1, unsigned expected2,
			  unsigned expected3, unsigned expected4)
{
	if (dirfd < 0 && dirfd != AT_FDCWD)
		return;

	check_statat(at, dirfd, "",             expected1);
	check_statat(at, dirfd, "./",           expected1);
	check_statat(at, dirfd, "/dirfdtest/",        expected2);
	check_statat(at, dirfd, "/dirfdtest/./",      expected2);
	check_statat(at, dirfd, "/dirfdtest/../dirfdtest/", expected2);
	check_statat(at, dirfd, "/",            expected3);
	check_statat(at, dirfd, "../",          expected4);

	if (dirfd >= 0)
		close(dirfd);
}


static void test_dirfd(void)
{
	int fd, ret;

	fd = open("/", O_PATH | O_DIRECTORY);
	if (!expect(fd < 0, false, "open(/, O_PATH | O_DIRECTORY) = %d", fd))
		close(fd);

	ret = make_directory("/dirfdtest");
	if (!expect(ret == 0, true, "make_directory(\"/dirfdtest\") = %d", ret))
		return;

	ret = mount("none", "ramfs", "/dirfdtest", NULL);
	if (!expect(ret == 0, true, "mount(\"none\", \"ramfs\", \"/dirfdtest\") = %d", ret))
		goto out;

	ret = write_file("/dirfdtest/testfile", __func__, strlen(__func__));
	if (!expect(ret == 0, true, "write_file() = %d", ret))
		goto out;

#define B(dot, dotdot, zero, dev) 0b##dev##zero##dotdot##dot
	/* We do fiften tests for every configuration
	 * for dir in ./ /dirfdtest / ../ ; do
	 *	for file in . .. zero dirfdtest ; do
	 *		test if file exists
	 *		test if file can be canonicalized
	 *		test if parent FS is mounted at /dirfdtest
	 *	done
	 * done
	 *
	 * The bits belows correspond to whether a test fails in the above loop
	 */

	do_test_dirfd("AT_FDCWD", AT_FDCWD,
		      B(110,110,000,111), B(111,110,111,000),
		      B(110,110,000,111), B(110,110,000,111));
	do_test_dirfd("/dirfdtest", open("/dirfdtest", O_PATH | O_DIRECTORY),
		      B(111,110,111,000), B(111,110,111,000),
		      B(110,110,000,111), B(110,110,000,111));
	do_test_dirfd("/dirfdtest O_CHROOT", open("/dirfdtest", O_PATH | O_CHROOT),
		      B(111,111,111,000), B(000,000,000,000),
		      B(111,111,111,000), B(111,111,111,000));

out:
	ret = umount("/dirfdtest");
	expect(ret == 0, true, "umount(\"/dirfdtest\") = %d", ret);

	ret = rmdir("/dirfdtest");
	expect(ret == 0, true, "rmdir(\"/dirfdtest\") = %d", ret);
}
bselftest(core, test_dirfd);
