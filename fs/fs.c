/*
 * fs.c - posix like file functions
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <driver.h>
#include <errno.h>
#include <malloc.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <fcntl.h>
#include <xfuncs.h>
#include <init.h>
#include <module.h>
#include <libbb.h>
#include <magicvar.h>
#include <environment.h>
#include <libgen.h>
#include <block.h>
#include <libfile.h>
#include <parseopt.h>

char *mkmodestr(unsigned long mode, char *str)
{
	static const char *l = "xwr";
	int mask = 1, i;
	char c;

	switch (mode & S_IFMT) {
		case S_IFDIR:    str[0] = 'd'; break;
		case S_IFBLK:    str[0] = 'b'; break;
		case S_IFCHR:    str[0] = 'c'; break;
		case S_IFIFO:    str[0] = 'f'; break;
		case S_IFLNK:    str[0] = 'l'; break;
		case S_IFSOCK:   str[0] = 's'; break;
		case S_IFREG:    str[0] = '-'; break;
		default:         str[0] = '?';
	}

	for(i = 0; i < 9; i++) {
		c = l[i%3];
		str[9-i] = (mode & mask)?c:'-';
		mask = mask<<1;
	}

	if(mode & S_ISUID) str[3] = (mode & S_IXUSR)?'s':'S';
	if(mode & S_ISGID) str[6] = (mode & S_IXGRP)?'s':'S';
	if(mode & S_ISVTX) str[9] = (mode & S_IXOTH)?'t':'T';
	str[10] = '\0';
	return str;
}
EXPORT_SYMBOL(mkmodestr);

static char *cwd;

static FILE *files;

static int init_fs(void)
{
	cwd = xzalloc(PATH_MAX);
	*cwd = '/';

	files = xzalloc(sizeof(FILE) * MAX_FILES);

	return 0;
}

postcore_initcall(init_fs);

char *normalise_path(const char *pathname)
{
	char *path = xzalloc(strlen(pathname) + strlen(cwd) + 2);
        char *in, *out, *slashes[32];
	int sl = 0;

	debug("in: %s\n", pathname);

	if (*pathname != '/')
		strcpy(path, cwd);
	strcat(path, "/");
	strcat(path, pathname);

	slashes[0] = in = out = path;

        while (*in) {
                if(*in == '/') {
			slashes[sl++] = out;
                        *out++ = *in++;
                        while(*in == '/')
                                in++;
                } else {
			if (*in == '.' && (*(in + 1) == '/' || !*(in + 1))) {
				sl--;
				if (sl < 0)
					sl = 0;
				out = slashes[sl];
				in++;
				continue;
			}
			if (*in == '.' && *(in + 1) == '.') {
				sl -= 2;
				if (sl < 0)
					sl = 0;
				out = slashes[sl];
				in += 2;
				continue;
			}
                        *out++ = *in++;
                }
        }

	*out-- = 0;

        /*
         * Remove trailing slash
         */
        if (*out == '/')
                *out = 0;

	if (!*path) {
		*path = '/';
		*(path + 1) = 0;
	}

	return path;
}
EXPORT_SYMBOL(normalise_path);

static int __lstat(const char *filename, struct stat *s);
static struct fs_device_d *get_fsdevice_by_path(const char *path);

static char *__canonicalize_path(const char *_pathname, int level)
{
	char *path, *freep;
	char *outpath;
	int ret;
	struct stat s;

	if (level > 10)
		return ERR_PTR(-ELOOP);

	path = freep = xstrdup(_pathname);

	if (*path == '/' || !strcmp(cwd, "/"))
		outpath = xstrdup("");
	else
		outpath = __canonicalize_path(cwd, level + 1);

	while (1) {
		char *p = strsep(&path, "/");
		char *tmp;
		char link[PATH_MAX] = {};
		struct fs_device_d *fsdev;

		if (!p)
			break;
		if (p[0] == '\0')
			continue;
		if (!strcmp(p, "."))
			continue;
		if (!strcmp(p, "..")) {
			tmp = xstrdup(dirname(outpath));
			free(outpath);
			outpath = tmp;
			continue;
		}

		tmp = basprintf("%s/%s", outpath, p);
		free(outpath);
		outpath = tmp;

		/*
		 * Don't bother filesystems without link support
		 * with an additional stat() call.
		 */
		fsdev = get_fsdevice_by_path(outpath);
		if (!fsdev || !fsdev->driver->readlink)
			continue;

		ret = __lstat(outpath, &s);
		if (ret)
			goto out;

		if (!S_ISLNK(s.st_mode))
			continue;

		ret = readlink(outpath, link, PATH_MAX - 1);
		if (ret < 0)
			goto out;

		if (link[0] == '/') {
			free(outpath);
			outpath = __canonicalize_path(link, level + 1);
		} else {
			tmp = basprintf("%s/%s", dirname(outpath), link);
			free(outpath);
			outpath = __canonicalize_path(tmp, level + 1);
			free(tmp);
		}

		if (IS_ERR(outpath))
			goto out;
	}
out:
	free(freep);

	if (!*outpath) {
		free(outpath);
		outpath = xstrdup("/");
	}

	return outpath;
}

/*
 * canonicalize_path - resolve links in path
 * @pathname: The input path
 *
 * This function resolves all links in @pathname and returns
 * a path without links in it.
 *
 * Return: Path with links resolved. Allocated, must be freed after use.
 */
char *canonicalize_path(const char *pathname)
{
	char *r, *p = __canonicalize_path(pathname, 0);

	if (IS_ERR(p))
		return ERR_CAST(p);

	r = normalise_path(p);
	free(p);

	return r;
}

/*
 * canonicalize_dir - resolve links in path
 * @pathname: The input path
 *
 * This function resolves all links except the last one. Needed to give
 * access to the link itself.
 *
 * Return: Path with links resolved. Allocated, must be freed after use.
 */
static char *canonicalize_dir(const char *pathname)
{
	char *f, *d, *r, *ret, *p;
	char *freep1, *freep2;

	freep1 = xstrdup(pathname);
	freep2 = xstrdup(pathname);
	f = basename(freep1);
	d = dirname(freep2);

	p = __canonicalize_path(d, 0);
	if (IS_ERR(p)) {
		ret = ERR_CAST(p);
		goto out;
	}

	r = basprintf("%s/%s", p, f);

	ret = normalise_path(r);

	free(r);
	free(p);
out:
	free(freep1);
	free(freep2);

	return ret;
}

LIST_HEAD(fs_device_list);
static struct fs_device_d *fs_dev_root;

static struct fs_device_d *get_fsdevice_by_path(const char *path)
{
	struct fs_device_d *fsdev = NULL;

	for_each_fs_device(fsdev) {
		int len = strlen(fsdev->path);
		if (!strncmp(path, fsdev->path, len) &&
				(path[len] == '/' || path[len] == 0))
			return fsdev;
	}

	return fs_dev_root;
}

/*
 * get_cdev_by_mountpath - return the cdev the given path
 *                         is mounted on
 */
struct cdev *get_cdev_by_mountpath(const char *path)
{
	struct fs_device_d *fsdev;

	fsdev = get_fsdevice_by_path(path);

	return fsdev->cdev;
}

char *get_mounted_path(const char *path)
{
	struct fs_device_d *fdev;

	fdev = get_fsdevice_by_path(path);

	return fdev->path;
}

static FILE *get_file(void)
{
	int i;

	for (i = 3; i < MAX_FILES; i++) {
		if (!files[i].in_use) {
			memset(&files[i], 0, sizeof(FILE));
			files[i].in_use = 1;
			files[i].no = i;
			return &files[i];
		}
	}
	return NULL;
}

static void put_file(FILE *f)
{
	free(f->path);
	f->path = NULL;
	f->in_use = 0;
}

static int check_fd(int fd)
{
	if (fd < 0 || fd >= MAX_FILES || !files[fd].in_use) {
		errno = EBADF;
		return -errno;
	}

	return 0;
}

#ifdef CONFIG_FS_AUTOMOUNT

#define AUTOMOUNT_IS_FILE (1 << 0)

struct automount {
	char *path;
	char *cmd;
	struct list_head list;
	unsigned int flags;
};

static LIST_HEAD(automount_list);

void automount_remove(const char *_path)
{
	char *path = normalise_path(_path);
	struct automount *am;

	list_for_each_entry(am, &automount_list, list) {
		if (!strcmp(path, am->path))
			goto found;
	}

	return;
found:
	list_del(&am->list);
	free(am->path);
	free(am->cmd);
	free(am);
}
EXPORT_SYMBOL(automount_remove);

int automount_add(const char *path, const char *cmd)
{
	struct automount *am = xzalloc(sizeof(*am));
	struct stat s;
	int ret;

	am->path = normalise_path(path);
	am->cmd = xstrdup(cmd);

	automount_remove(am->path);

	ret = stat(path, &s);
	if (!ret) {
		/*
		 * If it exists it must be a directory
		 */
		if (!S_ISDIR(s.st_mode))
			return -ENOTDIR;
	} else {
		am->flags |= AUTOMOUNT_IS_FILE;
	}

	list_add_tail(&am->list, &automount_list);

	return 0;
}
EXPORT_SYMBOL(automount_add);

void cdev_create_default_automount(struct cdev *cdev)
{
	char *path, *cmd;

	path = basprintf("/mnt/%s", cdev->name);
	cmd = basprintf("mount %s", cdev->name);

	make_directory(path);
	automount_add(path, cmd);

	free(cmd);
	free(path);
}

void automount_print(void)
{
	struct automount *am;

	list_for_each_entry(am, &automount_list, list)
		printf("%-20s %s\n", am->path, am->cmd);
}
EXPORT_SYMBOL(automount_print);

static void automount_mount(const char *path, int instat)
{
	struct automount *am;
	int ret;
	static int in_automount;

	if (in_automount)
		return;

	in_automount++;

	if (fs_dev_root != get_fsdevice_by_path(path))
		goto out;

	list_for_each_entry(am, &automount_list, list) {
		int len_path = strlen(path);
		int len_am_path = strlen(am->path);

		/*
		 * stat is a bit special. We do not want to trigger
		 * automount when someone calls stat() on the automount
		 * directory itself.
		 */
		if (instat && !(am->flags & AUTOMOUNT_IS_FILE) &&
				len_path == len_am_path) {
			continue;
		}

		if (len_path < len_am_path)
			continue;

		if (strncmp(path, am->path, len_am_path))
			continue;

		if (*(path + len_am_path) != 0 && *(path + len_am_path) != '/')
			continue;

		setenv("automount_path", am->path);
		export("automount_path");
		ret = run_command(am->cmd);
		setenv("automount_path", NULL);

		if (ret)
			printf("running automount command '%s' failed\n",
					am->cmd);

		break;
	}
out:
	in_automount--;
}

BAREBOX_MAGICVAR(automount_path, "mountpath passed to automount scripts");

#else
static void automount_mount(const char *path, int instat)
{
}
#endif /* CONFIG_FS_AUTOMOUNT */

static struct fs_device_d *get_fs_device_and_root_path(char **path)
{
	struct fs_device_d *fsdev;

	automount_mount(*path, 0);

	fsdev = get_fsdevice_by_path(*path);
	if (!fsdev)
		return NULL;
	if (fsdev != fs_dev_root)
		*path += strlen(fsdev->path);

	return fsdev;
}

static int dir_is_empty(const char *pathname)
{
	DIR *dir;
	struct dirent *d;
	int ret = 1;

	dir = opendir(pathname);
	if (!dir) {
		errno = ENOENT;
		return -ENOENT;
	}

	while ((d = readdir(dir))) {
		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
				continue;
		ret = 0;
		break;
	}

	closedir(dir);
	return ret;
}

static int parent_check_directory(const char *path)
{
	struct stat s;
	int ret;
	char *dir = dirname(xstrdup(path));

	ret = lstat(dir, &s);

	free(dir);

	if (ret)
		return -ENOENT;

	if (!S_ISDIR(s.st_mode))
		return -ENOTDIR;

	return 0;
}

const char *getcwd(void)
{
	return cwd;
}
EXPORT_SYMBOL(getcwd);

int chdir(const char *pathname)
{
	char *p = normalise_path(pathname);
	int ret;
	struct stat s;

	ret = stat(p, &s);
	if (ret)
		goto out;

	if (!S_ISDIR(s.st_mode)) {
		ret = -ENOTDIR;
		goto out;
	}

	automount_mount(p, 0);

	strcpy(cwd, p);

out:
	free(p);

	if (ret)
		errno = -ret;

	return ret;
}
EXPORT_SYMBOL(chdir);

int unlink(const char *pathname)
{
	struct fs_device_d *fsdev;
	struct fs_driver_d *fsdrv;
	char *p = canonicalize_dir(pathname);
	char *freep = p;
	int ret;
	struct stat s;

	ret = lstat(p, &s);
	if (ret)
		goto out;

	if (S_ISDIR(s.st_mode)) {
		ret = -EISDIR;
		goto out;
	}

	fsdev = get_fs_device_and_root_path(&p);
	if (!fsdev) {
		ret = -ENOENT;
		goto out;
	}
	fsdrv = fsdev->driver;

	if (!fsdrv->unlink) {
		ret = -ENOSYS;
		goto out;
	}

	ret = fsdrv->unlink(&fsdev->dev, p);
	if (ret)
		errno = -ret;
out:
	free(freep);
	if (ret)
		errno = -ret;
	return ret;
}
EXPORT_SYMBOL(unlink);

int open(const char *pathname, int flags, ...)
{
	struct fs_device_d *fsdev;
	struct fs_driver_d *fsdrv;
	FILE *f;
	int exist_err = 0;
	struct stat s;
	char *path;
	char *freep;
	int ret;

	path = canonicalize_path(pathname);
	if (IS_ERR(path)) {
		ret = PTR_ERR(path);
		goto out2;
	}

	exist_err = stat(path, &s);

	freep = path;

	if (!exist_err && S_ISDIR(s.st_mode)) {
		ret = -EISDIR;
		goto out1;
	}

	if (exist_err && !(flags & O_CREAT)) {
		ret = exist_err;
		goto out1;
	}

	if (exist_err) {
		ret = parent_check_directory(path);
		if (ret)
			goto out1;
	}

	f = get_file();
	if (!f) {
		ret = -EMFILE;
		goto out1;
	}

	fsdev = get_fs_device_and_root_path(&path);
	if (!fsdev) {
		ret = -ENOENT;
		goto out;
	}

	fsdrv = fsdev->driver;

	f->fsdev = fsdev;
	f->flags = flags;

	if ((flags & O_ACCMODE) && !fsdrv->write) {
		ret = -EROFS;
		goto out;
	}

	if (exist_err) {
		if (NULL != fsdrv->create)
			ret = fsdrv->create(&fsdev->dev, path,
					S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);
		else
			ret = -EROFS;
		if (ret)
			goto out;
	}

	f->path = xstrdup(path);

	ret = fsdrv->open(&fsdev->dev, f, path);
	if (ret)
		goto out;

	if (flags & O_TRUNC) {
		ret = fsdrv->truncate(&fsdev->dev, f, 0);
		f->size = 0;
		if (ret)
			goto out;
	}

	if (flags & O_APPEND)
		f->pos = f->size;

	free(freep);
	return f->no;

out:
	put_file(f);
out1:
	free(freep);
out2:
	if (ret)
		errno = -ret;
	return ret;
}
EXPORT_SYMBOL(open);

int creat(const char *pathname, mode_t mode)
{
	return open(pathname, O_CREAT | O_WRONLY | O_TRUNC);
}
EXPORT_SYMBOL(creat);

int ftruncate(int fd, loff_t length)
{
	struct fs_driver_d *fsdrv;
	FILE *f;
	int ret;

	if (check_fd(fd))
		return -errno;

	f = &files[fd];

	fsdrv = f->fsdev->driver;

	ret = fsdrv->truncate(&f->fsdev->dev, f, length);
	if (ret)
		return ret;

	f->size = length;

	return 0;
}

int ioctl(int fd, int request, void *buf)
{
	struct fs_driver_d *fsdrv;
	FILE *f;
	int ret;

	if (check_fd(fd))
		return -errno;

	f = &files[fd];

	fsdrv = f->fsdev->driver;

	if (fsdrv->ioctl)
		ret = fsdrv->ioctl(&f->fsdev->dev, f, request, buf);
	else
		ret = -ENOSYS;
	if (ret)
		errno = -ret;
	return ret;
}

static ssize_t __read(FILE *f, void *buf, size_t count)
{
	struct fs_driver_d *fsdrv;
	int ret;

	if ((f->flags & O_ACCMODE) == O_WRONLY) {
		ret = -EBADF;
		goto out;
	}

	fsdrv = f->fsdev->driver;

	if (f->size != FILE_SIZE_STREAM && f->pos + count > f->size)
		count = f->size - f->pos;

	if (!count)
		return 0;

	ret = fsdrv->read(&f->fsdev->dev, f, buf, count);
out:
	if (ret < 0)
		errno = -ret;
	return ret;
}

ssize_t pread(int fd, void *buf, size_t count, loff_t offset)
{
	loff_t pos;
	FILE *f;
	int ret;

	if (check_fd(fd))
		return -errno;

	f = &files[fd];

	pos = f->pos;
	f->pos = offset;
	ret = __read(f, buf, count);
	f->pos = pos;

	return ret;
}
EXPORT_SYMBOL(pread);

ssize_t read(int fd, void *buf, size_t count)
{
	FILE *f;
	int ret;

	if (check_fd(fd))
		return -errno;

	f = &files[fd];

	ret = __read(f, buf, count);

	if (ret > 0)
		f->pos += ret;
	return ret;
}
EXPORT_SYMBOL(read);

static ssize_t __write(FILE *f, const void *buf, size_t count)
{
	struct fs_driver_d *fsdrv;
	int ret;

	if (!(f->flags & O_ACCMODE)) {
		ret = -EBADF;
		goto out;
	}

	fsdrv = f->fsdev->driver;
	if (f->size != FILE_SIZE_STREAM && f->pos + count > f->size) {
		ret = fsdrv->truncate(&f->fsdev->dev, f, f->pos + count);
		if (ret) {
			if (ret != -ENOSPC)
				goto out;
			count = f->size - f->pos;
			if (!count)
				goto out;
		} else {
			f->size = f->pos + count;
		}
	}
	ret = fsdrv->write(&f->fsdev->dev, f, buf, count);
out:
	if (ret < 0)
		errno = -ret;
	return ret;
}

ssize_t pwrite(int fd, const void *buf, size_t count, loff_t offset)
{
	loff_t pos;
	FILE *f;
	int ret;

	if (check_fd(fd))
		return -errno;

	f = &files[fd];

	pos = f->pos;
	f->pos = offset;
	ret = __write(f, buf, count);
	f->pos = pos;

	return ret;
}
EXPORT_SYMBOL(pwrite);

ssize_t write(int fd, const void *buf, size_t count)
{
	FILE *f;
	int ret;

	if (check_fd(fd))
		return -errno;

	f = &files[fd];

	ret = __write(f, buf, count);

	if (ret > 0)
		f->pos += ret;
	return ret;
}
EXPORT_SYMBOL(write);

int flush(int fd)
{
	struct fs_driver_d *fsdrv;
	FILE *f;
	int ret;

	if (check_fd(fd))
		return -errno;

	f = &files[fd];

	fsdrv = f->fsdev->driver;
	if (fsdrv->flush)
		ret = fsdrv->flush(&f->fsdev->dev, f);
	else
		ret = 0;

	if (ret)
		errno = -ret;

	return ret;
}

loff_t lseek(int fildes, loff_t offset, int whence)
{
	struct fs_driver_d *fsdrv;
	FILE *f;
	loff_t pos;
	int ret;

	if (check_fd(fildes))
		return -1;

	f = &files[fildes];
	fsdrv = f->fsdev->driver;
	if (!fsdrv->lseek) {
		ret = -ENOSYS;
		goto out;
	}

	ret = -EINVAL;

	switch (whence) {
	case SEEK_SET:
		if (f->size != FILE_SIZE_STREAM && offset > f->size)
			goto out;
		if (offset < 0)
			goto out;
		pos = offset;
		break;
	case SEEK_CUR:
		if (f->size != FILE_SIZE_STREAM && offset + f->pos > f->size)
			goto out;
		pos = f->pos + offset;
		break;
	case SEEK_END:
		if (offset > 0)
			goto out;
		pos = f->size + offset;
		break;
	default:
		goto out;
	}

	pos = fsdrv->lseek(&f->fsdev->dev, f, pos);
	if (pos < 0) {
		errno = -pos;
		return -1;
	}

	return pos;

out:
	if (ret)
		errno = -ret;

	return -1;
}
EXPORT_SYMBOL(lseek);

int erase(int fd, loff_t count, loff_t offset)
{
	struct fs_driver_d *fsdrv;
	FILE *f;
	int ret;

	if (check_fd(fd))
		return -errno;
	f = &files[fd];
	if (offset >= f->size)
		return 0;
	if (count == ERASE_SIZE_ALL || count > f->size - offset)
		count = f->size - offset;
	if (count < 0)
		return -EINVAL;

	fsdrv = f->fsdev->driver;
	if (fsdrv->erase)
		ret = fsdrv->erase(&f->fsdev->dev, f, count, offset);
	else
		ret = -ENOSYS;

	if (ret)
		errno = -ret;

	return ret;
}
EXPORT_SYMBOL(erase);

int protect(int fd, size_t count, loff_t offset, int prot)
{
	struct fs_driver_d *fsdrv;
	FILE *f;
	int ret;

	if (check_fd(fd))
		return -errno;
	f = &files[fd];
	if (offset >= f->size)
		return 0;
	if (count > f->size - offset)
		count = f->size - offset;

	fsdrv = f->fsdev->driver;
	if (fsdrv->protect)
		ret = fsdrv->protect(&f->fsdev->dev, f, count, offset, prot);
	else
		ret = -ENOSYS;

	if (ret)
		errno = -ret;

	return ret;
}
EXPORT_SYMBOL(protect);

int protect_file(const char *file, int prot)
{
	int fd, ret;

	fd = open(file, O_WRONLY);
	if (fd < 0)
		return fd;

	ret = protect(fd, ~0, 0, prot);

	close(fd);

	return ret;
}

void *memmap(int fd, int flags)
{
	struct fs_driver_d *fsdrv;
	FILE *f;
	void *retp = (void *)-1;
	int ret;

	if (check_fd(fd))
		return retp;

	f = &files[fd];

	fsdrv = f->fsdev->driver;

	if (fsdrv->memmap)
		ret = fsdrv->memmap(&f->fsdev->dev, f, &retp, flags);
	else
		ret = -EINVAL;

	if (ret)
		errno = -ret;

	return retp;
}
EXPORT_SYMBOL(memmap);

int close(int fd)
{
	struct fs_driver_d *fsdrv;
	FILE *f;
	int ret;

	if (check_fd(fd))
		return -errno;

	f = &files[fd];

	fsdrv = f->fsdev->driver;
	ret = fsdrv->close(&f->fsdev->dev, f);

	put_file(f);

	if (ret)
		errno = -ret;

	return ret;
}
EXPORT_SYMBOL(close);

int readlink(const char *pathname, char *buf, size_t bufsiz)
{
	struct fs_driver_d *fsdrv;
	struct fs_device_d *fsdev;
	char *p = canonicalize_dir(pathname);
	char *freep = p;
	int ret;
	struct stat s;

	ret = lstat(pathname, &s);
	if (ret)
		goto out;

	if (!S_ISLNK(s.st_mode)) {
		ret = -EINVAL;
		goto out;
	}

	fsdev = get_fs_device_and_root_path(&p);
	if (!fsdev) {
		ret = -ENODEV;
		goto out;
	}
	fsdrv = fsdev->driver;

	if (fsdrv->readlink)
		ret = fsdrv->readlink(&fsdev->dev, p, buf, bufsiz);
	else
		ret = -ENOSYS;

	if (ret)
		goto out;

out:
	free(freep);

	if (ret)
		errno = -ret;

	return ret;
}
EXPORT_SYMBOL(readlink);

int symlink(const char *pathname, const char *newpath)
{
	struct fs_driver_d *fsdrv;
	struct fs_device_d *fsdev;
	char *p;
	int ret;
	struct stat s;

	p = canonicalize_path(newpath);
	if (IS_ERR(p)) {
		ret = PTR_ERR(p);
		goto out;
	}

	ret = lstat(p, &s);
	if (!ret) {
		ret = -EEXIST;
		goto out;
	}

	fsdev = get_fs_device_and_root_path(&p);
	if (!fsdev) {
		ret = -ENODEV;
		goto out;
	}
	fsdrv = fsdev->driver;

	if (fsdrv->symlink) {
		ret = fsdrv->symlink(&fsdev->dev, pathname, p);
	} else {
		ret = -EPERM;
	}

out:
	free(p);
	if (ret)
		errno = -ret;

	return ret;
}
EXPORT_SYMBOL(symlink);

static int fs_match(struct device_d *dev, struct driver_d *drv)
{
	return strcmp(dev->name, drv->name) ? -1 : 0;
}

static int fs_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev = dev_to_fs_device(dev);
	struct driver_d *drv = dev->driver;
	struct fs_driver_d *fsdrv = container_of(drv, struct fs_driver_d, drv);
	int ret;

	ret = dev->driver->probe(dev);
	if (ret)
		return ret;

	fsdev->driver = fsdrv;

	list_add_tail(&fsdev->list, &fs_device_list);

	if (!fs_dev_root)
		fs_dev_root = fsdev;

	return 0;
}

static void fs_remove(struct device_d *dev)
{
	struct fs_device_d *fsdev = dev_to_fs_device(dev);

	if (fsdev->dev.driver) {
		dev->driver->remove(dev);
		list_del(&fsdev->list);
	}

	free(fsdev->path);
	free(fsdev->options);

	if (fsdev == fs_dev_root)
		fs_dev_root = NULL;

	if (fsdev->cdev)
		cdev_close(fsdev->cdev);

	if (fsdev->loop)
		cdev_remove_loop(fsdev->cdev);

	free(fsdev->backingstore);
	free(fsdev);
}

struct bus_type fs_bus = {
	.name = "fs",
	.match = fs_match,
	.probe = fs_probe,
	.remove = fs_remove,
};

static int fs_bus_init(void)
{
	return bus_register(&fs_bus);
}
pure_initcall(fs_bus_init);

int register_fs_driver(struct fs_driver_d *fsdrv)
{
	fsdrv->drv.bus = &fs_bus;
	register_driver(&fsdrv->drv);

	return 0;
}
EXPORT_SYMBOL(register_fs_driver);

static const char *detect_fs(const char *filename, const char *fsoptions)
{
	enum filetype type;
	struct driver_d *drv;
	struct fs_driver_d *fdrv;
	bool loop = false;
	unsigned long long offset = 0;

	parseopt_b(fsoptions, "loop", &loop);
	parseopt_llu_suffix(fsoptions, "offset", &offset);
	if (loop)
		type = file_name_detect_type_offset(filename, offset);
	else
		type = cdev_detect_type(filename);

	if (type == filetype_unknown)
		return NULL;

	bus_for_each_driver(&fs_bus, drv) {
		fdrv = drv_to_fs_driver(drv);

		if (type == fdrv->type)
			return drv->name;
	}

	return NULL;
}

int fsdev_open_cdev(struct fs_device_d *fsdev)
{
	unsigned long long offset = 0;

	parseopt_b(fsdev->options, "loop", &fsdev->loop);
	parseopt_llu_suffix(fsdev->options, "offset", &offset);
	if (fsdev->loop)
		fsdev->cdev = cdev_create_loop(fsdev->backingstore, O_RDWR,
					       offset);
	else
		fsdev->cdev = cdev_open(fsdev->backingstore, O_RDWR);
	if (!fsdev->cdev)
		return -EINVAL;

	fsdev->dev.parent = fsdev->cdev->dev;
	fsdev->parent_device = fsdev->cdev->dev;

	return 0;
}

/*
 * Mount a device to a directory.
 * We do this by registering a new device on which the filesystem
 * driver will match.
 */
int mount(const char *device, const char *fsname, const char *_path,
		const char *fsoptions)
{
	struct fs_device_d *fsdev;
	int ret;
	char *path = normalise_path(_path);

	if (!fsoptions)
		fsoptions = "";

	debug("mount: %s on %s type %s, options=%s\n",
			device, path, fsname, fsoptions);

	if (fs_dev_root) {
		struct stat s;

		fsdev = get_fsdevice_by_path(path);
		if (fsdev != fs_dev_root) {
			printf("sorry, no nested mounts\n");
			ret = -EBUSY;
			goto err_free_path;
		}
		ret = lstat(path, &s);
		if (ret)
			goto err_free_path;
		if (!S_ISDIR(s.st_mode)) {
			ret = -ENOTDIR;
			goto err_free_path;
		}
	} else {
		/* no mtab, so we only allow to mount on '/' */
		if (*path != '/' || *(path + 1)) {
			ret = -ENOTDIR;
			goto err_free_path;
		}
	}

	if (!fsname)
		fsname = detect_fs(device, fsoptions);

	if (!fsname)
		return -ENOENT;

	fsdev = xzalloc(sizeof(struct fs_device_d));
	fsdev->backingstore = xstrdup(device);
	safe_strncpy(fsdev->dev.name, fsname, MAX_DRIVER_NAME);
	fsdev->dev.id = get_free_deviceid(fsdev->dev.name);
	fsdev->path = xstrdup(path);
	fsdev->dev.bus = &fs_bus;
	fsdev->options = xstrdup(fsoptions);

	ret = register_device(&fsdev->dev);
	if (ret)
		goto err_register;

	if (!fsdev->dev.driver) {
		/*
		 * Driver didn't accept the device or no driver for this
		 * device. Bail out
		 */
		ret = -EINVAL;
		goto err_no_driver;
	}

	if (!fsdev->linux_rootarg && fsdev->cdev && fsdev->cdev->partuuid[0] != 0) {
		char *str = basprintf("root=PARTUUID=%s",
					fsdev->cdev->partuuid);

		fsdev_set_linux_rootarg(fsdev, str);
	}

	free(path);

	return 0;

err_no_driver:
	unregister_device(&fsdev->dev);
err_register:
	fs_remove(&fsdev->dev);
err_free_path:
	free(path);

	errno = -ret;

	return ret;
}
EXPORT_SYMBOL(mount);

static int fsdev_umount(struct fs_device_d *fsdev)
{
	return unregister_device(&fsdev->dev);
}

/**
 * umount_by_cdev Use a cdev struct to umount all mounted filesystems
 * @param cdev cdev to the according device
 * @return 0 on success or if cdev was not mounted, -errno otherwise
 */
int umount_by_cdev(struct cdev *cdev)
{
	struct fs_device_d *fs;
	struct fs_device_d *fs_tmp;
	int first_error = 0;

	for_each_fs_device_safe(fs_tmp, fs) {
		int ret;

		if (fs->cdev == cdev) {
			ret = fsdev_umount(fs);
			if (ret) {
				pr_err("Failed umounting %s, %d, continuing anyway\n",
				       fs->path, ret);
				if (!first_error)
					first_error = ret;
			}
		}
	}

	return first_error;
}
EXPORT_SYMBOL(umount_by_cdev);

int umount(const char *pathname)
{
	struct fs_device_d *fsdev = NULL, *f;
	char *p = normalise_path(pathname);

	for_each_fs_device(f) {
		if (!strcmp(p, f->path)) {
			fsdev = f;
			break;
		}
	}

	if (!fsdev) {
		struct cdev *cdev = cdev_open(p, O_RDWR);

		if (cdev) {
			free(p);
			cdev_close(cdev);
			return umount_by_cdev(cdev);
		}
	}

	free(p);

	if (f == fs_dev_root && !list_is_singular(&fs_device_list)) {
		errno = EBUSY;
		return -EBUSY;
	}

	if (!fsdev) {
		errno = EFAULT;
		return -EFAULT;
	}

	return fsdev_umount(fsdev);
}
EXPORT_SYMBOL(umount);

DIR *opendir(const char *pathname)
{
	DIR *dir = NULL;
	struct fs_device_d *fsdev;
	struct fs_driver_d *fsdrv;
	char *p = canonicalize_path(pathname);
	char *freep = p;
	int ret;
	struct stat s;

	ret = stat(pathname, &s);
	if (ret)
		goto out;

	if (!S_ISDIR(s.st_mode)) {
		ret = -ENOTDIR;
		goto out;
	}

	fsdev = get_fs_device_and_root_path(&p);
	if (!fsdev) {
		ret = -ENOENT;
		goto out;
	}
	fsdrv = fsdev->driver;

	debug("opendir: fsdrv: %p\n",fsdrv);

	dir = fsdrv->opendir(&fsdev->dev, p);
	if (dir) {
		dir->dev = &fsdev->dev;
		dir->fsdrv = fsdrv;
	} else {
		/*
		 * FIXME: The fs drivers should return ERR_PTR here so that
		 * we are able to forward the error
		 */
		ret = -EINVAL;
	}

out:
	free(freep);

	if (ret)
		errno = -ret;

	return dir;
}
EXPORT_SYMBOL(opendir);

struct dirent *readdir(DIR *dir)
{
	struct dirent *ent;

	if (!dir)
		return NULL;

	ent = dir->fsdrv->readdir(dir->dev, dir);

	if (!ent)
		errno = EBADF;

	return ent;
}
EXPORT_SYMBOL(readdir);

int closedir(DIR *dir)
{
	int ret;

	if (!dir) {
		errno = EBADF;
		return -EBADF;
	}

	ret = dir->fsdrv->closedir(dir->dev, dir);
	if (ret)
		errno = -ret;

	return ret;
}
EXPORT_SYMBOL(closedir);

int stat(const char *filename, struct stat *s)
{
	char *path = canonicalize_path(filename);
	int ret;

	if (IS_ERR(path))
		return PTR_ERR(path);

	ret = lstat(path, s);

	free(path);

	return ret;
}
EXPORT_SYMBOL(stat);

static int __lstat(const char *filename, struct stat *s)
{
	struct fs_driver_d *fsdrv;
	struct fs_device_d *fsdev;
	char *f = normalise_path(filename);
	char *freep = f;
	int ret;

	automount_mount(f, 1);

	memset(s, 0, sizeof(struct stat));

	fsdev = get_fsdevice_by_path(f);
	if (!fsdev) {
		ret = -ENOENT;
		goto out;
	}

	if (fsdev != fs_dev_root && strcmp(f, fsdev->path))
		f += strlen(fsdev->path);
	else
		fsdev = fs_dev_root;

	fsdrv = fsdev->driver;

	if (*f == 0)
		f = "/";

	ret = fsdrv->stat(&fsdev->dev, f, s);
out:
	free(freep);

	if (ret)
		errno = -ret;

	return ret;
}

int lstat(const char *filename, struct stat *s)
{
	char *f = canonicalize_dir(filename);
	int ret;

	if (IS_ERR(f))
		return PTR_ERR(f);

	ret = __lstat(f, s);

	free(f);

	return ret;
}
EXPORT_SYMBOL(lstat);

int fstat(int fd, struct stat *s)
{
	FILE *f;
	struct fs_device_d *fsdev;

	if (check_fd(fd))
		return -errno;

	f = &files[fd];

	fsdev = f->fsdev;

	return fsdev->driver->stat(&fsdev->dev, f->path, s);
}
EXPORT_SYMBOL(fstat);

int mkdir (const char *pathname, mode_t mode)
{
	struct fs_driver_d *fsdrv;
	struct fs_device_d *fsdev;
	char *p = normalise_path(pathname);
	char *freep = p;
	int ret;
	struct stat s;

	ret = parent_check_directory(p);
	if (ret)
		goto out;

	ret = stat(pathname, &s);
	if (!ret) {
		ret = -EEXIST;
		goto out;
	}

	fsdev = get_fs_device_and_root_path(&p);
	if (!fsdev) {
		ret = -ENOENT;
		goto out;
	}
	fsdrv = fsdev->driver;

	if (fsdrv->mkdir)
		ret = fsdrv->mkdir(&fsdev->dev, p);
	else
		ret = -EROFS;
out:
	free(freep);

	if (ret)
		errno = -ret;

	return ret;
}
EXPORT_SYMBOL(mkdir);

int rmdir (const char *pathname)
{
	struct fs_driver_d *fsdrv;
	struct fs_device_d *fsdev;
	char *p = normalise_path(pathname);
	char *freep = p;
	int ret;
	struct stat s;

	ret = lstat(pathname, &s);
	if (ret)
		goto out;
	if (!S_ISDIR(s.st_mode)) {
		ret = -ENOTDIR;
		goto out;
	}

	if (!dir_is_empty(pathname)) {
		ret = -ENOTEMPTY;
		goto out;
	}

	fsdev = get_fs_device_and_root_path(&p);
	if (!fsdev) {
		ret = -ENODEV;
		goto out;
	}
	fsdrv = fsdev->driver;

	if (fsdrv->rmdir)
		ret = fsdrv->rmdir(&fsdev->dev, p);
	else
		ret = -EROFS;
out:
	free(freep);

	if (ret)
		errno = -ret;

	return ret;
}
EXPORT_SYMBOL(rmdir);

static void memcpy_sz(void *dst, const void *src, size_t count, int rwsize)
{
	/* no rwsize specification given. Do whatever memcpy likes best */
	if (!rwsize) {
		memcpy(dst, src, count);
		return;
	}

	rwsize = rwsize >> O_RWSIZE_SHIFT;

	count /= rwsize;

	while (count-- > 0) {
		switch (rwsize) {
		case 1:
			*((u8 *)dst) = *((u8 *)src);
			break;
		case 2:
			*((u16 *)dst) = *((u16 *)src);
			break;
		case 4:
			*((u32  *)dst) = *((u32  *)src);
			break;
		case 8:
			*((u64  *)dst) = *((u64  *)src);
			break;
		}
		dst += rwsize;
		src += rwsize;
	}
}

ssize_t mem_read(struct cdev *cdev, void *buf, size_t count, loff_t offset, ulong flags)
{
	ulong size;
	struct device_d *dev;

	if (!cdev->dev || cdev->dev->num_resources < 1)
		return -1;
	dev = cdev->dev;

	size = min((resource_size_t)count,
			resource_size(&dev->resource[0]) -
			(resource_size_t)offset);
	memcpy_sz(buf, dev_get_mem_region(dev, 0) + offset, size, flags & O_RWSIZE_MASK);
	return size;
}
EXPORT_SYMBOL(mem_read);

ssize_t mem_write(struct cdev *cdev, const void *buf, size_t count, loff_t offset, ulong flags)
{
	ulong size;
	struct device_d *dev;

	if (!cdev->dev || cdev->dev->num_resources < 1)
		return -1;
	dev = cdev->dev;

	size = min((resource_size_t)count,
			resource_size(&dev->resource[0]) -
			(resource_size_t)offset);
	memcpy_sz(dev_get_mem_region(dev, 0) + offset, buf, size, flags & O_RWSIZE_MASK);
	return size;
}
EXPORT_SYMBOL(mem_write);

/*
 * cdev_get_mount_path - return the path a cdev is mounted on
 *
 * If a cdev is mounted return the path it's mounted on, NULL
 * otherwise.
 */
const char *cdev_get_mount_path(struct cdev *cdev)
{
	struct fs_device_d *fsdev;

	for_each_fs_device(fsdev) {
		if (fsdev->cdev && fsdev->cdev == cdev)
			return fsdev->path;
	}

	return NULL;
}

/*
 * cdev_mount_default - mount a cdev to the default path
 *
 * If a cdev is already mounted return the path it's mounted on, otherwise
 * mount it to /mnt/<cdevname> and return the path. Returns an error pointer
 * on failure.
 */
const char *cdev_mount_default(struct cdev *cdev, const char *fsoptions)
{
	const char *path;
	char *newpath, *devpath;
	int ret;

	/*
	 * If this cdev is already mounted somewhere use this path
	 * instead of mounting it again to avoid corruption on the
	 * filesystem. Note this ignores eventual fsoptions though.
	 */
	path = cdev_get_mount_path(cdev);
	if (path)
		return path;

	newpath = basprintf("/mnt/%s", cdev->name);
	make_directory(newpath);

	devpath = basprintf("/dev/%s", cdev->name);

	ret = mount(devpath, NULL, newpath, fsoptions);

	free(devpath);

	if (ret) {
		free(newpath);
		return ERR_PTR(ret);
	}

	return cdev_get_mount_path(cdev);
}

/*
 * mount_all - iterate over block devices and mount all devices we are able to
 */
void mount_all(void)
{
	struct device_d *dev;
	struct block_device *bdev;

	if (!IS_ENABLED(CONFIG_BLOCK))
		return;

	for_each_device(dev)
		device_detect(dev);

	for_each_block_device(bdev) {
		struct cdev *cdev = &bdev->cdev;

		list_for_each_entry(cdev, &bdev->dev->cdevs, devices_list)
			cdev_mount_default(cdev, NULL);
	}
}

void fsdev_set_linux_rootarg(struct fs_device_d *fsdev, const char *str)
{
	fsdev->linux_rootarg = xstrdup(str);

	dev_add_param_fixed(&fsdev->dev, "linux.bootargs", fsdev->linux_rootarg);
}

/**
 * path_get_linux_rootarg() - Given a path return a suitable root= option for
 *                            Linux
 * @path: The path
 *
 * Return: A string containing the root= option or an ERR_PTR. the returned
 *         string must be freed by the caller.
 */
char *path_get_linux_rootarg(const char *path)
{
	struct fs_device_d *fsdev;
	const char *str;

	fsdev = get_fsdevice_by_path(path);
	if (!fsdev)
		return ERR_PTR(-EINVAL);

	str = dev_get_param(&fsdev->dev, "linux.bootargs");
	if (!str)
		return ERR_PTR(-ENOSYS);

	return xstrdup(str);
}

/**
 * __is_tftp_fs() - return true when path is mounted on TFTP
 * @path: The path
 *
 * Do not use directly, use is_tftp_fs instead.
 *
 * Return: true when @path is on TFTP, false otherwise
 */
bool __is_tftp_fs(const char *path)
{
	struct fs_device_d *fsdev;

	fsdev = get_fsdevice_by_path(path);
	if (!fsdev)
		return false;

	if (strcmp(fsdev->driver->drv.name, "tftp"))
		return false;

	return true;
}
