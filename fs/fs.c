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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <fs.h>
#include <driver.h>
#include <errno.h>
#include <malloc.h>
#include <linux/stat.h>
#include <fcntl.h>
#include <xfuncs.h>
#include <init.h>
#include <module.h>
#include <libbb.h>

void *read_file(const char *filename, size_t *size)
{
	int fd;
	struct stat s;
	void *buf = NULL;

	if (stat(filename, &s))
		return NULL;

	buf = xzalloc(s.st_size + 1);

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		goto err_out;

	if (read(fd, buf, s.st_size) < s.st_size)
		goto err_out1;

	close(fd);

	if (size)
		*size = s.st_size;

	return buf;

err_out1:
	close(fd);
err_out:
	free(buf);
	return NULL;
}

EXPORT_SYMBOL(read_file);

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

static int init_cwd(void)
{
	cwd = xzalloc(PATH_MAX);
	*cwd = '/';
	return 0;
}

postcore_initcall(init_cwd);

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

static struct mtab_entry *mtab;

struct mtab_entry *get_mtab_entry_by_path(const char *_path)
{
	struct mtab_entry *match = NULL, *e = mtab;
	char *path, *tok;

	if (*_path != '/')
		return NULL;

	path = strdup(_path);

	tok = strchr(path + 1, '/');
	if (tok)
		*tok = 0;

	while (e) {
		if (!strcmp(path, e->path)) {
			match = e;
			break;
		}
		e = e->next;
	}

	free(path);

	return match ? match : mtab;
}

struct mtab_entry *mtab_next_entry(struct mtab_entry *e)
{
	if (!e)
		return mtab;
	return e->next;
}

const char *fsdev_get_mountpoint(struct fs_device_d *fsdev)
{
	return fsdev->mtab.path;
}

static FILE files[MAX_FILES];

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
	files[f->no].in_use = 0;
}

static int check_fd(int fd)
{
	if (fd < 0 || fd >= MAX_FILES || !files[fd].in_use) {
		errno = -EBADF;
		return errno;
	}

	return 0;
}

static struct device_d *get_fs_device_by_path(char **path)
{
	struct device_d *dev;
	struct mtab_entry *e;

	e = get_mtab_entry_by_path(*path);
	if (!e)
		return NULL;
	if (e != mtab)
		*path += strlen(e->path);

	dev = e->dev;

	return dev;
}

static int dir_is_empty(const char *pathname)
{
	DIR *dir;
	struct dirent *d;
	int ret = 1;

	dir = opendir(pathname);
	if (!dir) {
		errno = -ENOENT;
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

#define S_UB_IS_EMPTY		(1 << 31)
#define S_UB_EXISTS		(1 << 30)
#define S_UB_DOES_NOT_EXIST	(1 << 29)

/*
 * Helper function to check the prerequisites of a path given
 * to fs functions. Besides the flags above S_IFREG and S_IFDIR
 * can be passed in.
 */
static int path_check_prereq(const char *path, unsigned int flags)
{
	struct stat s;
	unsigned int m;

	if (stat(path, &s)) {
		if (flags & S_UB_DOES_NOT_EXIST)
			return 0;
		errno = -ENOENT;
		goto out;
	}

	if (flags & S_UB_DOES_NOT_EXIST) {
		errno = -EEXIST;
		goto out;
	}

	if (flags == S_UB_EXISTS)
		return 0;

	m = s.st_mode;

	if (S_ISDIR(m)) {
		if (flags & S_IFREG) {
			errno = -EISDIR;
			goto out;
		}
		if ((flags & S_UB_IS_EMPTY) && !dir_is_empty(path)) {
			errno = -ENOTEMPTY;
			goto out;
		}
	}
	if ((flags & S_IFDIR) && S_ISREG(m)) {
		errno = -ENOTDIR;
		goto out;
	}

	errno = 0;
out:
	return errno;
}

const char *getcwd(void)
{
	return cwd;
}
EXPORT_SYMBOL(getcwd);

int chdir(const char *pathname)
{
	char *p = normalise_path(pathname);
	errno = 0;

	if (path_check_prereq(p, S_IFDIR))
		goto out;

	strcpy(cwd, p);

	free(p);
out:
	return errno;
}
EXPORT_SYMBOL(chdir);

int unlink(const char *pathname)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	char *p = normalise_path(pathname);
	char *freep = p;

	if (path_check_prereq(pathname, S_IFREG))
		goto out;

	dev = get_fs_device_by_path(&p);
	if (!dev)
		goto out;
	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	if (!fsdrv->unlink) {
		errno = -ENOSYS;
		goto out;
	}

	errno = fsdrv->unlink(dev, p);
out:
	free(freep);
	return errno;
}
EXPORT_SYMBOL(unlink);

int open(const char *pathname, int flags, ...)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f;
	int exist;
	struct stat s;
	char *path = normalise_path(pathname);
	char *freep = path;

	exist = (stat(path, &s) == 0) ? 1 : 0;

	if (exist && S_ISDIR(s.st_mode)) {
		errno = -EISDIR;
		goto out1;
	}

	if (!exist && !(flags & O_CREAT)) {
		errno = -ENOENT;
		goto out1;
	}

	f = get_file();
	if (!f) {
		errno = -EMFILE;
		goto out1;
	}

	dev = get_fs_device_by_path(&path);
	if (!dev)
		goto out;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	f->dev = dev;
	f->flags = flags;

	if ((flags & O_ACCMODE) && !fsdrv->write) {
		errno = -EROFS;
		goto out;
	}

	if (!exist) {
		if (NULL != fsdrv->create)
			errno = fsdrv->create(dev, path,
					S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);
		else
			errno = -EROFS;
		if (errno)
			goto out;
	}
	errno = fsdrv->open(dev, f, path);
	if (errno)
		goto out;


	if (flags & O_TRUNC) {
		errno = fsdrv->truncate(dev, f, 0);
		f->size = 0;
		if (errno)
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
	return errno;
}
EXPORT_SYMBOL(open);

int creat(const char *pathname, mode_t mode)
{
	return open(pathname, O_CREAT | O_WRONLY | O_TRUNC);
}
EXPORT_SYMBOL(creat);

int ioctl(int fd, int request, void *buf)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fd];

	if (check_fd(fd))
		return errno;

	dev = f->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	if (fsdrv->ioctl)
		errno = fsdrv->ioctl(dev, f, request, buf);
	else
		errno = -ENOSYS;
	return errno;
}

int read(int fd, void *buf, size_t count)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fd];

	if (check_fd(fd))
		return errno;

	dev = f->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	if (f->pos + count > f->size)
		count = f->size - f->pos;
	errno = fsdrv->read(dev, f, buf, count);

	if (errno > 0)
		f->pos += errno;
	return errno;
}
EXPORT_SYMBOL(read);

ssize_t write(int fd, const void *buf, size_t count)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fd];

	if (check_fd(fd))
		return errno;

	dev = f->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;
	if (f->pos + count > f->size) {
		errno = fsdrv->truncate(dev, f, f->pos + count);
		if (errno) {
			if (errno != -ENOSPC)
				return errno;
			count = f->size - f->pos;
			if (!count)
				return errno;
		} else {
			f->size = f->pos + count;
		}
	}
	errno = fsdrv->write(dev, f, buf, count);

	if (errno > 0)
		f->pos += errno;
	return errno;
}
EXPORT_SYMBOL(write);

int flush(int fd)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fd];

	if (check_fd(fd))
		return errno;

	dev = f->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;
	if (fsdrv->flush)
		errno = fsdrv->flush(dev, f);
	else
		errno = 0;

	return errno;
}

off_t lseek(int fildes, off_t offset, int whence)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fildes];
	off_t pos;

	if (check_fd(fildes))
		return -1;

	errno = 0;

	dev = f->dev;
	fsdrv = (struct fs_driver_d *)dev->driver->type_data;
	if (!fsdrv->lseek) {
		errno = -ENOSYS;
		return -1;
	}

	switch(whence) {
	case SEEK_SET:
		if (offset > f->size)
			goto out;
		pos = offset;
		break;
	case SEEK_CUR:
		if (offset + f->pos > f->size)
			goto out;
		pos = f->pos + offset;
		break;
	case SEEK_END:
		if (offset)
			goto out;
		pos = f->size;
		break;
	default:
		goto out;
	}

	return fsdrv->lseek(dev, f, pos);

out:
	errno = -EINVAL;
	return -1;
}
EXPORT_SYMBOL(lseek);

int erase(int fd, size_t count, unsigned long offset)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fd];

	if (check_fd(fd))
		return errno;

	dev = f->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	if (f->pos + count > f->size)
		count = f->size - f->pos;

	if (fsdrv->erase)
		errno = fsdrv->erase(dev, f, count, offset);
	else
		errno = -ENOSYS;

	return errno;
}
EXPORT_SYMBOL(erase);

int protect(int fd, size_t count, unsigned long offset, int prot)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fd];

	if (check_fd(fd))
		return errno;

	dev = f->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	if (f->pos + count > f->size)
		count = f->size - f->pos;

	if (fsdrv->protect)
		errno = fsdrv->protect(dev, f, count, offset, prot);
	else
		errno = -ENOSYS;

	return errno;
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
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fd];
	void *ret = (void *)-1;

	if (check_fd(fd))
		return ret;

	dev = f->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	if (fsdrv->memmap)
		errno = fsdrv->memmap(dev, f, &ret, flags);
	else
		errno = -EINVAL;

	return ret;
}
EXPORT_SYMBOL(memmap);

int close(int fd)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fd];

	if (check_fd(fd))
		return errno;

	dev = f->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;
	errno = fsdrv->close(dev, f);

	put_file(f);
	return errno;
}
EXPORT_SYMBOL(close);

static LIST_HEAD(fs_driver_list);

int register_fs_driver(struct fs_driver_d *fsdrv)
{
	list_add_tail(&fsdrv->list, &fs_driver_list);
	register_driver(&fsdrv->drv);
	return 0;
}
EXPORT_SYMBOL(register_fs_driver);

/*
 * Mount a device to a directory.
 * We do this by registering a new device on which the filesystem
 * driver will match. The filesystem driver then grabs the infomation
 * it needs from the new devices type_data.
 */
int mount(const char *device, const char *fsname, const char *_path)
{
	struct fs_driver_d *fs_drv = NULL, *f;
	struct mtab_entry *entry;
	struct fs_device_d *fsdev;
	struct device_d *dev, *parent_device = NULL;
	int ret;
	char *path = normalise_path(_path);

	errno = 0;

	debug("mount: %s on %s type %s\n", device, path, fsname);

	if (get_mtab_entry_by_path(path) != mtab) {
		errno = -EBUSY;
		goto out;
	}

	if (strchr(path + 1, '/')) {
		printf("mounting allowed on first directory level only\n");
		errno = -EBUSY;
		goto out;
	}

	list_for_each_entry(f, &fs_driver_list, list) {
		if (!strcmp(f->drv.name, fsname)) {
			fs_drv = f;
			break;
		}
	}

	if (!fs_drv) {
		errno = -EINVAL;
		goto out;
	}

	if (mtab) {
		if (path_check_prereq(path, S_IFDIR))
			goto out;
	} else {
		/* no mtab, so we only allow to mount on '/' */
		if (*path != '/' || *(path + 1)) {
			errno = -ENOTDIR;
			goto out;
		}
	}

	fsdev = xzalloc(sizeof(struct fs_device_d));
	if (!(fs_drv->flags & FS_DRIVER_NO_DEV)) {
		fsdev->backingstore = strdup(device);
		if (!device) {
			printf("need a device for driver %s\n", fsname);
			errno = -ENODEV;
			goto out1;
		}
	}
	safe_strncpy(fsdev->dev.name, fsname, MAX_DRIVER_NAME);
	fsdev->dev.type_data = fsdev;
	fsdev->dev.id = get_free_deviceid(fsdev->dev.name);

	if ((ret = register_device(&fsdev->dev))) {
		errno = ret;
		goto out1;
	}

	if (!fsdev->dev.driver) {
		/* driver didn't accept the device. Bail out */
		errno = -EINVAL;
		goto out2;
	}

	if (parent_device)
		dev_add_child(parent_device, &fsdev->dev);

	dev = &fsdev->dev;

	/* add mtab entry */
	entry = &fsdev->mtab; 
	safe_strncpy(entry->path, path, PATH_MAX);
	entry->dev = dev;
	entry->parent_device = parent_device;
	entry->next = NULL;

	if (!mtab)
		mtab = entry;
	else {
		struct mtab_entry *e = mtab;
		while (e->next)
			e = e->next;
		e->next = entry;
	}
	errno = 0;

	free(path);
	return 0;

out2:
	unregister_device(&fsdev->dev);
out1:
	if (fsdev->backingstore)
		free(fsdev->backingstore);
	free(fsdev);
out:
	free(path);
	return errno;
}
EXPORT_SYMBOL(mount);

int umount(const char *pathname)
{
	struct mtab_entry *entry = mtab;
	struct mtab_entry *last = mtab;
	char *p = normalise_path(pathname);
	struct fs_device_d *fsdev;

	while(entry && strcmp(p, entry->path)) {
		last = entry;
		entry = entry->next;
	}

	free(p);

	if (!entry) {
		errno = -EFAULT;
		return errno;
	}

	if (entry == mtab)
		mtab = mtab->next;
	else
		last->next = entry->next;

	unregister_device(entry->dev);
	fsdev = entry->dev->type_data;
	free(fsdev->backingstore);
	free(fsdev);

	return 0;
}
EXPORT_SYMBOL(umount);

DIR *opendir(const char *pathname)
{
	DIR *dir = NULL;
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	char *p = normalise_path(pathname);
	char *freep = p;

	if (path_check_prereq(pathname, S_IFDIR))
		goto out;

	dev = get_fs_device_by_path(&p);
	if (!dev)
		goto out;
	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	debug("opendir: fsdrv: %p\n",fsdrv);

	dir = fsdrv->opendir(dev, p);
	if (dir) {
		dir->dev = dev;
		dir->fsdrv = fsdrv;
	}

out:
	free(freep);
	return dir;
}
EXPORT_SYMBOL(opendir);

struct dirent *readdir(DIR *dir)
{
	if (!dir)
		return NULL;

	return dir->fsdrv->readdir(dir->dev, dir);
}
EXPORT_SYMBOL(readdir);

int closedir(DIR *dir)
{
	if (!dir) {
		errno = -EBADF;
		return -1;
	}

	return dir->fsdrv->closedir(dir->dev, dir);
}
EXPORT_SYMBOL(closedir);

int stat(const char *filename, struct stat *s)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	struct mtab_entry *e;
	char *f = normalise_path(filename);
	char *freep = f;

	memset(s, 0, sizeof(struct stat));

	e = get_mtab_entry_by_path(f);
	if (!e) {
		errno = -ENOENT;
		goto out;
	}

	if (e != mtab && strcmp(f, e->path)) {
		f += strlen(e->path);
		dev = e->dev;
	} else
		dev = mtab->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	if (*f == 0)
		f = "/";

	errno = fsdrv->stat(dev, f, s);
out:
	free(freep);
	return errno;
}
EXPORT_SYMBOL(stat);

int mkdir (const char *pathname, mode_t mode)
{
	struct fs_driver_d *fsdrv;
	struct device_d *dev;
	char *p = normalise_path(pathname);
	char *freep = p;

	if (path_check_prereq(pathname, S_UB_DOES_NOT_EXIST))
		goto out;

	dev = get_fs_device_by_path(&p);
	if (!dev)
		goto out;
	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	if (fsdrv->mkdir) {
		errno = fsdrv->mkdir(dev, p);
		goto out;
	}

	errno = -EROFS;
out:
	free(freep);
	return errno;
}
EXPORT_SYMBOL(mkdir);

int rmdir (const char *pathname)
{
	struct fs_driver_d *fsdrv;
	struct device_d *dev;
	char *p = normalise_path(pathname);
	char *freep = p;

	if (path_check_prereq(pathname, S_IFDIR | S_UB_IS_EMPTY))
		goto out;

	dev = get_fs_device_by_path(&p);
	if (!dev)
		goto out;
	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	if (fsdrv->rmdir) {
		errno = fsdrv->rmdir(dev, p);
		goto out;
	}

	errno = -EROFS;
out:
	free(freep);
	return errno;
}
EXPORT_SYMBOL(rmdir);

static void memcpy_sz(void *_dst, const void *_src, ulong count, ulong rwsize)
{
	ulong dst = (ulong)_dst;
	ulong src = (ulong)_src;

	/* no rwsize specification given. Do whatever memcpy likes best */
	if (!rwsize) {
		memcpy(_dst, _src, count);
		return;
	}

	rwsize = rwsize >> O_RWSIZE_SHIFT;

	count /= rwsize;

	while (count-- > 0) {
		switch (rwsize) {
		case 1:
			*((u_char *)dst) = *((u_char *)src);
			break;
		case 2:
			*((ushort *)dst) = *((ushort *)src);
			break;
		case 4:
			*((ulong  *)dst) = *((ulong  *)src);
			break;
		}
		dst += rwsize;
		src += rwsize;
	}
}

ssize_t mem_read(struct cdev *cdev, void *buf, size_t count, ulong offset, ulong flags)
{
	ulong size;
	struct device_d *dev;

	if (!cdev->dev)
		return -1;
	dev = cdev->dev;

	size = min((ulong)count, dev->size - offset);
	debug("mem_read: dev->map_base: %p size: %d offset: %d\n",dev->map_base, size, offset);
	memcpy_sz(buf, (void *)(dev->map_base + offset), size, flags & O_RWSIZE_MASK);
	return size;
}
EXPORT_SYMBOL(mem_read);

ssize_t mem_write(struct cdev *cdev, const void *buf, size_t count, ulong offset, ulong flags)
{
	ulong size;
	struct device_d *dev;

	if (!cdev->dev)
		return -1;
	dev = cdev->dev;

	size = min((ulong)count, dev->size - offset);
	memcpy_sz((void *)(dev->map_base + offset), buf, size, flags & O_RWSIZE_MASK);
	return size;
}
EXPORT_SYMBOL(mem_write);

