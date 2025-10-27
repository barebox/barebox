/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _LINUX_FS_H
#define _LINUX_FS_H

#include <linux/list.h>
#include <linux/time.h>
#include <linux/stat.h>
#include <linux/mount.h>
#include <linux/path.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/rwsem.h>

/* Page cache limit. The filesystems should put that into their s_maxbytes
   limits, otherwise bad things can happen in VM. */
#if BITS_PER_LONG==32
#define MAX_LFS_FILESIZE	(((loff_t)PAGE_CACHE_SIZE << (BITS_PER_LONG-1))-1)
#elif BITS_PER_LONG==64
#define MAX_LFS_FILESIZE 	((loff_t)0x7fffffffffffffffLL)
#endif

/*
 * File types
 *
 * NOTE! These match bits 12..15 of stat.st_mode
 * (ie "(i_mode >> 12) & 15").
 */
#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14

/*
 * This is the "filldir" function type, used by readdir() to let
 * the kernel specify what kind of dirent layout it wants to have.
 * This allows the kernel to read directories into kernel space or
 * to have different dirent layouts depending on the binary type.
 */
struct dir_context;
typedef int (*filldir_t)(struct dir_context *, const char *, int, loff_t, u64,
			 unsigned);

struct dir_context {
	const filldir_t actor;
	loff_t pos;
};

/*
 * sb->s_flags.  Note that these mirror the equivalent MS_* flags where
 * represented in both.
 */
#define SB_RDONLY        1      /* Mount read-only */
#define SB_NOSUID        2      /* Ignore suid and sgid bits */
#define SB_NODEV         4      /* Disallow access to device special files */
#define SB_NOEXEC        8      /* Disallow program execution */
#define SB_SYNCHRONOUS  16      /* Writes are synced at once */
#define SB_MANDLOCK     64      /* Allow mandatory locks on an FS */
#define SB_DIRSYNC      128     /* Directory modifications are synchronous */
#define SB_NOATIME      1024    /* Do not update access times. */
#define SB_NODIRATIME   2048    /* Do not update directory access times */
#define SB_SILENT       32768
#define SB_POSIXACL     (1<<16) /* VFS does not apply the umask */
#define SB_KERNMOUNT    (1<<22) /* this is a kern_mount call */
#define SB_I_VERSION    (1<<23) /* Update inode I_version field */
#define SB_LAZYTIME     (1<<25) /* Update the on-disk [acm]times lazily */

/*
 * These are the fs-independent mount-flags: up to 32 flags are supported
 */
#define MS_RDONLY	 1	/* Mount read-only */
#define MS_NOSUID	 2	/* Ignore suid and sgid bits */
#define MS_NODEV	 4	/* Disallow access to device special files */
#define MS_NOEXEC	 8	/* Disallow program execution */
#define MS_SYNCHRONOUS	16	/* Writes are synced at once */
#define MS_REMOUNT	32	/* Alter flags of a mounted FS */
#define MS_MANDLOCK	64	/* Allow mandatory locks on an FS */
#define MS_DIRSYNC	128	/* Directory modifications are synchronous */
#define MS_NOATIME	1024	/* Do not update access times. */
#define MS_NODIRATIME	2048	/* Do not update directory access times */
#define MS_BIND		4096
#define MS_MOVE		8192
#define MS_REC		16384
#define MS_VERBOSE	32768	/* War is peace. Verbosity is silence.
				   MS_VERBOSE is deprecated. */
#define MS_SILENT	32768
#define MS_POSIXACL	(1<<16)	/* VFS does not apply the umask */
#define MS_UNBINDABLE	(1<<17)	/* change to unbindable */
#define MS_PRIVATE	(1<<18)	/* change to private */
#define MS_SLAVE	(1<<19)	/* change to slave */
#define MS_SHARED	(1<<20)	/* change to shared */
#define MS_RELATIME	(1<<21)	/* Update atime relative to mtime/ctime. */
#define MS_KERNMOUNT	(1<<22) /* this is a kern_mount call */
#define MS_I_VERSION	(1<<23) /* Update inode I_version field */
#define MS_ACTIVE	(1<<30)
#define MS_NOUSER	(1<<31)

#define SEEK_SET	0	/* seek relative to beginning of file */
#define SEEK_CUR	1	/* seek relative to current file position */
#define SEEK_END	2	/* seek relative to end of file */
#define SEEK_DATA	3	/* seek to the next data */
#define SEEK_HOLE	4	/* seek to the next hole */
#define SEEK_MAX	SEEK_HOLE

struct inode {
	struct hlist_node	i_hash;
	struct list_head	i_list;
	struct list_head	i_sb_list;
	struct list_head	i_dentry;
	unsigned long		i_ino;
	/*
	 * Filesystems may only read i_nlink directly.  They shall use the
	 * following functions for modification:
	 *
	 *    (set|clear|inc|drop)_nlink
	 *    inode_(inc|dec)_link_count
	 */
	union {
		const unsigned int i_nlink;
		unsigned int __i_nlink;
	};
	uid_t			i_uid;
	gid_t			i_gid;
	u64			i_version;
	loff_t			i_size;
	struct timespec		i_atime;
	struct timespec		i_mtime;
	struct timespec		i_ctime;
	unsigned int		i_blkbits;
	blkcnt_t		i_blocks;
	unsigned short          i_bytes;
	umode_t			i_mode;
	const struct inode_operations	*i_op;
	const struct file_operations	*i_fop;	/* former ->i_op->default_file_ops */
	struct super_block	*i_sb;

	__u32			i_generation;

	unsigned long		i_state;

	unsigned int		i_flags;
	unsigned int		i_count;

	char			*i_link;

	void			*i_private; /* fs or device private pointer */
};

struct super_block {
	struct list_head	s_list;		/* Keep this first */
	dev_t			s_dev;		/* search index; _not_ kdev_t */
	unsigned long		s_blocksize;
	unsigned char		s_blocksize_bits;
	unsigned char		s_dirt;
	unsigned long long	s_maxbytes;	/* Max file size */
	struct file_system_type	*s_type;
	const struct super_operations	*s_op;
	struct dquot_operations	*dq_op;
	struct quotactl_ops	*s_qcop;
	const struct export_operations *s_export_op;
	unsigned long		s_flags;
	unsigned long		s_magic;
	struct dentry		*s_root;
	struct rw_semaphore	s_umount;
	struct mutex		s_lock;
	int			s_count;
	int			s_syncing;
	int			s_need_sync_fs;
	struct xattr_handler	**s_xattr;

	struct list_head	s_inodes;	/* all inodes */
	struct list_head	s_dirty;	/* dirty inodes */
	struct list_head	s_io;		/* parked for writeback */
	struct list_head	s_more_io;	/* parked for more writeback */
	struct hlist_head	s_anon;		/* anonymous dentries for (nfs) exporting */
	struct list_head	s_files;
	/* s_dentry_lru and s_nr_dentry_unused are protected by dcache_lock */
	struct list_head	s_dentry_lru;	/* unused dentry lru */
	int			s_nr_dentry_unused;	/* # of dentry on lru */

	struct block_device	*s_bdev;
	struct mtd_info		*s_mtd;
	struct hlist_node	s_instances;

	int			s_frozen;
	wait_queue_head_t	s_wait_unfrozen;

	char s_id[32];				/* Informational name */

	void 			*s_fs_info;	/* Filesystem private info */

	/*
	 * The next field is for VFS *only*. No filesystems have any business
	 * even looking at it. You had been warned.
	 */
	struct mutex s_vfs_rename_mutex;	/* Kludge */

	/* Granularity of c/m/atime in ns.
	   Cannot be worse than a second */
	u32		   s_time_gran;

	/* Number of inodes with nlink == 0 but still referenced */

	const struct dentry_operations *s_d_op; /* default d_op for dentries */
};

struct file_system_type {
	const char *name;
	int fs_flags;
	int (*get_sb) (struct file_system_type *, int,
		       const char *, void *, struct vfsmount *);
	void (*kill_sb) (struct super_block *);
	struct module *owner;
	struct file_system_type * next;
	struct hlist_head fs_supers;
};

struct file {
	struct fs_device	*fsdev; /* The device this file belongs to */
	char			*path;
	struct path		f_path;
#define FILE_SIZE_STREAM	((loff_t) -1)
#define f_size f_inode->i_size
	struct inode		*f_inode;	/* cached value */
	const struct file_operations	*f_op;
	unsigned int 		f_flags;
	loff_t			f_pos;
	unsigned int		f_uid, f_gid;

	u64			f_version;
	/* private to the filesystem driver */
	void			*private_data;
};

/*
 * Attribute flags.  These should be or-ed together to figure out what
 * has been changed!
 */
#define ATTR_SIZE	(1 << 3)
#define ATTR_FILE	(1 << 13)

struct iattr {
	unsigned int	ia_valid;
	loff_t		ia_size;

	/*
	 * Not an attribute, but an auxiliary info for filesystems wanting to
	 * implement an ftruncate() like method.  NOTE: filesystem should
	 * check for (ia_valid & ATTR_FILE), and not for (ia_file != NULL).
	 */
	struct file	*ia_file;
};

struct super_operations {
	struct inode *(*alloc_inode)(struct super_block *sb);
	void (*destroy_inode)(struct inode *);
};

static inline struct inode *file_inode(const struct file *f)
{
	return f->f_inode;
}

/*
 * Inode flags - they have no relation to superblock flags now
 */
#define S_SYNC		1	/* Writes are synced at once */
#define S_NOATIME	2	/* Do not update access times */
#define S_APPEND	4	/* Append-only file */
#define S_IMMUTABLE	8	/* Immutable file */
#define S_DEAD		16	/* removed, but still open directory */
#define S_NOQUOTA	32	/* Inode is not counted to quota */
#define S_DIRSYNC	64	/* Directory modifications are synchronous */
#define S_NOCMTIME	128	/* Do not update file c/mtime */
#define S_SWAPFILE	256	/* Do not truncate: swapon got its bmaps */
#define S_PRIVATE	512	/* Inode is fs-internal */
#define S_IMA		1024	/* Inode has an associated IMA struct */
#define S_AUTOMOUNT	2048	/* Automount/referral quasi-directory */
#define S_NOSEC		4096	/* no suid or xattr security attributes */
#define S_DAX		0	/* Make all the DAX code disappear */

/*
 * Note that nosuid etc flags are inode-specific: setting some file-system
 * flags just means all the inodes inherit those flags by default. It might be
 * possible to override it selectively if you really wanted to with some
 * ioctl() that is not currently implemented.
 *
 * Exception: MS_RDONLY is always applied to the entire file system.
 *
 * Unfortunately, it is possible to change a filesystems flags with it mounted
 * with files in use.  This means that all of the inodes will not have their
 * i_flags updated.  Hence, i_flags no longer inherit the superblock mount
 * flags, so these have to be checked separately. -- rmk@arm.uk.linux.org
 */
#define __IS_FLG(inode, flg)	((inode)->i_sb->s_flags & (flg))

#define IS_RDONLY(inode)	((inode)->i_sb->s_flags & MS_RDONLY)
#define IS_SYNC(inode)		(__IS_FLG(inode, MS_SYNCHRONOUS) || \
					((inode)->i_flags & S_SYNC))
#define IS_DIRSYNC(inode)	(__IS_FLG(inode, MS_SYNCHRONOUS|MS_DIRSYNC) || \
					((inode)->i_flags & (S_SYNC|S_DIRSYNC)))
#define IS_MANDLOCK(inode)	__IS_FLG(inode, MS_MANDLOCK)
#define IS_NOATIME(inode)	__IS_FLG(inode, MS_RDONLY|MS_NOATIME)
#define IS_I_VERSION(inode)	__IS_FLG(inode, MS_I_VERSION)

#define IS_NOQUOTA(inode)	((inode)->i_flags & S_NOQUOTA)
#define IS_APPEND(inode)	((inode)->i_flags & S_APPEND)
#define IS_IMMUTABLE(inode)	((inode)->i_flags & S_IMMUTABLE)
#define IS_POSIXACL(inode)	__IS_FLG(inode, MS_POSIXACL)

#define IS_DEADDIR(inode)	((inode)->i_flags & S_DEAD)
#define IS_NOCMTIME(inode)	((inode)->i_flags & S_NOCMTIME)
#define IS_SWAPFILE(inode)	((inode)->i_flags & S_SWAPFILE)
#define IS_PRIVATE(inode)	((inode)->i_flags & S_PRIVATE)
#define IS_IMA(inode)		((inode)->i_flags & S_IMA)
#define IS_AUTOMOUNT(inode)	((inode)->i_flags & S_AUTOMOUNT)
#define IS_NOSEC(inode)		((inode)->i_flags & S_NOSEC)
#define IS_DAX(inode)		((inode)->i_flags & S_DAX)

#define IS_WHITEOUT(inode)	(S_ISCHR(inode->i_mode) && \
				 (inode)->i_rdev == WHITEOUT_DEV)

/*
 * Inode state bits.  Protected by inode->i_lock
 *
 * Three bits determine the dirty state of the inode, I_DIRTY_SYNC,
 * I_DIRTY_DATASYNC and I_DIRTY_PAGES.
 *
 * Four bits define the lifetime of an inode.  Initially, inodes are I_NEW,
 * until that flag is cleared.  I_WILL_FREE, I_FREEING and I_CLEAR are set at
 * various stages of removing an inode.
 *
 * Two bits are used for locking and completion notification, I_NEW and I_SYNC.
 *
 * I_DIRTY_SYNC		Inode is dirty, but doesn't have to be written on
 *			fdatasync().  i_atime is the usual cause.
 * I_DIRTY_DATASYNC	Data-related inode changes pending. We keep track of
 *			these changes separately from I_DIRTY_SYNC so that we
 *			don't have to write inode on fdatasync() when only
 *			mtime has changed in it.
 * I_DIRTY_PAGES	Inode has dirty pages.  Inode itself may be clean.
 * I_NEW		Serves as both a mutex and completion notification.
 *			New inodes set I_NEW.  If two processes both create
 *			the same inode, one of them will release its inode and
 *			wait for I_NEW to be released before returning.
 *			Inodes in I_WILL_FREE, I_FREEING or I_CLEAR state can
 *			also cause waiting on I_NEW, without I_NEW actually
 *			being set.  find_inode() uses this to prevent returning
 *			nearly-dead inodes.
 * I_WILL_FREE		Must be set when calling write_inode_now() if i_count
 *			is zero.  I_FREEING must be set when I_WILL_FREE is
 *			cleared.
 * I_FREEING		Set when inode is about to be freed but still has dirty
 *			pages or buffers attached or the inode itself is still
 *			dirty.
 * I_CLEAR		Added by clear_inode().  In this state the inode is
 *			clean and can be destroyed.  Inode keeps I_FREEING.
 *
 *			Inodes that are I_WILL_FREE, I_FREEING or I_CLEAR are
 *			prohibited for many purposes.  iget() must wait for
 *			the inode to be completely released, then create it
 *			anew.  Other functions will just ignore such inodes,
 *			if appropriate.  I_NEW is used for waiting.
 *
 * I_SYNC		Writeback of inode is running. The bit is set during
 *			data writeback, and cleared with a wakeup on the bit
 *			address once it is done. The bit is also used to pin
 *			the inode in memory for flusher thread.
 *
 * I_REFERENCED		Marks the inode as recently references on the LRU list.
 *
 * I_DIO_WAKEUP		Never set.  Only used as a key for wait_on_bit().
 *
 * I_WB_SWITCH		Cgroup bdi_writeback switching in progress.  Used to
 *			synchronize competing switching instances and to tell
 *			wb stat updates to grab mapping->tree_lock.  See
 *			inode_switch_wb_work_fn() for details.
 *
 * Q: What is the difference between I_WILL_FREE and I_FREEING?
 */
#define I_DIRTY_SYNC		(1 << 0)
#define I_DIRTY_DATASYNC	(1 << 1)
#define I_DIRTY_PAGES		(1 << 2)
#define __I_NEW			3
#define I_NEW			(1 << __I_NEW)
#define I_WILL_FREE		(1 << 4)
#define I_FREEING		(1 << 5)
#define I_CLEAR			(1 << 6)
#define __I_SYNC		7
#define I_SYNC			(1 << __I_SYNC)
#define I_REFERENCED		(1 << 8)
#define __I_DIO_WAKEUP		9
#define I_DIO_WAKEUP		(1 << __I_DIO_WAKEUP)
#define I_LINKABLE		(1 << 10)
#define I_DIRTY_TIME		(1 << 11)
#define __I_DIRTY_TIME_EXPIRED	12
#define I_DIRTY_TIME_EXPIRED	(1 << __I_DIRTY_TIME_EXPIRED)
#define I_WB_SWITCH		(1 << 13)

#define I_DIRTY (I_DIRTY_SYNC | I_DIRTY_DATASYNC | I_DIRTY_PAGES)
#define I_DIRTY_ALL (I_DIRTY | I_DIRTY_TIME)

static inline loff_t i_size_read(const struct inode *inode)
{
	return inode->i_size;
}

static inline void i_size_write(struct inode *inode, loff_t i_size)
{
	inode->i_size = i_size;
}

static inline void truncate_setsize(struct inode *inode, loff_t i_size)
{
	i_size_write(inode, i_size);
}

struct inode *new_inode(struct super_block *sb);
unsigned int get_next_ino(void);
void iput(struct inode *);
struct inode *iget(struct inode *);
void ihold(struct inode *inode);
void inc_nlink(struct inode *inode);
void clear_nlink(struct inode *inode);
void set_nlink(struct inode *inode, unsigned int nlink);

struct inode_operations {
	struct dentry * (*lookup) (struct inode *,struct dentry *, unsigned int);

	const char *(*get_link) (struct dentry *dentry, struct inode *inode);

	int (*create) (struct inode *,struct dentry *, umode_t);
	int (*link) (struct dentry *,struct inode *,struct dentry *);
	int (*unlink) (struct inode *,struct dentry *);
	int (*symlink) (struct inode *,struct dentry *,const char *);
	int (*mkdir) (struct inode *,struct dentry *,umode_t);
	int (*rmdir) (struct inode *,struct dentry *);
	int (*rename) (struct inode *, struct dentry *,
		       struct inode *, struct dentry *, unsigned int);
	int (*tmpfile)(struct inode *, struct file *, umode_t);
};

static inline ino_t parent_ino(struct dentry *dentry)
{
	return dentry->d_parent->d_inode->i_ino;
}

static inline bool dir_emit(struct dir_context *ctx,
			    const char *name, int namelen,
			    u64 ino, unsigned type)
{
	return ctx->actor(ctx, name, namelen, ctx->pos, ino, type) == 0;
}

static inline bool dir_emit_dot(struct file *file, struct dir_context *ctx)
{
	return ctx->actor(ctx, ".", 1, ctx->pos,
			file->f_path.dentry->d_inode->i_ino, DT_DIR) == 0;
}
static inline bool dir_emit_dotdot(struct file *file, struct dir_context *ctx)
{
	return ctx->actor(ctx, "..", 2, ctx->pos,
			parent_ino(file->f_path.dentry), DT_DIR) == 0;
}

static inline int dir_emit_dots(struct file *file, struct dir_context *ctx)
{
	if (ctx->pos == 0) {
		dir_emit_dot(file, ctx);
		ctx->pos = 1;
	}
	if (ctx->pos == 1) {
		dir_emit_dotdot(file, ctx);
		ctx->pos = 2;
	}
	return true;
}

enum erase_type;

struct file_operations {
	int (*open) (struct inode *, struct file *);
	int (*release) (struct inode *, struct file *);
	int (*iterate) (struct file *, struct dir_context *);
	int (*read)(struct file *f, void *buf, size_t size);
	int (*write)(struct file *f, const void *buf,
			size_t size);
	int (*flush)(struct file *f);
	int (*lseek)(struct file *f, loff_t pos);

	int (*ioctl)(struct file *f, unsigned int request, void *buf);
	int (*erase)(struct file *f, loff_t count,
			loff_t offset, enum erase_type type);
	int (*protect)(struct file *f, size_t count,
			loff_t offset, int prot);
	int (*discard_range)(struct file *f, loff_t count, loff_t offset);
	int (*memmap)(struct file *f, void **map, int flags);
	int (*truncate)(struct file *f, loff_t size);
};

void drop_nlink(struct inode *inode);

extern const struct file_operations simple_dir_operations;
extern const struct inode_operations simple_symlink_inode_operations;

extern void d_tmpfile(struct file *, struct inode *);

int simple_empty(struct dentry *dentry);
int simple_unlink(struct inode *dir, struct dentry *dentry);
int simple_rmdir(struct inode *dir, struct dentry *dentry);
struct dentry *simple_lookup(struct inode *, struct dentry *, unsigned int flags);
int dcache_readdir(struct file *, struct dir_context *);
const char *simple_get_link(struct dentry *dentry, struct inode *inode);
struct inode *iget_locked(struct super_block *, unsigned long);
void iget_failed(struct inode *inode);

static inline void inode_init_owner(struct inode *inode,
				    const struct inode *dir, umode_t mode)
{
	if (dir && dir->i_mode & S_ISGID) {
		inode->i_gid = dir->i_gid;

		/* Directories are special, and always inherit S_ISGID */
		if (S_ISDIR(mode))
			mode |= S_ISGID;
	}

	inode->i_mode = mode;
}

static inline void mark_inode_dirty(struct inode *inode) {}

int finish_open(struct file *file, struct dentry *dentry);

/* Helper for the simple case when original dentry is used */
static inline int finish_open_simple(struct file *file, int error)
{
	if (error)
		return error;

	return finish_open(file, file->f_path.dentry);
}

static inline void inode_inc_link_count(struct inode *inode)
{
	inc_nlink(inode);
	mark_inode_dirty(inode);
}

static inline void inode_dec_link_count(struct inode *inode)
{
	drop_nlink(inode);
	mark_inode_dirty(inode);
}

#endif /* _LINUX_FS_H */
