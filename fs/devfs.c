#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <fs.h>
#include <command.h>
#include <errno.h>
#include <linux/stat.h>

static int devfs_read(struct device_d *_dev, FILE *f, void *buf, size_t size)
{
	struct device_d *dev = f->inode;

	return dev_read(dev, buf, size, f->pos, f->flags);
}

static int devfs_write(struct device_d *_dev, FILE *f, const void *buf, size_t size)
{
	struct device_d *dev = f->inode;

	return dev_write(dev, buf, size, f->pos, f->flags);
}

static int devfs_open(struct device_d *_dev, FILE *file, const char *filename)
{
	struct device_d *dev = get_device_by_id(filename + 1);

	if (!dev)
		return -ENOENT;

	file->size = dev->size;
	file->inode = dev;
	return 0;
}

static int devfs_close(struct device_d *dev, FILE *f)
{
	return 0;
}

DIR* devfs_opendir(struct device_d *dev, const char *pathname)
{
	DIR *dir;

	dir = malloc(sizeof(DIR));
	if (!dir)
		return NULL;

	dir->priv = get_first_device();

	return dir;
}

struct dirent* devfs_readdir(struct device_d *_dev, DIR *dir)
{
	struct device_d *dev = dir->priv;

	while (dev && (!strlen(dev->id) || !dev->driver))
		dev = dev->next;

	if (dev) {
		strcpy(dir->d.d_name, dev->id);
		dir->priv = dev->next;
		return &dir->d;
	}
	return NULL;
}

int devfs_closedir(struct device_d *dev, DIR *dir)
{
	free(dir);
	return 0;
}

int devfs_stat(struct device_d *_dev, const char *filename, struct stat *s)
{
	struct device_d *dev;

	dev = get_device_by_id(filename + 1);
	if (!dev)
		return -ENOENT;

	if (!dev->driver)
		return -ENXIO;

	s->st_mode = S_IFCHR;
	s->st_size = dev->size;
	if (dev->driver->write)
		s->st_mode |= S_IWUSR;
	if (dev->driver->read)
		s->st_mode |= S_IRUSR;

	return 0;
}

int devfs_probe(struct device_d *dev)
{
	return 0;
}

static struct fs_driver_d devfs_driver = {
	.type      = FS_TYPE_DEVFS,
	.read      = devfs_read,
	.write     = devfs_write,
	.open      = devfs_open,
	.close     = devfs_close,
	.opendir   = devfs_opendir,
	.readdir   = devfs_readdir,
	.closedir  = devfs_closedir,
	.stat      = devfs_stat,
	.flags     = FS_DRIVER_NO_DEV,
	.drv = {
		.type   = DEVICE_TYPE_FS,
		.probe  = devfs_probe,
		.name = "devfs",
		.type_data = &devfs_driver,
	}
};

int devfs_init(void)
{
	return register_driver(&devfs_driver.drv);
}

device_initcall(devfs_init);

