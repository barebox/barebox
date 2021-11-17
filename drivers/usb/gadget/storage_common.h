/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef USB_STORAGE_COMMON_H
#define USB_STORAGE_COMMON_H

#include <driver.h>
#include <usb/storage.h>
#include <asm/unaligned.h>
#include <usb/mass_storage.h>

#ifndef DEBUG
#undef VERBOSE_DEBUG
#undef DUMP_MSGS
#endif /* !DEBUG */

#define VLDBG(lun, fmt, args...)  dev_vdbg(&(lun)->dev, fmt, ## args)
#define LDBG(lun, fmt, args...)   dev_dbg (&(lun)->dev, fmt, ## args)
#define LERROR(lun, fmt, args...) dev_err (&(lun)->dev, fmt, ## args)
#define LWARN(lun, fmt, args...)  dev_warn(&(lun)->dev, fmt, ## args)
#define LINFO(lun, fmt, args...)  dev_info(&(lun)->dev, fmt, ## args)

/*
 * Keep those macros in sync with those in
 * include/linux/usb/composite.h or else GCC will complain.  If they
 * are identical (the same names of arguments, white spaces in the
 * same places) GCC will allow redefinition otherwise (even if some
 * white space is removed or added) warning will be issued.
 *
 * Those macros are needed here because File Storage Gadget does not
 * include the composite.h header.  For composite gadgets those macros
 * are redundant since composite.h is included any way.
 *
 * One could check whether those macros are already defined (which
 * would indicate composite.h had been included) or not (which would
 * indicate we were in FSG) but this is not done because a warning is
 * desired if definitions here differ from the ones in composite.h.
 *
 * We want the definitions to match and be the same in File Storage
 * Gadget as well as Mass Storage Function (and so composite gadgets
 * using MSF).  If someone changes them in composite.h it will produce
 * a warning in this file when building MSF.
 */

#define DBG(d, fmt, args...) \
	dev_dbg(&(d)->gadget->dev , fmt , ## args)
#define VDBG(d, fmt, args...) \
	dev_vdbg(&(d)->gadget->dev , fmt , ## args)
#define ERROR(d, fmt, args...) \
	dev_err(&(d)->gadget->dev , fmt , ## args)
#define WARNING(d, fmt, args...) \
	dev_warn(&(d)->gadget->dev , fmt , ## args)
#define INFO(d, fmt, args...) \
	dev_info(&(d)->gadget->dev , fmt , ## args)

#ifdef DUMP_MSGS

/* dump_msg(fsg, const char * label, const u8 * buf, unsigned length); */
# define dump_msg(fsg, label, buf, length) do {                         \
	if (length < 512) {						\
		DBG(fsg, "%s, length %u:\n", label, length);		\
		print_hex_dump("", DUMP_PREFIX_OFFSET,	\
			       16, 1, buf, length, 0);			\
	}								\
} while (0)

#  define dump_cdb(fsg) do { } while (0)

#else

#  define dump_msg(fsg, /* const char * */ label, \
		   /* const u8 * */ buf, /* unsigned */ length) do { } while (0)

#  ifdef VERBOSE_DEBUG

#    define dump_cdb(fsg)						\
	print_hex_dump("SCSI CDB: ", DUMP_PREFIX_NONE,	\
		       16, 1, (fsg)->cmnd, (fsg)->cmnd_size, 0)		\

#  else

#    define dump_cdb(fsg) do { } while (0)

#  endif /* VERBOSE_DEBUG */

#endif /* DUMP_MSGS */

/*
 * Thanks to NetChip Technologies for donating this product ID.
 *
 * DO NOT REUSE THESE IDs with any other driver!!  Ever!!
 * Instead:  allocate your own, using normal USB-IF procedures.
 */

#define FSG_VENDOR_ID	0x0525	/* NetChip */
#define FSG_PRODUCT_ID	0xa4a5	/* Linux-USB File-backed Storage Gadget */

/* Length of a SCSI Command Data Block */
#define MAX_COMMAND_SIZE	16

/* SCSI Sense Key/Additional Sense Code/ASC Qualifier values */
#define SS_NO_SENSE				0
#define SS_COMMUNICATION_FAILURE		0x040800
#define SS_INVALID_COMMAND			0x052000
#define SS_INVALID_FIELD_IN_CDB			0x052400
#define SS_LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE	0x052100
#define SS_LOGICAL_UNIT_NOT_SUPPORTED		0x052500
#define SS_MEDIUM_NOT_PRESENT			0x023a00
#define SS_MEDIUM_REMOVAL_PREVENTED		0x055302
#define SS_NOT_READY_TO_READY_TRANSITION	0x062800
#define SS_RESET_OCCURRED			0x062900
#define SS_SAVING_PARAMETERS_NOT_SUPPORTED	0x053900
#define SS_UNRECOVERED_READ_ERROR		0x031100
#define SS_WRITE_ERROR				0x030c02
#define SS_WRITE_PROTECTED			0x072700

#define SK(x)		((u8) ((x) >> 16))	/* Sense Key byte, etc. */
#define ASC(x)		((u8) ((x) >> 8))
#define ASCQ(x)		((u8) (x))

/*-------------------------------------------------------------------------*/

struct fsg_lun {
	loff_t		file_length;
	loff_t		num_sectors;

	unsigned int	initially_ro:1;
	unsigned int	ro:1;
	unsigned int	removable:1;
	unsigned int	cdrom:1;
	unsigned int	prevent_medium_removal:1;
	unsigned int	registered:1;
	unsigned int	info_valid:1;
	unsigned int	nofua:1;

	u32		sense_data;
	u32		sense_data_info;
	u32		unit_attention_data;

	struct device_d	dev;
};

#define fsg_lun_is_open(curlun)	((curlun)->filp != NULL)

/* Big enough to hold our biggest descriptor */
#define EP0_BUFSIZE	256
#define DELAYED_STATUS	(EP0_BUFSIZE + 999)	/* An impossibly large value */

/* Number of buffers we will use.  2 is enough for double-buffering */
#define FSG_NUM_BUFFERS	2

/* Default size of buffer length. */
#define FSG_BUFLEN	((u32)131072)

/* Maximal number of LUNs supported in mass storage function */
#define FSG_MAX_LUNS	8

enum fsg_buffer_state {
	BUF_STATE_EMPTY = 0,
	BUF_STATE_FULL,
	BUF_STATE_BUSY
};

/*
 * When FSG_BUFFHD_STATIC_BUFFER is defined when this file is included
 * the fsg_buffhd structure's buf field will be an array of FSG_BUFLEN
 * characters rather then a pointer to void.
 */

struct fsg_buffhd {
	void				*buf;
	enum fsg_buffer_state		state;
	struct fsg_buffhd		*next;

	/*
	 * The NetChip 2280 is faster, and handles some protocol faults
	 * better, if we don't submit any short bulk-out read requests.
	 * So we will record the intended request length here.
	 */
	unsigned int			bulk_out_intended_length;

	struct usb_request		*inreq;
	int				inreq_busy;
	struct usb_request		*outreq;
	int				outreq_busy;
};

enum fsg_state {
	/* This one isn't used anywhere */
	FSG_STATE_COMMAND_PHASE = -10,
	FSG_STATE_DATA_PHASE,
	FSG_STATE_STATUS_PHASE,

	FSG_STATE_IDLE = 0,
	FSG_STATE_ABORT_BULK_OUT,
	FSG_STATE_RESET,
	FSG_STATE_INTERFACE_CHANGE,
	FSG_STATE_CONFIG_CHANGE,
	FSG_STATE_DISCONNECT,
	FSG_STATE_EXIT,
	FSG_STATE_TERMINATED
};

enum data_direction {
	DATA_DIR_UNKNOWN = 0,
	DATA_DIR_FROM_HOST,
	DATA_DIR_TO_HOST,
	DATA_DIR_NONE
};

/*-------------------------------------------------------------------------*/

static inline u32 get_unaligned_be24(u8 *buf)
{
	return 0xffffff & (u32) get_unaligned_be32(buf - 1);
}

/*-------------------------------------------------------------------------*/

enum {
	FSG_STRING_INTERFACE
};

/*-------------------------------------------------------------------------*/

extern struct usb_interface_descriptor fsg_intf_desc;

extern struct usb_endpoint_descriptor fsg_fs_bulk_in_desc;
extern struct usb_endpoint_descriptor fsg_fs_bulk_out_desc;
extern struct usb_descriptor_header *fsg_fs_function[];

extern struct usb_endpoint_descriptor fsg_hs_bulk_in_desc;
extern struct usb_endpoint_descriptor fsg_hs_bulk_out_desc;
extern struct usb_descriptor_header *fsg_hs_function[];

int fsg_lun_open(struct fsg_lun *curlun, unsigned int num_sectors,
		 const char *filename);
void fsg_lun_close(struct fsg_lun *curlun);

struct usb_endpoint_descriptor *
fsg_ep_desc(struct usb_gadget *g, struct usb_endpoint_descriptor *fs,
		struct usb_endpoint_descriptor *hs);
int fsg_lun_fsync_sub(struct fsg_lun *curlun);
void store_cdrom_address(u8 *dest, int msf, u32 addr);

#endif /* USB_STORAGE_COMMON_H */
