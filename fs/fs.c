#include <common.h>
#include <fs.h>
#include <driver.h>
#include <errno.h>
#include <asm-generic/errno.h>
#include <malloc.h>
#include <linux/stat.h>
#include <fcntl.h>

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

/*
 * - Remove all multiple slashes
 * - Remove trailing slashes (except path consists of only
 *   a single slash)
 * - TODO: illegal characters?
 */
void normalise_path(char *path)
{
        char *out = path, *in = path;

        while(*in) {
                if(*in == '/') {
                        *out++ = *in++;
                        while(*in == '/')
                                in++;
                } else {
                        *out++ = *in++;
                }
        }

        /*
         * Remove trailing slash, but only if
         * we were given more than a single slash
         */
        if (out > path + 1 && *(out - 1) == '/')
                *(out - 1) = 0;

        *out = 0;
}

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

FILE files[MAX_FILES];

FILE *get_file(void)
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

void put_file(FILE *f)
{
	files[f->no].in_use = 0;
}

static FILE* get_file_by_pathname(const char *pathname)
{
	FILE *f;

	f = get_file();
	if (!f) {
		errno = -EMFILE;
		return NULL;
	}

	if (!strncmp(pathname, "/dev/", 5)) {
		f->dev = get_device_by_id(pathname + 5);
	}

	return f;
}

int creat(const char *pathname, mode_t mode)
{
	struct mtab_entry *e;
	struct device_d *dev;
	struct fs_driver_d *fsdrv;

	e = get_mtab_entry_by_path(pathname);
	if (!e) {
		errno = -ENOENT;
		return errno;
	}

	if (e != mtab)
		pathname += strlen(e->path);

	dev = e->dev;
	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	errno = fsdrv->create(dev, pathname, S_IFREG);

	return errno;
}

int open(const char *pathname, int flags)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	struct mtab_entry *e;
	FILE *f;
	int exist;
	struct stat s;

	exist = stat(pathname, &s) == 0 ? 1 : 0;

	if (!exist && !(flags & O_CREAT))
		return -EEXIST;

	f = get_file_by_pathname(pathname);

	if (f->dev)
		return f->no;

	e = get_mtab_entry_by_path(pathname);
	if (!e) {
		/* This can only happen when nothing is mounted */
		errno = -ENOENT;
		goto out;
	}

	/* Adjust the pathname to the root of the device */
	if (e != mtab)
		pathname += strlen(e->path);

	dev = e->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;
	f->dev = dev;

	if ((flags & O_ACCMODE) && !fsdrv->write) {
		errno = -EROFS;
		goto out;
	}

	if (!exist) {
		errno = fsdrv->create(dev, pathname, S_IFREG);
		if (errno)
			goto out;
	}
	errno = fsdrv->open(dev, f, pathname);
	if (errno)
		goto out;

	if (flags & O_APPEND)
		f->pos = f->size;

	if (flags & O_TRUNC) {
		errno = fsdrv->truncate(dev, f, 0);
		if (errno)
			goto out;
	}

	return f->no;

out:
	put_file(f);
	return errno;
}

int read(int fd, void *buf, size_t count)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fd];

	dev = f->dev;
	printf("READ: dev: %p\n",dev);
	if (dev->type == DEVICE_TYPE_FS) {
		fsdrv = (struct fs_driver_d *)dev->driver->type_data;
		printf("\nreading %d bytes at %d\n",count, f->pos);
		errno = fsdrv->read(dev, f, buf, count);
	} else {
		errno = dev->driver->read(dev, buf, count, f->pos, 0); /* FIXME: flags */
	}
	if (errno > 0)
		f->pos += errno;
	return errno;
}

ssize_t write(int fd, const void *buf, size_t count)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fd];

	dev = f->dev;
	printf("WRITE: dev: %p\n",dev);
	if (dev->type == DEVICE_TYPE_FS) {
		fsdrv = (struct fs_driver_d *)dev->driver->type_data;
		if (f->pos + count > f->size) {
			errno = fsdrv->truncate(dev, f, f->pos + count);
			if (errno)
				return errno;
			f->size = f->pos + count;
		}
		errno = fsdrv->write(dev, f, buf, count);
	} else {
		errno = dev->driver->write(dev, buf, count, f->pos, 0); /* FIXME: flags */
	}
	if (errno > 0)
		f->pos += errno;
	return errno;
}

int close(int fd)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fd];

	dev = f->dev;

	if (dev->type == DEVICE_TYPE_FS) {
		fsdrv = (struct fs_driver_d *)dev->driver->type_data;
		errno = fsdrv->close(dev, f);
	}

	put_file(f);
	return errno;
}

int mount (struct device_d *dev, char *fsname, char *path)
{
	struct driver_d *drv;
	struct fs_driver_d *fs_drv;
	struct mtab_entry *entry;
	struct fs_device_d *fsdev;
	struct dir *dir;
	int ret;

	errno = 0;

	drv = get_driver_by_name(fsname);
	if (!drv) {
		errno = -ENODEV;
		goto out;
	}

	if (drv->type != DEVICE_TYPE_FS) {
		errno = -EINVAL;
		goto out;
	}

	if (mtab) {
		/* check if path exists and is a directory */
		if (!(dir = opendir(path))) {
			errno = -ENOTDIR;
			goto out;
		}
		closedir(dir);
	} else {
		/* no mtab, so we only allow to mount on '/' */
		if (*path != '/' || *(path + 1)) {
			errno = -ENOTDIR;
			goto out;
		}
	}

	fs_drv = drv->type_data;

	if (fs_drv->flags & FS_DRIVER_NO_DEV) {
		dev = malloc(sizeof(struct device_d));
		memset(dev, 0, sizeof(struct device_d));
		sprintf(dev->name, "%s", fsname);
		dev->type = DEVICE_TYPE_FS;
		if ((ret = register_device(dev))) {
			free(dev);
			errno = ret;
			goto out;
		}
		if (!dev->driver) {
			/* driver didn't accept the device. Bail out */
			free(dev);
			errno = -EINVAL;
			goto out;
		}
	} else {
		fsdev = malloc(sizeof(struct fs_device_d));
		memset(fsdev, 0, sizeof(struct fs_device_d));
		fsdev->parent = dev;
		sprintf(fsdev->dev.name, "%s", fsname);
		fsdev->dev.type = DEVICE_TYPE_FS;
		fsdev->dev.type_data = fsdev;
		if ((ret = register_device(&fsdev->dev))) {
			free(fsdev);
			errno = ret;
			goto out;
		}
		if (!fsdev->dev.driver) {
			/* driver didn't accept the device. Bail out */
			free(fsdev);
			errno = -EINVAL;
			goto out;
		}
		dev = &fsdev->dev;
	}

	/* add mtab entry */
	entry = malloc(sizeof(struct mtab_entry));
	sprintf(entry->path, "%s", path);
	entry->dev = dev;
	entry->next = NULL;

	if (!mtab)
		mtab = entry;
	else {
		struct mtab_entry *e = mtab;
		while (e->next)
			e = e->next;
		e->next = entry;
	}
out:
	return errno;
}

int umount(const char *pathname)
{
	struct mtab_entry *entry = mtab;
	struct mtab_entry *last = mtab;
	char *p = strdup(pathname);

	normalise_path(p);

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
	free(entry);

	return 0;
}

struct dir *opendir(const char *pathname)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	struct dir *dir;
	struct mtab_entry *e;

	e = get_mtab_entry_by_path(pathname);
	if (!e)
		return NULL;
	if (e != mtab)
		pathname += strlen(e->path);

	dev = e->dev;
//	printf("opendir: dev: %p\n",dev);

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;
//	printf("opendir: fsdrv: %p\n",fsdrv);

	dir = fsdrv->opendir(dev, pathname);
	if (dir) {
		dir->dev = dev;
		dir->fsdrv = fsdrv;
	}

	return dir;
}

struct dirent *readdir(struct dir *dir)
{
	return dir->fsdrv->readdir(dir->dev, dir);
}

int closedir(struct dir *dir)
{
	return dir->fsdrv->closedir(dir->dev, dir);
}

int stat(const char *filename, struct stat *s)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	struct mtab_entry *e;
	char *buf = strdup(filename);
	char *f = buf;

	memset(s, 0, sizeof(struct stat));

	normalise_path(f);

	e = get_mtab_entry_by_path(f);
	if (!e) {
		errno = -ENOENT;
		goto out;
	}
	if (e != mtab)
		f += strlen(e->path);

	dev = e->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	if (*f == 0)
		f = "/";

	errno = fsdrv->stat(dev, f, s);
out:
	free(buf);
	return errno;
}

int ls(const char *path)
{
	struct dir *dir;
	struct dirent *d;
	char modestr[11];
	char tmp[PATH_MAX];
	struct stat s;

	dir = opendir(path);
	if (!dir) {
		errno = -ENOENT;
		return -ENOENT;
	}

	while ((d = readdir(dir))) {
		unsigned long namelen = strlen(d->name);
		sprintf(tmp, "%s/%s", path, d->name);
		if (stat(tmp, &s)) {
			perror("stat");
			return errno;
		}

		mkmodestr(s.st_mode, modestr);
		printf("%s %8d %*.*s\n",modestr, s.st_size, namelen, namelen, d->name);
	}

	closedir(dir);

	return 0;
}

int mkdir (const char *pathname)
{
	struct fs_driver_d *fsdrv;
	struct device_d *dev;
	struct mtab_entry *e;

	e = get_mtab_entry_by_path(pathname);
	if (!e) {
		errno = -ENOENT;
		return -ENOENT;
	}

	if (e != mtab)
		pathname += strlen(e->path);

	dev = e->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	if (fsdrv->mkdir)
		return fsdrv->mkdir(dev, pathname);

	errno = -EROFS;
	return -EROFS;
}

