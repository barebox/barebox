#ifndef _USB_DFU_TRAILER_H
#define _USB_DFU_TRAILER_H

/* trailer handling for DFU files */

#define BAREBOX_DFU_TRAILER_V1	1
#define BAREBOX_DFU_TRAILER_MAGIC	0x19731978
struct barebox_dfu_trailer {
	u_int32_t	magic;
	u_int16_t	version;
	u_int16_t	length;
	u_int16_t	vendor;
	u_int16_t	product;
	u_int32_t	revision;
} __attribute__((packed));

/* we mirror the trailer because we want it to be longer in later versions
 * while keeping backwards compatibility */
static inline void dfu_trailer_mirror(struct barebox_dfu_trailer *trailer,
				      unsigned char *eof)
{
	int i;
	int len = sizeof(struct barebox_dfu_trailer);
	unsigned char *src = eof - len;
	unsigned char *dst = (unsigned char *) trailer;

	for (i = 0; i < len; i++)
		dst[len-1-i] = src[i];
}

#endif /* _USB_DFU_TRAILER_H */
