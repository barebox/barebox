// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <common.h>
#include <fcntl.h>
#include <fs.h>
#include <dirent.h>
#include <libfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <bselftest.h>
#include <linux/sizes.h>

BSELFTEST_GLOBALS();

#define __expect(ret, cond, fmt, ...) ({ \
	bool __cond = (cond); \
	int __ret = (ret); \
	total_tests++; \
	\
	if (!__cond) { \
		failed_tests++; \
		printf("%s:%d error %pe: " fmt "\n", \
		       __func__, __LINE__, ERR_PTR(__ret), ##__VA_ARGS__); \
	} \
	__cond; \
})

static inline int ptr_to_err(const void *ptr)
{
	if (ptr)
		return PTR_ERR_OR_ZERO(ptr);

	return -errno ?: -EFAULT;
}

#define expect_success(ret, ...) __expect((ret), (ret) >= 0, __VA_ARGS__)
#define expect_ptrok(ptr, ...) expect_success(ptr_to_err(ptr), __VA_ARGS__)
#define expect_fail(ret, ...) __expect((ret), (ret) < 0, __VA_ARGS__)

static inline int get_file_count(int i)
{
	if (40 <= i && i < 50)
		return 40;

	if (i >= 50)
		i -= 10;

	/* we create a file for i == 0 as well */
	return i + 1;
}

static void test_ramfs(void)
{
	int files[] = { 1, 3, 5, 7, 11, 13, 17 };
	char fname[128];
	char *content = NULL;
	char *oldpwd = NULL;
	DIR *dir = NULL;
	char *dname;
	struct stat st;
	int i, j, ret, fd;
	struct dirent *d;

	dname = make_temp("ramfs-test");
	ret = mkdir(dname, 0777);

	if (!expect_success(ret, "creating directory"))
		return;

	ret = stat(dname, &st);
	if (!expect_success(ret, "stating directory"))
		goto out;

	expect_success(S_ISDIR(st.st_mode) ? 0 : -ENOTDIR,
		       "directory check");

	content = malloc(99);
	if (WARN_ON(!content))
		goto out;

	for (i = 0; i < 99; i++) {
		scnprintf(fname, sizeof(fname), "%s/file-%02d", dname, i);

		fd = open(fname, O_RDWR | O_CREAT);
		if (!expect_success(fd, "creating file"))
			continue;

		for (j = 0; j < i; j++)
			content[j] = i;

		ret = write(fd, content, i);
		expect_success(ret, "writing file");
		close(fd);

		if (40 <= i && i < 50) {
			ret = unlink(fname);
			expect_success(ret, "unlinking file");
		}

		ret = stat(fname, &st);
		if (40 <= i && i < 50) {
			expect_fail(ret, "stating file");
		} else {
			expect_success(ret, "stating file");

			expect_success(S_ISREG(st.st_mode) ? 0 : -EINVAL,
				       "stating file");
		}

		dir = opendir(dname);
		if (!expect_ptrok(dir, "opening parent directory"))
			continue;

		j = 0;

		while ((d = readdir(dir))) {
			if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
				continue;

			j++;
		}

		expect_success(j == get_file_count(i) ? 0 : -EINVAL,
			       "unexpected file count iterating directory");

		closedir(dir);
	}

	oldpwd = pushd(dname);
	if (!expect_ptrok(oldpwd, "pushd()"))
		goto out;;

	dir = opendir(".");
	if (!expect_ptrok(dir, "opening parent directory"))
		goto out;

	i = 1;
	j = 0;

	while ((d = readdir(dir))) {
		size_t size = 0;
		char *buf;

		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
			continue;

		buf = read_file(d->d_name, &size);
		if (size) {
			for (j = 0; j < size; j++) {
				expect_success(buf[j] == size ? 0 : -EINVAL,
					       "unexpected file content");
			}
		}

		scnprintf(fname, sizeof(fname), "file-%02zu", size);

		expect_success(strcmp(d->d_name, fname) == 0 ? 0 : -EINVAL,
			       "unexpected file content");

		free(buf);

		/*
		 * For select files, we test unreaddir once and check if
		 * we read back same element on repeated readdir
		 */
		for (j = 0; j < ARRAY_SIZE(files); j++) {
			if (size == files[j]) {
				ret = unreaddir(dir, d);
				expect_success(ret, "unreaddir");
				files[j] = -1;
				break;
			}
		}
		if (j == ARRAY_SIZE(files))
			i++;
	}

	expect_success(i == 90 ? 0 : -EINVAL,
		       "unexpected file count iterating directory: %u", i);

	ret = make_directory("test/a/b/c/d/e/f/g/h/i/j/k/l");
	expect_success(ret, "make_directory()");

	if (!ret) {
		const char hello[] = "hello";
		char *buf;

		ret = write_file("test/a/b/c/d/e/f/g/h/i/j/k/l/FILE",
				 ARRAY_AND_SIZE(hello));
		expect_success(ret, "write_file()");

		buf = read_file("test/a/b/c/d/e/f/g/h/i/j/k/l/FILE", NULL);
		if (expect_ptrok(buf, "read_file()")) {
			expect_success(memcmp(buf, ARRAY_AND_SIZE(hello)),
				       "read_file() content");
		}

		free(buf);
	}

out:
	popd(oldpwd);
	free(content);

	closedir(dir);

	ret = unlink_recursive(dname, NULL);
	expect_success(ret, "unlinking directory");

	dir = opendir(dname);
	expect_fail(dir ? 0 : -EISDIR, "opening removed directory");
	free(dname);
}
bselftest(core, test_ramfs);
