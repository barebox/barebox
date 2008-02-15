#ifndef _ENVFS_H
#define _ENVFS_H

#define ENVFS_MAGIC		    0x798fba79	/* some random number */
#define ENVFS_INODE_MAGIC	0x67a8c78d
#define ENVFS_END_MAGIC		0x6a87d6cd
#define ENVFS_SIGNATURE	"U-Boot envfs"

struct envfs_inode {
	uint32_t magic;	/* ENVFS_INODE_MAGIC */
	uint32_t size;	/* data size in bytes  */
	uint32_t namelen; /* The length of the filename _including_ a trailing 0 */
	char data[0];	/* The filename (zero terminated) + padding to 4 byte boundary
			 * followed by the data for this inode.
			 * The next inode follows after the data + padding to 4 byte
			 * boundary.
			 */
};

/*
 * Superblock information at the beginning of the FS.
 */
struct envfs_super {
	uint32_t magic;			/* ENVFS_MAGIC */
	uint32_t priority;
	uint32_t crc;			/* crc for the data */
	uint32_t size;			/* size of data */
	uint32_t flags;			/* feature flags */
	uint32_t future;		/* reserved for future use */
	uint32_t sb_crc;		/* crc for the superblock */
};

#ifndef __BYTE_ORDER
#error "No byte order defined in __BYTE_ORDER"
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#warning "envfs compiled on little endian host"
#define ENVFS_16(x)	(x)
#define ENVFS_24(x)	(x)
#define ENVFS_32(x)	(x)
#define ENVFS_GET_NAMELEN(x)	((x)->namelen)
#define ENVFS_GET_OFFSET(x)	((x)->offset)
#define ENVFS_SET_OFFSET(x,y)	((x)->offset = (y))
#define ENVFS_SET_NAMELEN(x,y) ((x)->namelen = (y))
#elif __BYTE_ORDER == __BIG_ENDIAN
#warning "envfs compiled on big endian host"
#ifdef __KERNEL__
#define ENVFS_16(x)	swab16(x)
#define ENVFS_24(x)	((swab32(x)) >> 8)
#define ENVFS_32(x)	swab32(x)
#else /* not __KERNEL__ */
#define ENVFS_16(x)	bswap_16(x)
#define ENVFS_24(x)	((bswap_32(x)) >> 8)
#define ENVFS_32(x)	bswap_32(x)
#endif /* not __KERNEL__ */
#define CRAMFS_GET_NAMELEN(x)	(((u8*)(x))[8] & 0x3f)
#define CRAMFS_GET_OFFSET(x)	((CRAMFS_24(((u32*)(x))[2] & 0xffffff) << 2) |\
				 ((((u32*)(x))[2] & 0xc0000000) >> 30))
#define CRAMFS_SET_NAMELEN(x,y)	(((u8*)(x))[8] = (((0x3f & (y))) | \
						  (0xc0 & ((u8*)(x))[8])))
#define CRAMFS_SET_OFFSET(x,y)	(((u32*)(x))[2] = (((y) & 3) << 30) | \
				 CRAMFS_24((((y) & 0x03ffffff) >> 2)) | \
				 (((u32)(((u8*)(x))[8] & 0x3f)) << 24))
#else
#error "__BYTE_ORDER must be __LITTLE_ENDIAN or __BIG_ENDIAN"
#endif

#endif /* _ENVFS_H */

