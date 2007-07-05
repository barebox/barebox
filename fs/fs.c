#include <common.h>
#include <command.h>
#include <fs.h>
#include <driver.h>
#include <errno.h>
#include <asm-generic/errno.h>
#include <malloc.h>
#include <linux/stat.h>

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

struct mtab_entry {
	char path[PATH_MAX];
	struct mtab_entry *next;
	struct device_d *dev;
};

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

int mount (struct device_d *dev, char *fsname, char *path)
{
	struct driver_d *drv;
	struct fs_driver_d *fs_drv;
	struct mtab_entry *entry;
	struct fs_device_d *fsdev;
	int ret;

	drv = get_driver_by_name(fsname);
	if (!drv) {
		printf("no driver for fstype %s\n", fsname);
		return -EINVAL;
	}

	if (drv->type != DEVICE_TYPE_FS) {
		printf("driver %s is no filesystem driver\n");
		return -EINVAL;
	}

	/* check if path exists */
	/* TODO */

	fs_drv = drv->type_data;
	printf("mount: fs_drv: %p\n", fs_drv);

	if (fs_drv->flags & FS_DRIVER_NO_DEV) {
		dev = malloc(sizeof(struct device_d));
		memset(dev, 0, sizeof(struct device_d));
		sprintf(dev->name, "%s", fsname);
		dev->type = DEVICE_TYPE_FS;
		if ((ret = register_device(dev))) {
			free(dev);
			return ret;
		}
		if (!dev->driver) {
			/* driver didn't accept the device. Bail out */
			free(dev);
			return -EINVAL;
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
			return ret;
		}
		if (!fsdev->dev.driver) {
			/* driver didn't accept the device. Bail out */
			free(fsdev);
			return -EINVAL;
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

	printf("mount: mtab->dev: %p\n", mtab->dev);

	return 0;
}

int do_mount (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	struct device_d *dev;
	int ret = 0;

	if (argc != 4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	dev = get_device_by_id(argv[1]);

	if ((ret = mount(dev, argv[2], argv[3]))) {
		perror("mount", ret);
		return 1;
	}
	return 0;
}

struct dir *opendir(const char *pathname)
{
	struct device_d *dev;
	struct fs_driver_d *fsdrv;
	struct dir *dir;
	struct mtab_entry *e;

	e = get_mtab_entry_by_path(pathname);
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

static int ls(const char *path)
{
	struct dir *dir;
	struct dirent *d;
	char modestr[11];

	dir = opendir(path);
	if (!dir)
		return -ENOENT;

	while ((d = readdir(dir))) {
		unsigned long namelen = strlen(d->name);
		mkmodestr(d->mode, modestr);
		printf("%s %8d %*.*s\n",modestr, d->size, namelen, namelen, d->name);
	}

	closedir(dir);

	return 0;
}

int do_ls (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret;

	ret = ls(argv[1]);
	if (ret) {
		perror("ls", ret);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	ls,     2,     0,      do_ls,
	"ls      - list a file or directory\n",
	"<path> list files on path"
);

int mkdir (const char *pathname)
{
	struct fs_driver_d *fsdrv;
	struct device_d *dev;
	struct mtab_entry *e;

	e = get_mtab_entry_by_path(pathname);
	if (e != mtab)
		pathname += strlen(e->path);

	dev = e->dev;

	fsdrv = (struct fs_driver_d *)dev->driver->type_data;

	if (fsdrv->mkdir)
		return fsdrv->mkdir(dev, pathname);
	return -EROFS;
}

int do_mkdir (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret;

	ret = mkdir(argv[1]);
	if (ret) {
		perror("ls", ret);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	mkdir,     2,     0,      do_mkdir,
	"mkdir    - create a new directory\n",
	""
);

U_BOOT_CMD(
	mount,     4,     0,      do_mount,
	"mount   - mount a filesystem to a device\n",
	" <device> <type> <path> add a filesystem of type 'type' on the given device"
);
#if 0
U_BOOT_CMD(
	delfs,     2,     0,      do_delfs,
	"delfs   - delete a filesystem from a device\n",
	""
);
#endif
