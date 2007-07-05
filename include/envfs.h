#ifndef _ENVFS_H
#define _ENVFS_H

#define ENVFS_MAGIC		0x798fba79	/* some random number */
#define ENVFS_INODE_MAGIC	0x67a8c78d
#define ENVFS_END_MAGIC		0x6a87d6cd
#define ENVFS_SIGNATURE	"U-Boot envfs"

struct envfs_inode {
	u32 magic;	/* 0x67a8c78d */
	u32 crc;	/* crc for this inode and corresponding data */
	u32 size;	/* data size in bytes  */
	char name[32];
	char data[0];
};

/*
 * Superblock information at the beginning of the FS.
 */
struct envfs_super {
	u32 magic;			/* 0x798fba79 - random number */
	u32 priority;
	u32 flags;			/* feature flags */
	u32 future;			/* reserved for future use */
};

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ENVFS_16(x)	(x)
#define ENVFS_24(x)	(x)
#define ENVFS_32(x)	(x)
#define ENVFS_GET_NAMELEN(x)	((x)->namelen)
#define ENVFS_GET_OFFSET(x)	((x)->offset)
#define ENVFS_SET_OFFSET(x,y)	((x)->offset = (y))
#define ENVFS_SET_NAMELEN(x,y) ((x)->namelen = (y))
#elif __BYTE_ORDER == __BIG_ENDIAN
#ifdef __KERNEL__
#define ENVFS_16(x)	swab16(x)
#define ENVFS_24(x)	((swab32(x)) >> 8)
#define ENVFS_32(x)	swab32(x)
#else /* not __KERNEL__ */
#define ENVFS_16(x)	bswap_16(x)
#define ENVFS_24(x)	((bswap_32(x)) >> 8)
#define ENVFS_32(x)	bswap_32(x)
#endif /* not __KERNEL__ */
#endif

#endif /* _ENVFS_H */

