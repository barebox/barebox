// SPDX-License-Identifier: GPL-2.0+
// SPDX-FileCopyrightText: 2024 Ahmad Fatoum

#define pr_fmt(fmt) "qemu_fw_cfg-fs: " fmt

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <fs.h>
#include <string.h>
#include <libfile.h>
#include <errno.h>
#include <linux/stat.h>
#include <xfuncs.h>
#include <envfs.h>
#include <fcntl.h>
#include <linux/qemu_fw_cfg.h>
#include <wchar.h>
#include <linux/err.h>
#include <linux/ctype.h>

struct fw_cfg_fs_inode {
	struct inode inode;
	const char *name;
	struct list_head sibling;
	struct list_head children;
	char *buf;
};

struct fw_cfg_fs_data {
	int fd;
	int next_ino;
};

static struct fw_cfg_fs_inode *inode_to_node(struct inode *inode)
{
	return container_of(inode, struct fw_cfg_fs_inode, inode);
}

static const char *fw_cfg_fs_get_link(struct dentry *dentry, struct inode *inode)
{
	return inode->i_link;
}

static int fw_cfg_fs_open(struct inode *inode, struct file *file)
{
	struct fw_cfg_fs_inode *node = inode_to_node(inode);

	if (node->buf && (file->f_flags & O_ACCMODE) != O_RDONLY)
		return -EACCES;

	return 0;
}

static const struct inode_operations fw_cfg_fs_file_inode_operations;
static const struct inode_operations fw_cfg_fs_dir_inode_operations;
static const struct inode_operations fw_cfg_fs_symlink_inode_operations = {
	.get_link = fw_cfg_fs_get_link,
};
static const struct file_operations fw_cfg_fs_file_operations;
static const struct file_operations fw_cfg_fs_dir_operations;

static struct inode *fw_cfg_fs_get_inode(struct inode *iparent,
					 const char *name)
{
	struct fw_cfg_fs_inode *parent = inode_to_node(iparent);

	while (true) {
		struct fw_cfg_fs_inode *node;
		char *slash;

		slash = strchrnul(name, '/');

		list_for_each_entry(node, &parent->children, sibling) {
			size_t namelen = slash - name;
			if (!strncmp_ptr(name, node->name, namelen) &&
			    !node->name[namelen]) {
				if (*slash == '\0')
					return &node->inode;
				parent = node;
				goto next;
			}
		}

		return NULL;

next:
		name = slash + 1;
	}
}

static struct dentry *fw_cfg_fs_lookup(struct inode *dir,
				       struct dentry *dentry,
				       unsigned int flags)
{
	struct inode *inode;

	inode = fw_cfg_fs_get_inode(dir, dentry->name);
	if (IS_ERR_OR_NULL(inode))
		return ERR_CAST(inode);

	d_add(dentry, inode);

	return NULL;
}

static const struct inode_operations fw_cfg_fs_dir_inode_operations = {
	.lookup = fw_cfg_fs_lookup,
};

static struct fw_cfg_fs_inode *fw_cfg_fs_node_new(struct super_block *sb,
						  struct fw_cfg_fs_inode *parent,
						  const char *name,
						  ulong select,
						  umode_t mode)
{
	struct fw_cfg_fs_inode *node;
	struct inode *inode;


	inode = new_inode(sb);
	if (!inode)
		return NULL;

	inode->i_ino = select;
	inode->i_mode = 0777 | mode;
	node = inode_to_node(inode);
	node->name = strdup(name);

	switch (inode->i_mode & S_IFMT) {
	default:
		return ERR_PTR(-EINVAL);
	case S_IFREG:
		inode->i_op = &fw_cfg_fs_file_inode_operations;
		inode->i_fop = &fw_cfg_fs_file_operations;
		break;
	case S_IFDIR:
		inode->i_op = &fw_cfg_fs_dir_inode_operations;
		inode->i_fop = &fw_cfg_fs_dir_operations;
		inc_nlink(inode);
		break;
	case S_IFLNK:
		inode->i_op = &fw_cfg_fs_symlink_inode_operations;
		break;
	}

	if (parent)
		list_add_tail(&node->sibling, &parent->children);

	return node;
}

#define fw_cfg_fs_node_sprintf(node, args...)		\
do {							\
	node->inode.i_size = asprintf(&node->buf, args);\
} while (0)

static int fw_cfg_fs_parse(struct super_block *sb)
{
	struct fw_cfg_fs_data *data = sb->s_fs_info;
	struct fw_cfg_fs_inode *root, *by_key, *by_name;
	__be32 count;
	int i, ret;

	ioctl(data->fd, FW_CFG_SELECT, &(u16) { FW_CFG_FILE_DIR });

	lseek(data->fd, 0, SEEK_SET);

	ret = read(data->fd, &count, sizeof(count));
	if (ret < 0)
		return ret;

	root = inode_to_node(d_inode(sb->s_root));

	by_key = fw_cfg_fs_node_new(sb, root, "by_key", data->next_ino++, S_IFDIR);
	if (IS_ERR(by_key))
		return PTR_ERR(by_key);

	by_name = fw_cfg_fs_node_new(sb, root, "by_name", data->next_ino++, S_IFDIR);
	if (IS_ERR(by_name))
		return PTR_ERR(by_name);

	for (i = 0; i < be32_to_cpu(count); i++) {
		struct fw_cfg_fs_inode *parent, *dir, *node;
		struct fw_cfg_file qfile;
		char buf[sizeof("65536")];
		char *context, *name;
		int ndirs = 0;

		ret = read(data->fd, &qfile, sizeof(qfile));
		if (ret < 0)
			break;

		snprintf(buf, sizeof(buf), "%u", be16_to_cpu(qfile.select));

		dir = fw_cfg_fs_node_new(sb, by_key, buf, data->next_ino++, S_IFDIR);
		if (IS_ERR(dir))
			return PTR_ERR(dir);

		node = fw_cfg_fs_node_new(sb, dir, "name", data->next_ino++, S_IFREG);
		fw_cfg_fs_node_sprintf(node, "%s", qfile.name);

		node = fw_cfg_fs_node_new(sb, dir, "size", data->next_ino++, S_IFREG);
		fw_cfg_fs_node_sprintf(node, "%u", be32_to_cpu(qfile.size));

		node = fw_cfg_fs_node_new(sb, dir, "key", data->next_ino++, S_IFREG);
		fw_cfg_fs_node_sprintf(node, "%u", be16_to_cpu(qfile.select));

		node = fw_cfg_fs_node_new(sb, dir, "raw", be16_to_cpu(qfile.select), S_IFREG);
		node->inode.i_size = be32_to_cpu(qfile.size);

		for (const char *s = qfile.name; *s; s++) {
			if (*s == '/')
				ndirs++;
		}

		context = qfile.name;
		parent = by_name;

		while ((name = strsep(&context, "/"))) {
			struct fw_cfg_fs_inode *node;
			mode_t mode;

			list_for_each_entry(node, &parent->children, sibling) {
				if (streq_ptr(name, node->name)) {
					parent = node;
					goto next;
				}
			}

			mode = context && *context ? S_IFDIR : S_IFLNK;
			parent = fw_cfg_fs_node_new(sb, parent, name, data->next_ino++,
						    mode);
			if (IS_ERR(parent))
				break;
			if (mode == S_IFLNK) {
				char *s	= basprintf("%*sby_key/%s/raw",
						    (ndirs + 1) * 3, "", buf);

				parent->inode.i_link = s;
				while (*s == ' ')
					s = mempcpy(s, "../", 3);
			}
next:
			;
		}
	}

	return ret >= 0 ? 0 : ret;
}

static inline unsigned char dt_type(struct inode *inode)
{
	return (inode->i_mode >> 12) & 15;
}

static int fw_cfg_fs_dcache_readdir(struct file *file, struct dir_context *ctx)
{
	struct dentry *dentry = file->f_path.dentry;
	struct inode *iparent = d_inode(dentry);
	struct fw_cfg_fs_inode *node, *parent = inode_to_node(iparent);

	dir_emit_dots(file, ctx);

	list_for_each_entry(node, &parent->children, sibling) {
		dir_emit(ctx, node->name, strlen(node->name),
			 node->inode.i_ino, dt_type(&node->inode));
	}

	return 0;
}

static const struct file_operations fw_cfg_fs_dir_operations = {
	.iterate = fw_cfg_fs_dcache_readdir,
};

static struct inode *fw_cfg_fs_alloc_inode(struct super_block *sb)
{
	struct fw_cfg_fs_inode *node;

	node = xzalloc(sizeof(*node));

	INIT_LIST_HEAD(&node->children);
	INIT_LIST_HEAD(&node->sibling);

	return &node->inode;
}

static void fw_cfg_fs_destroy_inode(struct inode *inode)
{
	struct fw_cfg_fs_inode *node = inode_to_node(inode);

	list_del(&node->children);
	list_del(&node->sibling);
	free(node->buf);
	free(node);
}

static const struct super_operations fw_cfg_fs_ops = {
	.alloc_inode = fw_cfg_fs_alloc_inode,
	.destroy_inode = fw_cfg_fs_destroy_inode,
};

static int fw_cfg_fs_io(struct file *f, void *buf, size_t insize, bool read)
{
	struct inode *inode = f->f_inode;
	struct fw_cfg_fs_inode *node = inode_to_node(inode);
	struct fw_cfg_fs_data *data = f->fsdev->dev.priv;
	int fd = data->fd;

	if (node->buf) {
		if (!read)
			return -EBADF;

		memcpy(buf, node->buf + f->f_pos, insize);
		return insize;
	}

	ioctl(fd, FW_CFG_SELECT, &(u16) { inode->i_ino });

	if (read)
		return pread(fd, buf, insize, f->f_pos);
	else
		return pwrite(fd, buf, insize, f->f_pos);
}

static int fw_cfg_fs_read(struct file *f, void *buf, size_t insize)
{
	return fw_cfg_fs_io(f, buf, insize, true);
}

static int fw_cfg_fs_write(struct file *f, const void *buf, size_t insize)
{
	return fw_cfg_fs_io(f, (void *)buf, insize, false);
}

static const struct file_operations fw_cfg_fs_file_operations = {
	.open = fw_cfg_fs_open,
	.read = fw_cfg_fs_read,
	.write = fw_cfg_fs_write,
};

static int fw_cfg_fs_probe(struct device *dev)
{
	struct fw_cfg_fs_inode *node;
	struct fw_cfg_fs_data *data = xzalloc(sizeof(*data));
	struct fs_device *fsdev = dev_to_fs_device(dev);
	struct super_block *sb = &fsdev->sb;
	int ret;

	dev->priv = data;

	data->next_ino = U16_MAX + 1;
	data->fd = open(fsdev->backingstore, O_RDWR);
	if (data->fd < 0) {
		ret = -errno;
		goto free_data;
	}

	sb->s_op = &fw_cfg_fs_ops;
	node = fw_cfg_fs_node_new(sb, NULL, NULL, data->next_ino++, S_IFDIR);
	if (IS_ERR(node))
		return PTR_ERR(node);
	sb->s_root = d_make_root(&node->inode);
	sb->s_fs_info = data;


	/*
	 * We don't use cdev * directly, but this is needed for
	 * cdev_get_mount_path() to work right
	 */
	fsdev->cdev = cdev_by_name(devpath_to_name(fsdev->backingstore));

	ret = fw_cfg_fs_parse(sb);
	if (ret)
		goto free_data;

	defaultenv_append_runtime_directory("/mnt/fw_cfg/by_name/opt/org.barebox.env");

	return 0;
free_data:
	free(data);
	return ret;
}

static void fw_cfg_fs_remove(struct device *dev)
{
	struct fw_cfg_fs_data *data = dev->priv;

	flush(data->fd);
	close(data->fd);
	free(data);
}

static struct fs_driver fw_cfg_fs_driver = {
	.type = filetype_qemu_fw_cfg,
	.drv = {
		.probe = fw_cfg_fs_probe,
		.remove = fw_cfg_fs_remove,
		.name = "qemu_fw_cfg-fs",
	}
};

static int qemu_fw_cfg_fs_init(void)
{
	return register_fs_driver(&fw_cfg_fs_driver);
}
coredevice_initcall(qemu_fw_cfg_fs_init);

static int qemu_fw_cfg_early_mount(void)
{
	struct cdev *cdev;
	struct device *dev;
	const char *mntpath;
	int dirfd, fd;

	cdev = cdev_by_name("fw_cfg");
	if (!cdev)
		return 0;

	/*
	 * Trigger a mount, so ramfb device can be detected and
	 * environment can be loaded
	 */
	mntpath = cdev_mount(cdev);
	if (IS_ERR(mntpath))
		return PTR_ERR(mntpath);

	dirfd = open(mntpath, O_PATH | O_DIRECTORY);
	if (dirfd < 0)
		return dirfd;

	fd = openat(dirfd, "by_name/etc/ramfb", O_WRONLY);
	close(dirfd);
	if (fd >= 0) {
		dev = device_alloc("qemu-ramfb", DEVICE_ID_SINGLE);
		dev->parent = cdev->dev;
		dev->platform_data = (void *)(uintptr_t)fd;
		platform_device_register(dev);
	}

	return 0;
}
late_initcall(qemu_fw_cfg_early_mount);
