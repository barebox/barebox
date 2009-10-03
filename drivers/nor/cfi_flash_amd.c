#include <common.h>
#include <stdio.h>
#include <cfi_flash_new.h>

static void flash_unlock_seq (flash_info_t * info, flash_sect_t sect)
{
	flash_write_cmd (info, sect, AMD_ADDR_START, AMD_CMD_UNLOCK_START);
	flash_write_cmd (info, sect, AMD_ADDR_ACK, AMD_CMD_UNLOCK_ACK);
}

/*
 * read jedec ids from device and set corresponding fields in info struct
 *
 * Note: assume cfi->vendor, cfi->portwidth and cfi->chipwidth are correct
 *
*/
static void amd_read_jedec_ids (flash_info_t * info)
{
	info->manufacturer_id = 0;
	info->device_id       = 0;
	info->device_id2      = 0;

	flash_write_cmd(info, 0, 0, AMD_CMD_RESET);
	flash_unlock_seq(info, 0);
	flash_write_cmd(info, 0, AMD_ADDR_START, FLASH_CMD_READ_ID);
	udelay(1000); /* some flash are slow to respond */
	info->manufacturer_id = flash_read_uchar (info,
					FLASH_OFFSET_MANUFACTURER_ID);
	info->device_id = flash_read_uchar (info,
					FLASH_OFFSET_DEVICE_ID);
	if (info->device_id == 0x7E) {
		/* AMD 3-byte (expanded) device ids */
		info->device_id2 = flash_read_uchar (info,
					FLASH_OFFSET_DEVICE_ID2);
		info->device_id2 <<= 8;
		info->device_id2 |= flash_read_uchar (info,
					FLASH_OFFSET_DEVICE_ID3);
	}
	flash_write_cmd(info, 0, 0, AMD_CMD_RESET);
}

static int flash_toggle (flash_info_t * info, flash_sect_t sect, uint offset, uchar cmd)
{
	cfiptr_t cptr;
	cfiword_t cword;
	int retval;

	cptr.cp = flash_make_addr (info, sect, offset);
	flash_make_cmd (info, cmd, &cword);
	if (bankwidth_is_1(info)) {
		retval = ((cptr.cp[0] & cword.c) != (cptr.cp[0] & cword.c));
	} else if (bankwidth_is_2(info)) {
		retval = ((cptr.wp[0] & cword.w) != (cptr.wp[0] & cword.w));
	} else if (bankwidth_is_4(info)) {
		retval = ((cptr.lp[0] & cword.l) != (cptr.lp[0] & cword.l));
	} else if (bankwidth_is_8(info)) {
		retval = ((cptr.llp[0] & cword.ll) !=
			  (cptr.llp[0] & cword.ll));
	} else
		retval = 0;

	return retval;
}

/*
 * flash_is_busy - check to see if the flash is busy
 * This routine checks the status of the chip and returns true if the chip is busy
 */
static int amd_flash_is_busy (flash_info_t * info, flash_sect_t sect)
{
	return flash_toggle (info, sect, 0, AMD_STATUS_TOGGLE);
}

static int amd_flash_erase_one (flash_info_t * info, long sect)
{
	int rcode = 0;

	flash_unlock_seq (info, sect);
	flash_write_cmd (info, sect, AMD_ADDR_ERASE_START,
				AMD_CMD_ERASE_START);
	flash_unlock_seq (info, sect);
	flash_write_cmd (info, sect, 0, AMD_CMD_ERASE_SECTOR);

	if (flash_status_check
	    (info, sect, info->erase_blk_tout, "erase")) {
		rcode = 1;
	} else
		putchar('.');
	return rcode;
}

static void amd_flash_prepare_write(flash_info_t * info)
{
	flash_unlock_seq (info, 0);
	flash_write_cmd (info, 0, AMD_ADDR_START, AMD_CMD_WRITE);
}

#ifdef CONFIG_CFI_BUFFER_WRITE
static int amd_flash_write_cfibuffer (flash_info_t * info, ulong dest, const uchar * cp,
				  int len)
{
	flash_sect_t sector;
	int cnt;
	int retcode;
	volatile cfiptr_t src;
	volatile cfiptr_t dst;
	cfiword_t cword;

	src.cp = (uchar *)cp;
	dst.cp = (uchar *) dest;
	sector = find_sector (info, dest);

	flash_unlock_seq(info,0);
	flash_make_cmd (info, AMD_CMD_WRITE_TO_BUFFER, &cword);
	flash_write_word(info, cword, (void *)dest);

	if (bankwidth_is_1(info)) {
		cnt = len;
		flash_write_cmd (info, sector, 0,  (uchar) cnt - 1);
		while (cnt-- > 0) *dst.cp++ = *src.cp++;
	} else if (bankwidth_is_2(info)) {
		cnt = len >> 1;
		flash_write_cmd (info, sector, 0,  (uchar) cnt - 1);
		while (cnt-- > 0) *dst.wp++ = *src.wp++;
	} else if (bankwidth_is_4(info)) {
		cnt = len >> 2;
		flash_write_cmd (info, sector, 0,  (uchar) cnt - 1);
		while (cnt-- > 0) *dst.lp++ = *src.lp++;
	} else if (bankwidth_is_8(info)) {
		cnt = len >> 3;
		flash_write_cmd (info, sector, 0,  (uchar) cnt - 1);
		while (cnt-- > 0) *dst.llp++ = *src.llp++;
	}

	flash_write_cmd (info, sector, 0, AMD_CMD_WRITE_BUFFER_CONFIRM);
	retcode = flash_status_check (info, sector, info->buffer_write_tout,
					   "buffer write");
	return retcode;
}
#else
#define amd_flash_write_cfibuffer NULL
#endif /* CONFIG_CFI_BUFFER_WRITE */

struct cfi_cmd_set cfi_cmd_set_amd = {
	.flash_write_cfibuffer = amd_flash_write_cfibuffer,
	.flash_erase_one = amd_flash_erase_one,
	.flash_is_busy = amd_flash_is_busy,
	.flash_read_jedec_ids = amd_read_jedec_ids,
	.flash_prepare_write = amd_flash_prepare_write,
	.flash_status_check = flash_generic_status_check,
};

