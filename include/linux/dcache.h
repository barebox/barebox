#ifndef __LINUX_DCACHE_H
#define __LINUX_DCACHE_H

/*
 * linux/include/linux/dcache.h
 *
 * Dirent cache data structures
 *
 * (C) Copyright 1997 Thomas Schoebel-Theuer,
 * with heavy changes by Linus Torvalds
 */

#include <linux/list.h>
#include <linux/spinlock.h>

#define IS_ROOT(x) ((x) == (x)->d_parent)

/* The hash is always the low bits of hash_len */
#ifdef __LITTLE_ENDIAN
 #define HASH_LEN_DECLARE u32 hash; u32 len
 #define bytemask_from_count(cnt)	(~(~0ul << (cnt)*8))
#else
 #define HASH_LEN_DECLARE u32 len; u32 hash
 #define bytemask_from_count(cnt)	(~(~0ul >> (cnt)*8))
#endif

/*
 * "quick string" -- eases parameter passing, but more importantly
 * saves "metadata" about the string (ie length and the hash).
 *
 * hash comes first so it snuggles against d_parent in the
 * dentry.
 */
struct qstr {
	union {
		struct {
			HASH_LEN_DECLARE;
		};
		u64 hash_len;
	};
	const unsigned char *name;
};

#define QSTR_INIT(n,l) { { { .len = l } }, .name = n }
#define hashlen_hash(hashlen) ((u32) (hashlen))
#define hashlen_len(hashlen)  ((u32)((hashlen) >> 32))
#define hashlen_create(hash,len) (((u64)(len)<<32)|(u32)(hash))

#define DNAME_INLINE_LEN_MIN 36

struct dentry {
	unsigned int d_flags;		/* protected by d_lock */
	spinlock_t d_lock;		/* per dentry lock */
	struct inode *d_inode;		/* Where the name belongs to - NULL is
					 * negative */

	unsigned int d_count;
	const struct dentry_operations *d_op;

	/*
	 * The next three fields are touched by __d_lookup.  Place them here
	 * so they all fit in a cache line.
	 */
	struct hlist_node d_hash;	/* lookup hash list */
	struct dentry *d_parent;	/* parent directory */
	struct qstr d_name;

	struct list_head d_lru;		/* LRU list */
	/*
	 * d_child and d_rcu can share memory
	 */
	struct list_head d_child;       /* child of parent list */
	struct list_head d_subdirs;	/* our children */
	unsigned long d_time;		/* used by d_revalidate */
	struct super_block *d_sb;	/* The root of the dentry tree */
	void *d_fsdata;			/* fs-specific data */
#ifdef CONFIG_PROFILING
	struct dcookie_struct *d_cookie; /* cookie, if any */
#endif
	int d_mounted;
	unsigned char *name;		/* all names */
};

struct dentry_operations {
};

struct dentry * d_make_root(struct inode *);
void d_add(struct dentry *, struct inode *);
struct dentry * d_alloc_anon(struct super_block *);
void d_set_d_op(struct dentry *dentry, const struct dentry_operations *op);
void d_instantiate(struct dentry *dentry, struct inode *inode);
void d_delete(struct dentry *);
struct dentry *dget(struct dentry *);
void dput(struct dentry *);

#define DCACHE_ENTRY_TYPE		0x00700000
#define DCACHE_MISS_TYPE		0x00000000 /* Negative dentry (maybe fallthru to nowhere) */
#define DCACHE_WHITEOUT_TYPE		0x00100000 /* Whiteout dentry (stop pathwalk) */
#define DCACHE_DIRECTORY_TYPE		0x00200000 /* Normal directory */
#define DCACHE_AUTODIR_TYPE		0x00300000 /* Lookupless directory (presumed automount) */
#define DCACHE_REGULAR_TYPE		0x00400000 /* Regular file type (or fallthru to such) */
#define DCACHE_SPECIAL_TYPE		0x00500000 /* Other file type (or fallthru to such) */
#define DCACHE_SYMLINK_TYPE		0x00600000 /* Symlink (or fallthru to such) */

#define DCACHE_FALLTHRU			0x01000000 /* Fall through to lower layer */
#define DCACHE_CANT_MOUNT		0x00000100
#define DCACHE_MOUNTED                  0x00010000 /* is a mountpoint */
#define DCACHE_NEED_AUTOMOUNT           0x00020000 /* handle automount on this dir */
#define DCACHE_MANAGED_DENTRY \
		(DCACHE_MOUNTED|DCACHE_NEED_AUTOMOUNT)

static inline bool d_mountpoint(const struct dentry *dentry)
{
	return dentry->d_flags & DCACHE_MOUNTED;
}

/*
 * Directory cache entry type accessor functions.
 */
static inline unsigned __d_entry_type(const struct dentry *dentry)
{
	return dentry->d_flags & DCACHE_ENTRY_TYPE;
}

static inline bool d_is_miss(const struct dentry *dentry)
{
	return __d_entry_type(dentry) == DCACHE_MISS_TYPE;
}

static inline bool d_can_lookup(const struct dentry *dentry)
{
	return __d_entry_type(dentry) == DCACHE_DIRECTORY_TYPE;
}

static inline bool d_is_autodir(const struct dentry *dentry)
{
	return __d_entry_type(dentry) == DCACHE_AUTODIR_TYPE;
}

static inline bool d_is_dir(const struct dentry *dentry)
{
	return d_can_lookup(dentry) || d_is_autodir(dentry);
}

static inline bool d_is_symlink(const struct dentry *dentry)
{
	return __d_entry_type(dentry) == DCACHE_SYMLINK_TYPE;
}

static inline bool d_is_reg(const struct dentry *dentry)
{
	return __d_entry_type(dentry) == DCACHE_REGULAR_TYPE;
}

static inline bool d_is_special(const struct dentry *dentry)
{
	return __d_entry_type(dentry) == DCACHE_SPECIAL_TYPE;
}

static inline bool d_is_file(const struct dentry *dentry)
{
	return d_is_reg(dentry) || d_is_special(dentry);
}

static inline bool d_is_negative(const struct dentry *dentry)
{
	// TODO: check d_is_whiteout(dentry) also.
	return d_is_miss(dentry);
}

static inline bool d_is_positive(const struct dentry *dentry)
{
	return !d_is_negative(dentry);
}

static inline struct inode *d_inode(const struct dentry *dentry)
{
	return dentry->d_inode;
}

#define IS_ROOT(x) ((x) == (x)->d_parent)

char *dpath(struct dentry *dentry, struct dentry *root);

#endif	/* __LINUX_DCACHE_H */
