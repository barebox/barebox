#include <common.h>
#include <fs.h>
#include <driver.h>
#include <errno.h>
#include <malloc.h>
#include <linux/stat.h>
#include <fcntl.h>
#include <xfuncs.h>
#include <init.h>

void *read_file(const char *filename)
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
	return buf;

err_out1:
	close(fd);
err_out:
	free(buf);
	return NULL;
}

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

static struct device_d *get_device_by_path(char **path)
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

int dir_is_empty(const char *pathname)
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

	errno = 0;

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
out:
	return errno;
}

const char *getcwd(void)
{
	return cwd;
}

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

int unlink(const char *pathname)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	char *p = normalise_path(pathname);
	char *freep = p;

	if (path_check_prereq(pathname, S_IFREG))
		goto out;

	dev = get_device_by_path(&p);
	if (!dev)
		goto out;
	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	errno = fsdrv->unlink(dev, p);
out:
	free(freep);
	return errno;
}

int open(const char *pathname, int flags)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f;
	int exist;
	struct stat s;
	char *path = normalise_path(pathname);
	char *freep = path;

	exist = (stat(path, &s) == 0) ? 1 : 0;

	if (exist && (s.st_mode & S_IFDIR)) {
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

	dev = get_device_by_path(&path);
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
		errno = fsdrv->create(dev, path, S_IFREG);
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

int creat(const char *pathname, mode_t mode)
{
	return open(pathname, O_CREAT | O_WRONLY | O_TRUNC);
}

int read(int fd, void *buf, size_t count)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fd];

	dev = f->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	if (f->pos + count > f->size)
		count = f->size - f->pos;
	errno = fsdrv->read(dev, f, buf, count);

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

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;
	if (f->pos + count > f->size) {
		errno = fsdrv->truncate(dev, f, f->pos + count);
		if (errno)
			return errno;
		f->size = f->pos + count;
	}
	errno = fsdrv->write(dev, f, buf, count);

	if (errno > 0)
		f->pos += errno;
	return errno;
}

off_t lseek(int fildes, off_t offset, int whence)
{
	FILE *f = &files[fildes];
	errno = 0;

	switch(whence) {
	case SEEK_SET:
		if (offset > f->size)
			goto out;
		f->pos = offset;
		break;
	case SEEK_CUR:
		if (offset + f->pos > f->size)
			goto out;
		f->pos += offset;
		break;
	case SEEK_END:
		if (offset)
			goto out;
		f->pos = f->size;
		break;
	default:
		goto out;
	}

	return 0;
out:
	errno = -EINVAL;
	return errno;
}

int erase(int fd, size_t count, unsigned long offset)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fd];

	dev = f->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	if (f->pos + count > f->size)
		count = f->size - f->pos;
	errno = fsdrv->erase(dev, f, count, offset);

	return errno;
}

int close(int fd)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	FILE *f = &files[fd];

	dev = f->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;
	errno = fsdrv->close(dev, f);

	put_file(f);
	return errno;
}

/*
 * Mount a device to a directory.
 * We do this by registering a new device on which the filesystem
 * driver will match. The filesystem driver then grabs the infomation
 * it needs from the new devices type_data.
 */
int mount(const char *device, const char *fsname, const char *path)
{
	struct driver_d *drv;
	struct fs_driver_d *fs_drv;
	struct mtab_entry *entry;
	struct fs_device_d *fsdev;
	struct device_d *dev, *parent_device = 0;
	int ret;

	errno = 0;

	debug("mount: %s on %s type %s\n", device, path, fsname);

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
		if (path_check_prereq(path, S_IFDIR))
			goto out;
	} else {
		/* no mtab, so we only allow to mount on '/' */
		if (*path != '/' || *(path + 1)) {
			errno = -ENOTDIR;
			goto out;
		}
	}

	fs_drv = drv->type_data;

	if (!fs_drv->flags & FS_DRIVER_NO_DEV) {
		parent_device = get_device_by_id(device + 5);
		if (!parent_device) {
			printf("need a device for driver %s\n", fsname);
			errno = -ENODEV;
			goto out;
		}
	}
	fsdev = xzalloc(sizeof(struct fs_device_d));
	fsdev->parent = parent_device;
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

	/* add mtab entry */
	entry = malloc(sizeof(struct mtab_entry));
	sprintf(entry->path, "%s", path);
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
out:
	return errno;
}

int umount(const char *pathname)
{
	struct mtab_entry *entry = mtab;
	struct mtab_entry *last = mtab;
	char *p = normalise_path(pathname);

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
	free(entry->dev->type_data);
	free(entry);
	return 0;
}

DIR *opendir(const char *pathname)
{
	DIR *dir = NULL;
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	char *p = normalise_path(pathname);
	char *freep = p;

	if (path_check_prereq(pathname, S_IFDIR))
		goto out;

	dev = get_device_by_path(&p);
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

struct dirent *readdir(DIR *dir)
{
	return dir->fsdrv->readdir(dir->dev, dir);
}

int closedir(DIR *dir)
{
	return dir->fsdrv->closedir(dir->dev, dir);
}

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

int mkdir (const char *pathname)
{
	struct fs_driver_d *fsdrv;
	struct device_d *dev;
	char *p = normalise_path(pathname);
	char *freep = p;

	if (path_check_prereq(pathname, S_UB_DOES_NOT_EXIST))
		goto out;

	dev = get_device_by_path(&p);
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

int rmdir (const char *pathname)
{
	struct fs_driver_d *fsdrv;
	struct device_d *dev;
	char *p = normalise_path(pathname);
	char *freep = p;

	if (path_check_prereq(pathname, S_IFDIR | S_UB_IS_EMPTY))
		goto out;

	dev = get_device_by_path(&p);
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

