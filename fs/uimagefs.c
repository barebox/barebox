/*
 * Copyright (c) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * under GPLv2 ONLY
 */

#include <common.h>
#include <driver.h>
#include <fs.h>
#include <errno.h>
#include <fcntl.h>
#include <fs.h>
#include <malloc.h>
#include <init.h>
#include <linux/stat.h>
#include <linux/err.h>
#include <uimagefs.h>
#include <libbb.h>
#include <rtc.h>
#include <crc.h>
#include <libfile.h>

static bool uimagefs_is_data_file(struct uimagefs_handle_data *d)
{
	return d->type == UIMAGEFS_DATA;
}

const char* uimagefs_type_to_str(enum uimagefs_type type)
{
	switch (type) {
	case UIMAGEFS_DATA:
		return "data";
	case UIMAGEFS_DATA_CRC:
		return "data.crc";
	case UIMAGEFS_NAME:
		return "name";
	case UIMAGEFS_TIME:
		return "time";
	case UIMAGEFS_LOAD:
		return "load_addr";
	case UIMAGEFS_EP:
		return "entry_point";
	case UIMAGEFS_OS:
		return "os";
	case UIMAGEFS_ARCH:
		return "arch";
	case UIMAGEFS_TYPE:
		return "type";
	case UIMAGEFS_COMP:
		return "compression";
	}

	return "unknown";
}

static struct uimagefs_handle_data *uimagefs_get_by_name(
	struct uimagefs_handle *handle, const char *name)
{
	struct uimagefs_handle_data *d;

	if (!name)
		return NULL;

	list_for_each_entry(d, &handle->list, list) {
		if (strcmp(d->name, name) == 0)
			return d;
	}

	return NULL;
}

static int uimagefs_open(struct device_d *dev, FILE *file, const char *filename)
{
	struct uimagefs_handle *priv = dev->priv;
	struct uimagefs_handle_data *d;

	if (filename[0] == '/')
		filename++;

	d = uimagefs_get_by_name(priv, filename);
	if (!d)
		return -EINVAL;

	if (uimagefs_is_data_file(d)) {
		d->fd = open(priv->filename, O_RDONLY);
		if (d->fd < 0)
			return d->fd;

		lseek(d->fd, d->offset, SEEK_SET);
	}

	file->size = d->size;
	file->priv = d;

	return 0;
}

static int uimagefs_close(struct device_d *dev, FILE *file)
{
	struct uimagefs_handle_data *d = file->priv;

	close(d->fd);

	return 0;
}

static int uimagefs_read(struct device_d *dev, FILE *file, void *buf, size_t insize)
{
	struct uimagefs_handle_data *d = file->priv;

	if (!uimagefs_is_data_file(d)) {
		memcpy(buf, &d->data[d->pos], insize);
		return insize;
	} else {
		return read(d->fd, buf, insize);
	}
}

static loff_t uimagefs_lseek(struct device_d *dev, FILE *file, loff_t pos)
{
	struct uimagefs_handle_data *d = file->priv;

	if (uimagefs_is_data_file(d))
		lseek(d->fd, d->offset + pos, SEEK_SET);

	d->pos = pos;

	return pos;
}

static DIR *uimagefs_opendir(struct device_d *dev, const char *pathname)
{
	struct uimagefs_handle *priv = dev->priv;
	DIR *dir;

	dir = xzalloc(sizeof(DIR));

	if (list_empty(&priv->list))
		return dir;

	dir->priv = list_first_entry(&priv->list,
					struct uimagefs_handle_data, list);
	return dir;
}

static struct dirent *uimagefs_readdir(struct device_d *dev, DIR *dir)
{
	struct uimagefs_handle *priv = dev->priv;
	struct uimagefs_handle_data *d = dir->priv;

	if (!d || &d->list == &priv->list)
		return NULL;

	strcpy(dir->d.d_name, d->name);
	dir->priv = list_entry(d->list.next, struct uimagefs_handle_data, list);
	return &dir->d;
}

static int uimagefs_closedir(struct device_d *dev, DIR *dir)
{
	free(dir);
	return 0;
}

static int uimagefs_stat(struct device_d *dev, const char *filename, struct stat *s)
{
	struct uimagefs_handle *priv = dev->priv;
	struct uimagefs_handle_data *d;

	if (filename[0] == '/')
		filename++;

	d = uimagefs_get_by_name(priv, filename);
	if (!d)
		return -EINVAL;

	s->st_size = d->size;
	s->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

	return 0;
}

static int uimagefs_ioctl(struct device_d *dev, FILE *f, int request, void *buf)
{
	struct uimagefs_handle *priv = dev->priv;

	if (request != UIMAGEFS_METADATA)
		return  -EINVAL;

	memcpy(buf, &priv->header, sizeof(struct image_header));

	return 0;
}

static void uimagefs_remove(struct device_d *dev)
{
	struct uimagefs_handle *priv = dev->priv;
	struct uimagefs_handle_data *d, *tmp;
	struct stat s;

	list_for_each_entry_safe(d, tmp, &priv->list, list) {
		free(d->name);
		free(d->data);
		free(d);
	}

	if (IS_BUILTIN(CONFIG_FS_TFTP) && !stat(priv->tmp, &s))
		unlink(priv->tmp);

	free(priv->tmp);
	free(priv);
}

static inline int uimage_is_multi_image(struct uimagefs_handle *priv)
{
	return (priv->header.ih_type == IH_TYPE_MULTI) ? 1 : 0;
}

static int uimagefs_add_str(struct uimagefs_handle *priv, enum uimagefs_type type,
				char *s)
{
	struct uimagefs_handle_data *d;

	d = xzalloc(sizeof(*d));
	d->type = type;
	d->name = xstrdup(uimagefs_type_to_str(type));
	d->data = s;
	d->size = strlen(s);

	list_add_tail(&d->list, &priv->list);

	return 0;
}

static int uimagefs_add_name(struct uimagefs_handle *priv)
{
	char *name;
	struct image_header *header = &priv->header;

	if (header->ih_name[0]) {
		name = xzalloc(IH_NMLEN + 1);
		strncpy(name, header->ih_name, IH_NMLEN);
	} else {
		name = xstrdup(priv->filename);
	}

	return uimagefs_add_str(priv, UIMAGEFS_NAME, name);
}

static int uimagefs_add_hex(struct uimagefs_handle *priv, enum uimagefs_type type,
			uint32_t data)
{
	char *val = basprintf("0x%x", data);

	return uimagefs_add_str(priv, type, val);
}

static int __uimagefs_add_data(struct uimagefs_handle *priv, size_t offset,
				uint64_t size, int i)
{
	struct uimagefs_handle_data *d;
	const char *name = uimagefs_type_to_str(UIMAGEFS_DATA);

	d = xzalloc(sizeof(*d));
	d->type = UIMAGEFS_DATA;
	if (i < 0)
		d->name = xstrdup(name);
	else
		d->name = basprintf("%s%d", name, i);

	d->offset = offset;
	d->size = size;

	list_add_tail(&d->list, &priv->list);

	return 0;
}

static int uimagefs_add_data(struct uimagefs_handle *priv, size_t offset,
				uint64_t size)
{
	return __uimagefs_add_data(priv, offset, size, -1);
}

static int uimagefs_add_data_entry(struct uimagefs_handle *priv, size_t offset,
				uint64_t size)
{
	int ret;

	ret = __uimagefs_add_data(priv, offset, size, priv->nb_data_entries);
	if (ret)
		return ret;

	priv->nb_data_entries++;

	return 0;
}

#if defined(CONFIG_TIMESTAMP)
static int uimagefs_add_time(struct uimagefs_handle *priv)
{
	struct image_header *header = &priv->header;
	struct rtc_time tm;
	char *val;

	to_tm(header->ih_time, &tm);
	val = basprintf("%4d-%02d-%02d  %2d:%02d:%02d UTC",
			tm.tm_year, tm.tm_mon, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);

	return uimagefs_add_str(priv, UIMAGEFS_TIME, val);
}
#else
static int uimagefs_add_time(struct uimagefs_handle *priv)
{
	struct image_header *header = &priv->header;

	return uimagefs_add_hex(priv, UIMAGEFS_TIME, header->ih_time);
}
#endif

static int uimagefs_add_os(struct uimagefs_handle *priv)
{
	struct image_header *header = &priv->header;
	char *val = xstrdup(image_get_os_name(header->ih_os));

	return uimagefs_add_str(priv, UIMAGEFS_OS, val);
}

static int uimagefs_add_arch(struct uimagefs_handle *priv)
{
	struct image_header *header = &priv->header;
	char *val = xstrdup(image_get_arch_name(header->ih_arch));

	return uimagefs_add_str(priv, UIMAGEFS_ARCH, val);
}

static int uimagefs_add_type(struct uimagefs_handle *priv)
{
	struct image_header *header = &priv->header;
	char *val = xstrdup(image_get_type_name(header->ih_type));

	return uimagefs_add_str(priv, UIMAGEFS_TYPE, val);
}

static int uimagefs_add_comp(struct uimagefs_handle *priv)
{
	struct image_header *header = &priv->header;
	char *val = xstrdup(image_get_comp_name(header->ih_comp));

	return uimagefs_add_str(priv, UIMAGEFS_COMP, val);
}

/*
 * open a uimage. This will check the header contents and
 * return a handle to the uImage
 */
static int __uimage_open(struct uimagefs_handle *priv)
{
	int fd;
	uint32_t checksum;
	struct image_header *header;
	int ret;
	size_t offset = 0;
	size_t data_offset = 0;

again:
	fd = open(priv->filename, O_RDONLY);
	if (fd < 0) {
		printf("could not open: %s\n", errno_str());
		return fd;
	}

	/*
	 * Hack around tftp fs. We need lseek for uImage support, but
	 * this cannot be implemented in tftp fs, so we detect this
	 * and copy the file to ram if it fails
	 */
	if (IS_BUILTIN(CONFIG_FS_TFTP) && !can_lseek_backward(fd)) {
		close(fd);
		ret = copy_file(priv->filename, priv->tmp, 0);
		if (ret)
			return ret;
		priv->filename = priv->tmp;
		goto again;
	}

	header = &priv->header;

	ret = read(fd, header, sizeof(*header));
	if (ret < 0) {
		printf("could not read: %s\n", errno_str());
		goto err_out;
	}
	offset += sizeof(*header);

	if (uimage_to_cpu(header->ih_magic) != IH_MAGIC) {
		printf("Bad Magic Number\n");
		ret = -EINVAL;
		goto err_out;
	}

	checksum = uimage_to_cpu(header->ih_hcrc);
	header->ih_hcrc = 0;

	if (crc32(0, header, sizeof(*header)) != checksum) {
		printf("Bad Header Checksum\n");
		ret = -EIO;
		goto err_out;
	}

	/* convert header to cpu native endianess */
	header->ih_magic = uimage_to_cpu(header->ih_magic);
	header->ih_hcrc = checksum;
	header->ih_time = uimage_to_cpu(header->ih_time);
	header->ih_size = uimage_to_cpu(header->ih_size);
	header->ih_load = uimage_to_cpu(header->ih_load);
	header->ih_ep = uimage_to_cpu(header->ih_ep);
	header->ih_dcrc = uimage_to_cpu(header->ih_dcrc);

	ret = uimagefs_add_arch(priv);
	if (ret)
		goto err_out;

	ret = uimagefs_add_comp(priv);
	if (ret)
		goto err_out;

	ret = uimagefs_add_name(priv);
	if (ret)
		goto err_out;

	ret = uimagefs_add_os(priv);
	if (ret)
		goto err_out;

	ret = uimagefs_add_time(priv);
	if (ret)
		goto err_out;

	ret = uimagefs_add_type(priv);
	if (ret)
		goto err_out;

	ret = uimagefs_add_hex(priv, UIMAGEFS_LOAD, header->ih_load);
	if (ret)
		goto err_out;

	ret = uimagefs_add_hex(priv, UIMAGEFS_EP, header->ih_ep);
	if (ret)
		goto err_out;

	data_offset = offset;

	if (uimage_is_multi_image(priv)) {
		struct uimagefs_handle_data *d;

		do {
			u32 size;

			ret = read(fd, &size, sizeof(size));
			if (ret < 0)
				goto err_out;

			offset += sizeof(size);

			if (!size)
				break;

			ret = uimagefs_add_data_entry(priv, 0, uimage_to_cpu(size));
			if (ret)
				goto err_out;
		} while(1);

		/* offset of the first image in a multifile image */
		list_for_each_entry(d, &priv->list, list) {
			if (!uimagefs_is_data_file(d))
				continue;
			d->offset = offset;
			offset += (d->size + 3) & ~3;
		}
	} else {
		ret = uimagefs_add_data_entry(priv, offset, header->ih_size);
		if (ret)
			goto err_out;
	}

	ret = uimagefs_add_data(priv, data_offset, header->ih_size);
	if (ret)
		goto err_out;

	ret = uimagefs_add_hex(priv, UIMAGEFS_DATA_CRC, header->ih_dcrc);
	if (ret)
		goto err_out;

	ret = 0;
err_out:

	close(fd);

	return ret;
}

static int uimagefs_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev = dev_to_fs_device(dev);
	struct uimagefs_handle *priv;
	int ret = 0;

	priv = xzalloc(sizeof(struct uimagefs_handle));
	INIT_LIST_HEAD(&priv->list);
	dev->priv = priv;

	priv->filename = fsdev->backingstore;
	dev_dbg(dev, "mount: %s\n", fsdev->backingstore);

	if (IS_BUILTIN(CONFIG_FS_TFTP))
		priv->tmp = basprintf("/.uImage_tmp_%08x",
					crc32(0, fsdev->path, strlen(fsdev->path)));

	ret = __uimage_open(priv);
	if (ret)
		goto err;

	return 0;

err:
	uimagefs_remove(dev);

	return ret;
}

static struct fs_driver_d uimagefs_driver = {
	.open      = uimagefs_open,
	.close     = uimagefs_close,
	.read      = uimagefs_read,
	.lseek     = uimagefs_lseek,
	.opendir   = uimagefs_opendir,
	.readdir   = uimagefs_readdir,
	.closedir  = uimagefs_closedir,
	.stat      = uimagefs_stat,
	.ioctl	   = uimagefs_ioctl,
	.flags     = 0,
	.type = filetype_uimage,
	.drv = {
		.probe  = uimagefs_probe,
		.remove = uimagefs_remove,
		.name = "uimagefs",
	}
};

static int uimagefs_init(void)
{
	return register_fs_driver(&uimagefs_driver);
}
coredevice_initcall(uimagefs_init);
