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

static int fs_probe(struct device_d *dev)
{
	struct fs_device_d *fs_dev = dev->platform_data;

	return fs_dev->driver->probe(fs_dev->parent);
}

int register_fs_driver(struct fs_driver_d *new_fs_drv)
{
	new_fs_drv->drv.probe = fs_probe;
	new_fs_drv->drv.driver_data = new_fs_drv;

	return register_driver(&new_fs_drv->drv);
}

int register_filesystem(struct device_d *dev, char *fsname)
{
	struct fs_device_d *new_fs_dev;
	struct driver_d *drv;
	struct fs_driver_d *fs_drv;

	drv = get_driver_by_name(fsname);
	if (!drv) {
		printf("no driver for fstype %s\n", fsname);
		return -EINVAL;
	}

	if (drv->type != DEVICE_TYPE_FS) {
		printf("driver %s is no filesystem driver\n");
		return -EINVAL;
	}

	new_fs_dev = malloc(sizeof(struct fs_device_d));

	fs_drv = drv->driver_data;

	new_fs_dev->driver = fs_drv;
	new_fs_dev->parent = dev;
	new_fs_dev->dev.platform_data = new_fs_dev;
	new_fs_dev->dev.type = DEVICE_TYPE_FS;
	sprintf(new_fs_dev->dev.name, "%s", fsname);
	sprintf(new_fs_dev->dev.id, "%s", "fs0");

	register_device(&new_fs_dev->dev);

	if (!new_fs_dev->dev.driver) {
		unregister_device(&new_fs_dev->dev);
		return -ENODEV;
	}

	return 0;
}

int ls(struct device_d *dev, const char *filename)
{
	struct fs_device_d *fs_dev;

	if (!dev || dev->type != DEVICE_TYPE_FS || !dev->driver)
		return -ENODEV;

	fs_dev = dev->platform_data;

	return fs_dev->driver->ls(fs_dev->parent, filename);
}

/* addfs <device> <fstype> */
int do_addfs ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	struct device_d *dev;
	int ret = 0;

	if (argc != 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	dev = get_device_by_id(argv[1]);
	if (!dev) {
		printf("no such device: %s\n", argv[1]);
		return -ENODEV;
	}

	if ((ret = register_filesystem(dev, argv[2]))) {
		perror("register_device", ret);
		return 1;
	}
	return 0;
}

int do_ls ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	struct device_d *dev;
	char *endp;
	int ret;

	dev = device_from_spec_str(argv[1], &endp);

	ret = ls(dev, endp);
	if (ret) {
		perror("ls", ret);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	ls,     2,     0,      do_ls,
	"ls      - list a file or directory\n",
	"<dev:path> list files on device"
);

U_BOOT_CMD(
	addfs,     3,     0,      do_addfs,
	"addfs   - add a filesystem to a device\n",
	" <device> <type> add a filesystem of type 'type' on the given device"
);
#if 0
U_BOOT_CMD(
	delfs,     2,     0,      do_delfs,
	"delfs   - delete a filesystem from a device\n",
	""
);
#endif
