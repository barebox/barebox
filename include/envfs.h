#ifndef _ENVFS_H
#define _ENVFS_H

#ifdef __BAREBOX__
#include <asm/byteorder.h>
#endif

#define ENVFS_MAJOR		1
#define ENVFS_MINOR		0

#define ENVFS_MAGIC		    0x798fba79	/* some random number */
#define ENVFS_INODE_MAGIC	0x67a8c78d
#define ENVFS_INODE_END_MAGIC	0x68a8c78d
#define ENVFS_END_MAGIC		0x6a87d6cd
#define ENVFS_SIGNATURE	"barebox envfs"

struct envfs_inode {
	uint32_t magic;	/* ENVFS_INODE_MAGIC */
	uint32_t size;	/* data size in bytes  */
	uint32_t headerlen; /* The length of the filename _including_ a trailing 0 */
	char data[0];	/* The filename (zero terminated) + padding to 4 byte boundary
			 * followed by the data for this inode.
			 * The next inode follows after the data + padding to 4 byte
			 * boundary.
			 */
};

struct envfs_inode_end {
	uint32_t magic;	/* ENVFS_INODE_END_MAGIC */
	uint32_t mode;	/* file mode */
};

/*
 * Superblock information at the beginning of the FS.
 */
struct envfs_super {
	uint32_t magic;			/* ENVFS_MAGIC */
	uint32_t priority;
	uint32_t crc;			/* crc for the data */
	uint32_t size;			/* size of data */
	uint8_t major;			/* major */
	uint8_t minor;			/* minor */
	uint16_t future;		/* reserved for future use */
	uint32_t flags;			/* feature flags */
	uint32_t sb_crc;		/* crc for the superblock */
};

#ifdef __BAREBOX__
#  ifdef __LITTLE_ENDIAN
#    define ENVFS_ORDER_LITTLE
#  elif defined __BIG_ENDIAN
#    define ENVFS_ORDER_BIG
#  else
#    error "could not determine byte order"
#  endif
#else
#  if __BYTE_ORDER == __LITTLE_ENDIAN
#    define ENVFS_ORDER_LITTLE
#  elif __BYTE_ORDER == __BIG_ENDIAN
#    define ENVFS_ORDER_BIG
#  else
#    error "could not determine byte order"
#  endif
#endif

#ifdef ENVFS_ORDER_LITTLE
#define ENVFS_16(x)	(x)
#define ENVFS_24(x)	(x)
#define ENVFS_32(x)	(x)
#define ENVFS_GET_NAMELEN(x)	((x)->namelen)
#define ENVFS_GET_OFFSET(x)	((x)->offset)
#define ENVFS_SET_OFFSET(x,y)	((x)->offset = (y))
#define ENVFS_SET_NAMELEN(x,y) ((x)->namelen = (y))
#elif defined ENVFS_ORDER_BIG
#ifdef __KERNEL__
#define ENVFS_16(x)	swab16(x)
#define ENVFS_24(x)	((swab32(x)) >> 8)
#define ENVFS_32(x)	swab32(x)
#else /* not __KERNEL__ */
#define ENVFS_16(x)	bswap_16(x)
#define ENVFS_24(x)	((bswap_32(x)) >> 8)
#define ENVFS_32(x)	bswap_32(x)
#endif /* not __KERNEL__ */
#define ENVFS_GET_NAMELEN(x) ENVFS_32(((x)->namelen))
#define ENVFS_GET_OFFSET(x)	ENVFS_32(((x)->offset))
#define ENVFS_SET_NAMELEN(x,y)((x)->offset = ENVFS_32((y)))
#define ENVFS_SET_OFFSET(x,y) ((x)->namelen = ENVFS_32((y)))
#else
#error "__BYTE_ORDER must be __LITTLE_ENDIAN or __BIG_ENDIAN"
#endif

#define ENV_FLAG_NO_OVERWRITE	(1 << 0)
int envfs_load(char *filename, char *dirname, unsigned flags);
int envfs_save(char *filename, char *dirname);

/* defaults to /dev/env0 */
extern char *default_environment_path;

int envfs_register_partition(const char *devname, unsigned int partnr);

#endif /* _ENVFS_H */
