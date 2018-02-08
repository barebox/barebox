#ifndef __INCLUDE_IMAGE_METADTA_H
#define __INCLUDE_IMAGE_METADTA_H

/*
 * barebox Image MetaData (IMD)
 *
 * IMD is a mechanism to store metadata in barebox images. With IMD
 * it's possible to extract the release, the build timestamp and the
 * board type from a barebox image.
 *
 * Since there is no fixed place in the image suitable for all SoC image
 * types the metadata can be stored anywhere in the image and is found
 * by iterating over the image. The metadata starts with a start header
 * and ends with an end header. All tags in between carry the payload.
 *
 * Make sure source files containing IMD data are compiled with lwl-y so
 * that the tags end up in the PBL if one exists and in the regular image
 * without PBL.
 *
 * The following types exist:
 */
#define IMD_TYPE_START		0x640c0001
#define IMD_TYPE_RELEASE	0x640c8002 /* The barebox release aka UTS_RELEASE */
#define IMD_TYPE_BUILD		0x640c8003 /* build number and timestamp (UTS_VERSION) */
#define IMD_TYPE_MODEL		0x640c8004 /* The board name this image is for */
#define IMD_TYPE_OF_COMPATIBLE	0x640c8005 /* the device tree compatible string */
#define IMD_TYPE_PARAMETER	0x640c8006 /* A generic parameter. Use key=value as data */
#define IMD_TYPE_END		0x640c7fff
#define IMD_TYPE_INVALID	0xffffffff

/*
 * The IMD header. All data is stored in little endian format in the image.
 * The next header starts at the next 4 byte boundary after the data.
 */
struct imd_header {
	uint32_t type;		/* One of IMD_TYPE_* above */
	uint32_t datalength;	/* Length of the data (exluding the header) */
};

/*
 * A IMD string. Set bit 15 of the IMD_TYPE to indicate the data is printable
 * as string.
 */
struct imd_entry_string {
	struct imd_header header;
	char data[];
};

static inline int imd_is_string(uint32_t type)
{
	return (type & 0x8000) ? 1 : 0;
}

static inline int imd_type_valid(uint32_t type)
{
	return (type & 0xffff0000) == 0x640c0000;
}

const struct imd_header *imd_next(const struct imd_header *imd);

#define imd_for_each(start, imd) \
	for (imd = imd_next(start); imd_read_type(imd) != IMD_TYPE_END; imd = imd_next(imd))

static inline uint32_t imd_read_le32(const void *_ptr)
{
	const uint8_t *ptr = _ptr;

	return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
}

static inline uint32_t imd_read_type(const struct imd_header *imd)
{
	return imd_read_le32(&imd->type);
}

static inline uint32_t imd_read_length(const struct imd_header *imd)
{
	return imd_read_le32(&imd->datalength);
}

const struct imd_header *imd_find_type(const struct imd_header *imd,
				       uint32_t type);

const struct imd_header *imd_get(const void *buf, int size);
const char *imd_string_data(const struct imd_header *imd, int index);
const char *imd_type_to_name(uint32_t type);
char *imd_concat_strings(const struct imd_header *imd);
const char *imd_get_param(const struct imd_header *imd, const char *name);

extern int imd_command_verbose;
int imd_command_setenv(const char *variable_name, const char *value);
int imd_command(int argc, char *argv[]);

#ifdef __BAREBOX__

#include <linux/stringify.h>

#define __BAREBOX_IMD_SECTION(_section) \
	__attribute__ ((unused,section (__stringify(_section)))) \
	__attribute__((aligned(4)))

#define BAREBOX_IMD_TAG_STRING(_name, _type, _string, _keep_if_unused)			\
	const struct imd_entry_string __barebox_imd_##_name				\
	__BAREBOX_IMD_SECTION(.barebox_imd_ ## _keep_if_unused ## _ ## _name) = {	\
		.header.type = cpu_to_le32(_type),					\
		.header.datalength = cpu_to_le32(sizeof(_string)),			\
		.data = _string,							\
	}


#ifdef CONFIG_IMD
void imd_used(const void *);
#else
static inline void imd_used(const void *unused)
{
}
#endif

#define IMD_USED(_name) \
	imd_used(&__barebox_imd_##_name)

#endif /* __BAREBOX__ */

#endif /* __INCLUDE_IMAGE_METADTA_H */
