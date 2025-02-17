/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */
#include <common.h>
#include <fs.h>

static struct inode *legacy_get_inode(struct super_block *sb, const struct inode *dir,
				      umode_t mode);

static int legacy_iterate(struct file *file, struct dir_context *ctx)
{
	struct dentry *dentry = file->f_path.dentry;
	struct inode *dir = d_inode(dentry);
	struct super_block *sb = dir->i_sb;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	const struct fs_legacy_ops *legacy_ops = fsdev->driver->legacy_ops;
	struct dir *d;
	struct dirent *dirent;
	char *pathname;

	dir_emit_dots(file, ctx);

	pathname = dpath(dentry, fsdev->vfsmount.mnt_root);

	d = legacy_ops->opendir(&fsdev->dev, pathname);
	if (!d)
		goto out;

	while (1) {
		dirent = legacy_ops->readdir(&fsdev->dev, d);
		if (!dirent)
			break;

		dir_emit(ctx, dirent->d_name, strlen(dirent->d_name), 0, DT_UNKNOWN);
	}

	legacy_ops->closedir(&fsdev->dev, d);
out:
	free(pathname);

	return 0;
}

static struct dentry *legacy_lookup(struct inode *dir, struct dentry *dentry,
				    unsigned int flags)
{
	struct super_block *sb = dir->i_sb;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	const struct fs_legacy_ops *legacy_ops = fsdev->driver->legacy_ops;
	struct inode *inode;
	char *pathname;
	struct stat s;
	int ret;

	pathname = dpath(dentry, fsdev->vfsmount.mnt_root);
	if (!pathname)
		return NULL;

	ret = legacy_ops->stat(&fsdev->dev, pathname, &s);
	if (!ret) {
		inode = legacy_get_inode(sb, dir, s.st_mode);
		if (!inode)
			return NULL;

		inode->i_size = s.st_size;
		inode->i_mode = s.st_mode;

		d_add(dentry, inode);
	}

	return NULL;
}

static int legacy_open(struct inode *inode, struct file *file)
{
	struct super_block *sb = inode->i_sb;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	const struct fs_legacy_ops *legacy_ops = fsdev->driver->legacy_ops;
	char *pathname;
	int error;

	if (!legacy_ops->open)
		return 0;

	pathname = dpath(file->f_dentry, fsdev->vfsmount.mnt_root);
	error = legacy_ops->open(&file->fsdev->dev, file, pathname);
	free(pathname);

	return error;
}

static int legacy_release(struct inode *inode, struct file *file)
{
	struct super_block *sb = inode->i_sb;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	const struct fs_legacy_ops *legacy_ops = fsdev->driver->legacy_ops;

	if (!legacy_ops->close)
		return 0;

	return legacy_ops->close(&file->fsdev->dev, file);
}

static int legacy_create(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	struct super_block *sb = dir->i_sb;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	const struct fs_legacy_ops *legacy_ops = fsdev->driver->legacy_ops;
	struct inode *inode;
	char *pathname;
	int ret;

	if (!legacy_ops->create)
		return -EROFS;

	pathname = dpath(dentry, fsdev->vfsmount.mnt_root);

	ret = legacy_ops->create(&fsdev->dev, pathname, mode | S_IFREG);

	free(pathname);

	if (ret)
		return ret;

	inode = legacy_get_inode(sb, dir, mode | S_IFREG);

	d_instantiate(dentry, inode);

	return 0;
}

static int legacy_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	struct super_block *sb = dir->i_sb;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	const struct fs_legacy_ops *legacy_ops = fsdev->driver->legacy_ops;
	struct inode *inode;
	char *pathname;
	int ret;

	if (!legacy_ops->mkdir)
		return -EROFS;

	pathname = dpath(dentry, fsdev->vfsmount.mnt_root);

	ret = legacy_ops->mkdir(&fsdev->dev, pathname);

	free(pathname);

	if (ret)
		return ret;

	inode = legacy_get_inode(sb, dir, mode | S_IFDIR);

	d_instantiate(dentry, inode);

	return 0;
}

static int legacy_dir_is_empty(struct dentry *dentry)
{
	struct inode *dir = d_inode(dentry);
	struct super_block *sb = dir->i_sb;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	const struct fs_legacy_ops *legacy_ops = fsdev->driver->legacy_ops;
	struct dir *d;
	struct dirent *dirent;
	char *pathname;

	pathname = dpath(dentry, fsdev->vfsmount.mnt_root);

	d = legacy_ops->opendir(&fsdev->dev, pathname);
	dirent = legacy_ops->readdir(&fsdev->dev, d);

	legacy_ops->closedir(&fsdev->dev, d);

	free(pathname);

	return dirent ? 0 : 1;
}

static int legacy_rmdir(struct inode *dir, struct dentry *dentry)
{
	struct super_block *sb = dir->i_sb;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	const struct fs_legacy_ops *legacy_ops = fsdev->driver->legacy_ops;
	char *pathname;
	int ret;

	if (!legacy_ops->rmdir)
		return -EROFS;

	if (!legacy_dir_is_empty(dentry))
		return -ENOTEMPTY;

	pathname = dpath(dentry, fsdev->vfsmount.mnt_root);

	ret = legacy_ops->rmdir(&fsdev->dev, pathname);

	free(pathname);

	if (ret)
		return ret;

	drop_nlink(d_inode(dentry));
	dput(dentry);
	drop_nlink(dir);

	return 0;
}

static int legacy_unlink(struct inode *dir, struct dentry *dentry)
{
	struct super_block *sb = dir->i_sb;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	const struct fs_legacy_ops *legacy_ops = fsdev->driver->legacy_ops;
	char *pathname;
	int ret;

	if (!legacy_ops->unlink)
		return -EROFS;

	pathname = dpath(dentry, fsdev->vfsmount.mnt_root);

	ret = legacy_ops->unlink(&fsdev->dev, pathname);

	free(pathname);

	if (ret)
		return ret;

	drop_nlink(d_inode(dentry));
	dput(dentry);

	return 0;
}

static int legacy_symlink(struct inode *dir, struct dentry *dentry,
			  const char *dest)
{
	struct super_block *sb = dir->i_sb;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	const struct fs_legacy_ops *legacy_ops = fsdev->driver->legacy_ops;
	struct inode *inode;
	char *pathname;
	int ret;

	if (!legacy_ops->symlink)
		return -ENOSYS;

	pathname = dpath(dentry, fsdev->vfsmount.mnt_root);

	ret = legacy_ops->symlink(&fsdev->dev, dest, pathname);

	free(pathname);

	if (ret)
		return ret;

	inode = legacy_get_inode(sb, dir, S_IFLNK);
	inode->i_link = xstrdup(dest);

	d_instantiate(dentry, inode);

	return 0;
}

static const char *legacy_get_link(struct dentry *dentry, struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	struct fs_device *fsdev = container_of(sb, struct fs_device, sb);
	const struct fs_legacy_ops *legacy_ops = fsdev->driver->legacy_ops;
	char *pathname;
	int ret;
	char link[PATH_MAX] = {};

	if (!legacy_ops->readlink)
		return ERR_PTR(-ENOSYS);

	pathname = dpath(dentry, fsdev->vfsmount.mnt_root);

	ret = legacy_ops->readlink(&fsdev->dev, pathname, link, PATH_MAX - 1);

	free(pathname);

	if (ret)
		return NULL;

	inode->i_link = xstrdup(link);

	return inode->i_link;
}

static const struct super_operations legacy_s_ops;
static const struct inode_operations legacy_file_inode_operations;

static const struct inode_operations legacy_dir_inode_operations = {
	.lookup = legacy_lookup,
	.create = legacy_create,
	.mkdir = legacy_mkdir,
	.rmdir = legacy_rmdir,
	.unlink = legacy_unlink,
	.symlink = legacy_symlink,
};

static const struct file_operations legacy_file_operations = {
	.open = legacy_open,
	.release = legacy_release,
};

static const struct file_operations legacy_dir_operations = {
	.iterate = legacy_iterate,
};

static const struct inode_operations legacy_symlink_inode_operations = {
	.get_link = legacy_get_link,
};

static struct inode *legacy_get_inode(struct super_block *sb, const struct inode *dir,
				      umode_t mode)
{
	struct inode *inode = new_inode(sb);

	if (!inode)
		return NULL;

	inode->i_ino = get_next_ino();
	inode->i_mode = mode;

	switch (mode & S_IFMT) {
	default:
		return NULL;
	case S_IFREG:
	case S_IFCHR:
	case S_IFBLK:
		inode->i_op = &legacy_file_inode_operations;
		inode->i_fop = &legacy_file_operations;
		break;
	case S_IFDIR:
		inode->i_op = &legacy_dir_inode_operations;
		inode->i_fop = &legacy_dir_operations;
		inc_nlink(inode);
		break;
	case S_IFLNK:
		inode->i_op = &legacy_symlink_inode_operations;
		break;
	}

	return inode;
}

int fs_init_legacy(struct fs_device *fsdev)
{
	struct inode *inode;

	fsdev->sb.s_op = &legacy_s_ops;
	inode = legacy_get_inode(&fsdev->sb, NULL, S_IFDIR);
	fsdev->sb.s_root = d_make_root(inode);

	return 0;
}
