#include <common.h>
#include "cfi_flash.h"

/*
 * read jedec ids from device and set corresponding fields in info struct
 *
 * Note: assume cfi->vendor, cfi->portwidth and cfi->chipwidth are correct
 *
*/
static void intel_read_jedec_ids (struct flash_info *info)
{
	info->manufacturer_id = 0;
	info->device_id       = 0;
	info->device_id2      = 0;

	flash_write_cmd(info, 0, 0, FLASH_CMD_RESET);
	flash_write_cmd(info, 0, 0, FLASH_CMD_READ_ID);
	udelay(1000); /* some flash are slow to respond */

	info->manufacturer_id = jedec_read_mfr(info);
	info->device_id = flash_read_uchar (info,
					FLASH_OFFSET_DEVICE_ID);
	flash_write_cmd(info, 0, 0, FLASH_CMD_RESET);
}

/*
 * flash_is_busy - check to see if the flash is busy
 * This routine checks the status of the chip and returns true if the chip is busy
 */
static int intel_flash_is_busy (struct flash_info *info, flash_sect_t sect)
{
	return !flash_isset (info, sect, 0, FLASH_STATUS_DONE);
}

static int intel_flash_erase_one (struct flash_info *info, long sect)
{
	flash_write_cmd (info, sect, 0, FLASH_CMD_CLEAR_STATUS);
	flash_write_cmd (info, sect, 0, FLASH_CMD_BLOCK_ERASE);
	flash_write_cmd (info, sect, 0, FLASH_CMD_ERASE_CONFIRM);

	return flash_status_check(info, sect, info->erase_blk_tout, "erase");
}

static void intel_flash_prepare_write(struct flash_info *info)
{
	flash_write_cmd (info, 0, 0, FLASH_CMD_CLEAR_STATUS);
	flash_write_cmd (info, 0, 0, FLASH_CMD_WRITE);
}

#ifdef CONFIG_CFI_BUFFER_WRITE
static int intel_flash_write_cfibuffer (struct flash_info *info, ulong dest, const uchar * cp,
				  int len)
{
	flash_sect_t sector;
	int cnt;
	int retcode;
	volatile cfiptr_t src;
	volatile cfiptr_t dst;

	src.cp = (uchar *)cp;
	dst.cp = (uchar *) dest;
	sector = find_sector (info, dest);
	flash_write_cmd (info, sector, 0, FLASH_CMD_CLEAR_STATUS);
	flash_write_cmd (info, sector, 0, FLASH_CMD_WRITE_TO_BUFFER);

	retcode = flash_generic_status_check (info, sector, info->buffer_write_tout,
					   "write to buffer");
	if (retcode != ERR_OK)
		return retcode;

	/* reduce the number of loops by the width of the port	*/
	cnt = len >> (info->portwidth - 1);

	flash_write_cmd (info, sector, 0, (uchar) cnt - 1);
	while (cnt-- > 0) {
		if (bankwidth_is_1(info)) {
			*dst.cp++ = *src.cp++;
		} else if (bankwidth_is_2(info)) {
			*dst.wp++ = *src.wp++;
		} else if (bankwidth_is_4(info)) {
			*dst.lp++ = *src.lp++;
		} else if (bankwidth_is_8(info)) {
			*dst.llp++ = *src.llp++;
		}
	}

	flash_write_cmd (info, sector, 0, FLASH_CMD_WRITE_BUFFER_CONFIRM);
	retcode = flash_status_check (info, sector,
					   info->buffer_write_tout,
					   "buffer write");
	return retcode;
}
#else
#define intel_flash_write_cfibuffer NULL
#endif /* CONFIG_CFI_BUFFER_WRITE */

static int intel_flash_status_check (struct flash_info *info, flash_sect_t sector,
				    uint64_t tout, char *prompt)
{
	int retcode;

	retcode = flash_generic_status_check (info, sector, tout, prompt);

	if ((retcode == ERR_OK)
	    && !flash_isequal (info, sector, 0, FLASH_STATUS_DONE)) {
		retcode = ERR_INVAL;
		printf ("Flash %s error at address %lx\n", prompt,
			info->start[sector]);
		if (flash_isset (info, sector, 0, FLASH_STATUS_ECLBS | FLASH_STATUS_PSLBS)) {
			puts ("Command Sequence Error.\n");
		} else if (flash_isset (info, sector, 0, FLASH_STATUS_ECLBS)) {
			puts ("Block Erase Error.\n");
			retcode = ERR_NOT_ERASED;
		} else if (flash_isset (info, sector, 0, FLASH_STATUS_PSLBS)) {
			puts ("Locking Error\n");
		}
		if (flash_isset (info, sector, 0, FLASH_STATUS_DPS)) {
			puts ("Block locked.\n");
			retcode = ERR_PROTECTED;
		}
		if (flash_isset (info, sector, 0, FLASH_STATUS_VPENS))
			puts ("Vpp Low Error.\n");
	}
	flash_write_cmd (info, sector, 0, info->cmd_reset);

	return retcode;
}

static int intel_flash_real_protect (struct flash_info *info, long sector, int prot)
{
	flash_write_cmd (info, sector, 0, FLASH_CMD_CLEAR_STATUS);
	flash_write_cmd (info, sector, 0, FLASH_CMD_PROTECT);
	if (prot)
		flash_write_cmd (info, sector, 0, FLASH_CMD_PROTECT_SET);
	else
		flash_write_cmd (info, sector, 0, FLASH_CMD_PROTECT_CLEAR);

	return 0;
}

struct cfi_cmd_set cfi_cmd_set_intel = {
	.flash_write_cfibuffer = intel_flash_write_cfibuffer,
	.flash_erase_one = intel_flash_erase_one,
	.flash_is_busy = intel_flash_is_busy,
	.flash_read_jedec_ids = intel_read_jedec_ids,
	.flash_prepare_write = intel_flash_prepare_write,
	.flash_status_check = intel_flash_status_check,
	.flash_real_protect = intel_flash_real_protect,
};

