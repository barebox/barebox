/*
 * Copyright (c) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Simple update file format developed for Somfy, tools and library are
 * available under LGPLv2 (https://www.gitorious.org/libbpk).
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
#include <bpkfs.h>
#include <libgen.h>

static bool bpkfs_is_crc_file(struct bpkfs_handle_data *d)
{
	return d->type & (1 << 31);
}

const char* bpkfs_type_to_str(uint32_t type)
{
	switch (type) {
	case BPKFS_TYPE_BL:
		return "bootloader";
	case BPKFS_TYPE_BLV:
		return "bootloader_version";
	case BPKFS_TYPE_DSC:
		return "description.gz";
	case BPKFS_TYPE_KER:
		return "kernel";
	case BPKFS_TYPE_RFS:
		return "rootfs";
	case BPKFS_TYPE_FMV:
		return "firmware_version";
	}

	return NULL;
}

static struct bpkfs_handle_hw *bpkfs_get_by_hw_id(
	struct bpkfs_handle *handle, uint32_t hw_id)
{
	struct bpkfs_handle_hw *h;

	list_for_each_entry(h, &handle->list, list_hw_id) {
		if (h->hw_id == hw_id)
			return h;
	}

	return NULL;
}

static struct bpkfs_handle_hw *bpkfs_hw_id_get_by_name(
	struct bpkfs_handle *handle, const char *name)
{
	struct bpkfs_handle_hw *h;

	if (!name)
		return NULL;

	list_for_each_entry(h, &handle->list, list_hw_id) {
		if (strcmp(h->name, name) == 0)
			return h;
	}

	return NULL;
}

static struct bpkfs_handle_data *bpkfs_data_get_by_name(
	struct bpkfs_handle_hw *h, const char *name)
{
	struct bpkfs_handle_data *d;

	if (!name)
		return NULL;

	list_for_each_entry(d, &h->list_data, list) {
		if (strcmp(d->name, name) == 0)
			return d;
	}

	return NULL;
}

static struct bpkfs_handle_hw *bpkfs_get_or_add_hw_id(
	struct bpkfs_handle *handle, uint32_t hw_id)
{
	struct bpkfs_handle_hw *h;

	h = bpkfs_get_by_hw_id(handle, hw_id);
	if (h)
		return h;

	h = xzalloc(sizeof(*h));

	INIT_LIST_HEAD(&h->list_data);
	h->hw_id = hw_id;
	h->name = asprintf("hw_id_%x", hw_id);
	list_add_tail(&h->list_hw_id, &handle->list);

	return h;
}

static struct bpkfs_handle_data *bpkfs_get_by_type(
	struct bpkfs_handle *handle, uint32_t hw_id, uint32_t type)
{
	struct bpkfs_handle_data *d;
	struct bpkfs_handle_hw *h;

	h = bpkfs_get_by_hw_id(handle, hw_id);
	if (!h)
		return NULL;

	list_for_each_entry(d, &h->list_data, list) {
		if (d->type == type)
			return d;
	}

	return NULL;
}

static int bpkfs_open(struct device_d *dev, FILE *f, const char *filename)
{
	struct bpkfs_handle *priv = dev->priv;
	struct bpkfs_handle_data *d;
	struct bpkfs_handle_hw *h;
	char *dir, *file;
	int ret = -EINVAL;
	char *tmp = xstrdup(filename);
	char *tmp2 = xstrdup(filename);

	dir = dirname(tmp);

	if (dir[0] == '/')
		dir++;

	h = bpkfs_hw_id_get_by_name(priv, dir);
	if (!h)
		goto out;

	file = basename(tmp2);
	d = bpkfs_data_get_by_name(h, file);
	if (!d)
		goto out;

	if (!bpkfs_is_crc_file(d)) {
		d->fd = open(priv->filename, O_RDONLY);
		if (d->fd < 0) {
			ret = d->fd;
			goto out;
		}

		lseek(d->fd, d->offset, SEEK_SET);
	}

	f->size = d->size;
	f->inode = d;
	ret = 0;

out:
	free(tmp);
	free(tmp2);
	return ret;
}

static int bpkfs_close(struct device_d *dev, FILE *file)
{
	struct bpkfs_handle_data *d = file->inode;

	close(d->fd);

	return 0;
}

static int bpkfs_read(struct device_d *dev, FILE *file, void *buf, size_t insize)
{
	struct bpkfs_handle_data *d = file->inode;

	if (bpkfs_is_crc_file(d)) {
		memcpy(buf, &d->data[d->pos], insize);
		return insize;
	} else {
		return read(d->fd, buf, insize);
	}
}

static loff_t bpkfs_lseek(struct device_d *dev, FILE *file, loff_t pos)
{
	struct bpkfs_handle_data *d = file->inode;

	if (!bpkfs_is_crc_file(d))
		lseek(d->fd, d->offset + pos, SEEK_SET);

	d->pos = pos;

	return pos;
}

struct somfy_readdir {
	struct bpkfs_handle_hw *h;
	struct bpkfs_handle_data *d;

	DIR dir;
};

static DIR *bpkfs_opendir(struct device_d *dev, const char *pathname)
{
	struct bpkfs_handle *priv = dev->priv;
	struct somfy_readdir *sdir;
	DIR *dir;

	sdir = xzalloc(sizeof(*sdir));
	dir = &sdir->dir;
	dir->priv = sdir;

	if (pathname[0] == '/')
		pathname++;

	if (!strlen(pathname)) {
		if (list_empty(&priv->list))
			return dir;

		sdir->h = list_first_entry(&priv->list,
					struct bpkfs_handle_hw, list_hw_id);
	} else {
		sdir->h = bpkfs_hw_id_get_by_name(priv, pathname);
		if (!sdir->h || list_empty(&sdir->h->list_data))
			return dir;

		sdir->d = list_first_entry(&sdir->h->list_data,
					struct bpkfs_handle_data, list);
	}

	return dir;
}

static struct dirent *bpkfs_readdir(struct device_d *dev, DIR *dir)
{
	struct bpkfs_handle *priv = dev->priv;
	struct somfy_readdir *sdir = dir->priv;
	struct bpkfs_handle_hw *h = sdir->h;
	struct bpkfs_handle_data *d = sdir->d;

	if (!h)
		return NULL;

	if (!d) {
		if (&h->list_hw_id == &priv->list)
			return NULL;

		strcpy(dir->d.d_name, h->name);
		sdir->h = list_entry(h->list_hw_id.next, struct bpkfs_handle_hw, list_hw_id);
	} else {
		if (&d->list == &h->list_data)
			return NULL;

		strcpy(dir->d.d_name, d->name);
		sdir->d = list_entry(d->list.next, struct bpkfs_handle_data, list);
	}

	return &dir->d;
}

static int bpkfs_closedir(struct device_d *dev, DIR *dir)
{
	struct somfy_readdir *sdir = dir->priv;

	free(sdir);
	return 0;
}

static int bpkfs_stat(struct device_d *dev, const char *filename, struct stat *s)
{
	struct bpkfs_handle *priv = dev->priv;
	struct bpkfs_handle_data *d;
	struct bpkfs_handle_hw *h;
	char *dir, *file;
	int ret = -EINVAL;
	char *tmp = xstrdup(filename);
	char *tmp2 = xstrdup(filename);

	dir = dirname(tmp);

	if (filename[0] == '/')
		filename++;

	if (dir[0] == '/')
		dir++;

	if (!strlen(dir)) {
		h = bpkfs_hw_id_get_by_name(priv, filename);
		if (!h)
			goto out;

		s->st_size = strlen(filename);
		s->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
		ret = 0;
		goto out;
	}
	h = bpkfs_hw_id_get_by_name(priv, dir);
	if (!h)
		goto out;

	file = basename(tmp2);
	d = bpkfs_data_get_by_name(h, file);
	if (!d)
		goto out;

	s->st_size = d->size;
	s->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

	ret = 0;

out:
	free(tmp);
	free(tmp2);
	return ret;
}

static void bpkfs_remove_data(struct bpkfs_handle_hw *h)
{
	struct bpkfs_handle_data *d, *tmp;

	list_for_each_entry_safe(d, tmp, &h->list_data, list) {
		free(d->name);
		free(d);
	}
}

static void bpkfs_remove(struct device_d *dev)
{
	struct bpkfs_handle *priv = dev->priv;
	struct bpkfs_handle_hw *h, *tmp;

	list_for_each_entry_safe(h, tmp, &priv->list, list_hw_id) {
		bpkfs_remove_data(h);
		free(h->name);
		free(h);
	}

	free(priv);
}

static int bpkfs_probe(struct device_d *dev)
{
	struct fs_device_d *fsdev = dev_to_fs_device(dev);
	struct bpkfs_handle *priv;
	struct bpkfs_header *header;
	struct bpkfs_data_header data_header;
	int ret = 0;
	uint32_t checksum, crc;
	uint64_t size;
	int i;
	size_t offset = 0;
	char *buf;
	int fd;

	priv = xzalloc(sizeof(struct bpkfs_handle));
	INIT_LIST_HEAD(&priv->list);
	buf = xmalloc(2048);
	dev->priv = priv;

	priv->filename = fsdev->backingstore;
	dev_dbg(dev, "mount: %s\n", fsdev->backingstore);

	fd = open(fsdev->backingstore, O_RDONLY);
	if (fd < 0) {
		ret = fd;
		goto err;
	}

	header = &priv->header;

	ret = read(fd, header, sizeof(*header));
	if (ret < 0) {
		dev_err(dev, "could not read: %s (ret = %d)\n", errno_str(), ret);
		goto err;
	}

	dev_dbg(dev, "header.magic = 0x%x\n", be32_to_cpu(header->magic));
	dev_dbg(dev, "header.version = 0x%x\n", be32_to_cpu(header->version));
	dev_dbg(dev, "header.crc = 0x%x\n", be32_to_cpu(header->crc));
	dev_dbg(dev, "header.size = %llu\n", be64_to_cpu(header->size));
	dev_dbg(dev, "header.spare = %llu\n", be64_to_cpu(header->spare));

	size = be64_to_cpu(header->size);
	offset += sizeof(*header);
	size -= sizeof(*header);

	checksum = be32_to_cpu(header->crc);
	header->crc = 0;

	crc = crc32(0, header, sizeof(*header));

	for (i = 0; size; i++) {
		struct bpkfs_handle_data *d;
		struct bpkfs_handle_hw *h;
		const char *type;

		ret = read(fd, &data_header, sizeof(data_header));
		if (ret < 0) {
			dev_err(dev, "could not read: %s\n", errno_str());
			goto err;
		} else if (ret == 0) {
			dev_err(dev, "EOF: to_read %llu\n", size);
			goto err;
		}

		d = xzalloc(sizeof(*d));

		crc = crc32(crc, &data_header, sizeof(data_header));
		offset += sizeof(data_header);
		size -= sizeof(data_header);

		d->type = be32_to_cpu(data_header.type);
		d->hw_id = be32_to_cpu(data_header.hw_id);
		d->size = be64_to_cpu(data_header.size);
		d->offset = offset;
		d->crc = be32_to_cpu(data_header.crc);
		type = bpkfs_type_to_str(d->type);

		h = bpkfs_get_or_add_hw_id(priv, d->hw_id);

		if (!type) {
			type = "unknown";
			d->name = asprintf("%s_%08x", type, d->type);
		} else {
			d->name = xstrdup(type);
		}

		dev_dbg(dev, "%d: type = 0x%x => %s\n", i, d->type, d->name);
		dev_dbg(dev, "%d: size = %llu\n", i, d->size);
		dev_dbg(dev, "%d: offset = %d\n", i, d->offset);

		dev_dbg(dev, "%d: hw_id = 0x%x => %s\n", i, h->hw_id, h->name);

		offset += d->size;
		size -= d->size;

		if (bpkfs_get_by_type(priv, d->hw_id, d->type)) {
			dev_info(dev, "ignore data %d type %s already present, ignored\n",
				 i, type);
			free(d);
			continue;
		}

		list_add_tail(&d->list, &h->list_data);
		priv->nb_data_entries++;

		ret = lseek(fd, d->size, SEEK_CUR);
		if (ret < 0) {
			dev_err(dev, "could not seek: %s\n", errno_str());
			goto err;
		}

		type = d->name;
		d = xzalloc(sizeof(*d));
		d->type = be32_to_cpu(data_header.type);
		d->name = asprintf("%s.crc", type);
		d->type |= (1 << 31);
		d->size = 8;
		sprintf(d->data, "%08x", be32_to_cpu(data_header.crc));
		list_add_tail(&d->list, &h->list_data);
	}

	if (crc != checksum) {
		dev_err(dev, "invalid crc (0x%x != 0x%x)\n", checksum, crc);
		goto err;
	}

	close(fd);
	free(buf);

	return 0;

err:
	close(fd);
	free(buf);
	bpkfs_remove(dev);

	return ret;
}

static struct fs_driver_d bpkfs_driver = {
	.open      = bpkfs_open,
	.close     = bpkfs_close,
	.read      = bpkfs_read,
	.lseek     = bpkfs_lseek,
	.opendir   = bpkfs_opendir,
	.readdir   = bpkfs_readdir,
	.closedir  = bpkfs_closedir,
	.stat      = bpkfs_stat,
	.flags     = 0,
	.type = filetype_bpk,
	.drv = {
		.probe  = bpkfs_probe,
		.remove = bpkfs_remove,
		.name = "bpkfs",
	}
};

static int bpkfs_init(void)
{
	return register_fs_driver(&bpkfs_driver);
}
coredevice_initcall(bpkfs_init);
