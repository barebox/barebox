#include <common.h>
#include <stdio.h>
#include "cfi_flash.h"

/*-----------------------------------------------------------------------
 * Reverse the order of the erase regions in the CFI QRY structure.
 * This is needed for chips that are either a) correctly detected as
 * top-boot, or b) buggy.
 */
static void cfi_reverse_geometry(struct cfi_qry *qry)
{
	unsigned int i, j;
	u32 tmp;

	for (i = 0, j = qry->num_erase_regions - 1; i < j; i++, j--) {
		tmp = qry->erase_region_info[i];
		qry->erase_region_info[i] = qry->erase_region_info[j];
		qry->erase_region_info[j] = tmp;
	}
}

static void flash_unlock_seq (struct flash_info *info)
{
	flash_write_cmd (info, 0, info->addr_unlock1, AMD_CMD_UNLOCK_START);
	flash_write_cmd (info, 0, info->addr_unlock2, AMD_CMD_UNLOCK_ACK);
}

/*
 * read jedec ids from device and set corresponding fields in info struct
 *
 * Note: assume cfi->vendor, cfi->portwidth and cfi->chipwidth are correct
 *
*/
static void amd_read_jedec_ids (struct flash_info *info)
{
	info->cmd_reset		= AMD_CMD_RESET;
	info->manufacturer_id = 0;
	info->device_id       = 0;
	info->device_id2      = 0;

	/* calculate command offsets as in the Linux driver */
	info->addr_unlock1 = 0x555;
	info->addr_unlock2 = 0x2AA;

	/*
	 * modify the unlock address if we are in compatibility mode
	 */
	if (	/* x8/x16 in x8 mode */
		((info->chipwidth == FLASH_CFI_BY8) &&
			(info->interface == FLASH_CFI_X8X16)) ||
		/* x16/x32 in x16 mode */
		((info->chipwidth == FLASH_CFI_BY16) &&
			(info->interface == FLASH_CFI_X16X32)))
	{
		info->addr_unlock1 = 0xaaa;
		info->addr_unlock2 = 0x555;
	}

	flash_write_cmd(info, 0, 0, info->cmd_reset);
	flash_unlock_seq(info);
	flash_write_cmd(info, 0, info->addr_unlock1, FLASH_CMD_READ_ID);
	udelay(1000); /* some flash are slow to respond */

	info->manufacturer_id = jedec_read_mfr(info);
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
	flash_write_cmd(info, 0, 0, info->cmd_reset);
}

static int flash_toggle (struct flash_info *info, flash_sect_t sect, uint offset, uchar cmd)
{
	void *addr;
	cfiword_t cword;
	int retval;

	addr = flash_make_addr (info, sect, offset);
	flash_make_cmd (info, cmd, &cword);
	if (bankwidth_is_1(info)) {
		retval = flash_read8(addr) != flash_read8(addr);
	} else if (bankwidth_is_2(info)) {
		retval = flash_read16(addr) != flash_read16(addr);
	} else if (bankwidth_is_4(info)) {
		retval = flash_read32(addr) != flash_read32(addr);
	} else if (bankwidth_is_8(info)) {
		retval = ( (flash_read32( addr ) != flash_read32( addr )) ||
			   (flash_read32(addr+4) != flash_read32(addr+4)) );
	} else
		retval = 0;

	return retval;
}

/*
 * flash_is_busy - check to see if the flash is busy
 * This routine checks the status of the chip and returns true if the chip is busy
 */
static int amd_flash_is_busy (struct flash_info *info, flash_sect_t sect)
{
	return flash_toggle (info, sect, 0, AMD_STATUS_TOGGLE);
}

static int amd_flash_erase_one (struct flash_info *info, long sect)
{
	flash_unlock_seq(info);
	flash_write_cmd (info, 0, info->addr_unlock1, AMD_CMD_ERASE_START);
	flash_unlock_seq(info);
	flash_write_cmd (info, sect, 0, AMD_CMD_ERASE_SECTOR);

	return flash_status_check(info, sect, info->erase_blk_tout, "erase");
}

static void amd_flash_prepare_write(struct flash_info *info)
{
	flash_unlock_seq(info);
	flash_write_cmd (info, 0, info->addr_unlock1, AMD_CMD_WRITE);
}

#ifdef CONFIG_CFI_BUFFER_WRITE
static int amd_flash_write_cfibuffer (struct flash_info *info, ulong dest, const uchar * cp,
				  int len)
{
	flash_sect_t sector;
	int cnt;
	int retcode;
	void *src = (void*)cp;
	void *dst = (void *)dest;
	cfiword_t cword;

	sector = find_sector (info, dest);

	flash_unlock_seq(info);
	flash_make_cmd (info, AMD_CMD_WRITE_TO_BUFFER, &cword);
	flash_write_word(info, cword, (void *)dest);

	if (bankwidth_is_1(info)) {
		cnt = len;
		flash_write_cmd(info, sector, 0, (u32)cnt - 1);
		while (cnt-- > 0) {
			flash_write8(flash_read8(src), dst);
			src += 1, dst += 1;
		}
	} else if (bankwidth_is_2(info)) {
		cnt = len >> 1;
		flash_write_cmd(info, sector, 0, (u32)cnt - 1);
		while (cnt-- > 0) {
			flash_write16(flash_read16(src), dst);
			src += 2, dst += 2;
		}
	} else if (bankwidth_is_4(info)) {
		cnt = len >> 2;
		flash_write_cmd(info, sector, 0, (u32)cnt - 1);
		while (cnt-- > 0) {
			flash_write32(flash_read32(src), dst);
			src += 4, dst += 4;
		}
	} else if (bankwidth_is_8(info)) {
		cnt = len >> 3;
		flash_write_cmd(info, sector, 0, (u32)cnt - 1);
		while (cnt-- > 0) {
			flash_write64(flash_read64(src), dst);
			src += 8, dst += 8;
		}
	}

	flash_write_cmd (info, sector, 0, AMD_CMD_WRITE_BUFFER_CONFIRM);
	retcode = flash_status_check (info, sector, info->buffer_write_tout,
					   "buffer write");
	return retcode;
}
#else
#define amd_flash_write_cfibuffer NULL
#endif /* CONFIG_CFI_BUFFER_WRITE */

static int amd_flash_real_protect (struct flash_info *info, long sector, int prot)
{
	if (info->manufacturer_id != (uchar)ATM_MANUFACT)
		return 0;

	if (prot) {
		flash_unlock_seq (info);
		flash_write_cmd (info, 0, info->addr_unlock1,
				 ATM_CMD_SOFTLOCK_START);
		flash_unlock_seq (info);
		flash_write_cmd (info, sector, 0, ATM_CMD_LOCK_SECT);
	} else {
		flash_write_cmd (info, 0, info->addr_unlock1,
				 AMD_CMD_UNLOCK_START);
		if (info->device_id == ATM_ID_BV6416)
			flash_write_cmd (info, sector, 0,
					 ATM_CMD_UNLOCK_SECT);
	}

	return 0;
}

/*
 * Manufacturer-specific quirks. Add workarounds for geometry
 * reversal, etc. here.
 */
static void flash_fixup_amd (struct flash_info *info, struct cfi_qry *qry)
{
	/* check if flash geometry needs reversal */
	if (qry->num_erase_regions > 1) {
		/* reverse geometry if top boot part */
		if (info->cfi_version < 0x3131) {
			/* CFI < 1.1, try to guess from device id */
			if ((info->device_id & 0x80) != 0)
				cfi_reverse_geometry(qry);
		} else if (flash_read_uchar(info, info->ext_addr + 0xf) == 3) {
			/* CFI >= 1.1, deduct from top/bottom flag */
			/* note: ext_addr is valid since cfi_version > 0 */
			cfi_reverse_geometry(qry);
		}
	}
}

static void flash_fixup_atmel(struct flash_info *info, struct cfi_qry *qry)
{
	int reverse_geometry = 0;

	/* Check the "top boot" bit in the PRI */
	if (info->ext_addr && !(flash_read_uchar(info, info->ext_addr + 6) & 1))
		reverse_geometry = 1;

	/* AT49BV6416(T) list the erase regions in the wrong order.
	 * However, the device ID is identical with the non-broken
	 * AT49BV642D since u-boot only reads the low byte (they
	 * differ in the high byte.) So leave out this fixup for now.
	 */
	if (info->device_id == 0xd6 || info->device_id == 0xd2)
		reverse_geometry = !reverse_geometry;

	if (reverse_geometry)
		cfi_reverse_geometry(qry);
}

static void amd_flash_fixup(struct flash_info *info, struct cfi_qry *qry)
{
	/* Do manufacturer-specific fixups */
	switch (info->manufacturer_id) {
	case 0x0001:
		flash_fixup_amd(info, qry);
		break;
	case 0x001f:
		flash_fixup_atmel(info, qry);
		break;
        }
}

struct cfi_cmd_set cfi_cmd_set_amd = {
	.flash_write_cfibuffer = amd_flash_write_cfibuffer,
	.flash_erase_one = amd_flash_erase_one,
	.flash_is_busy = amd_flash_is_busy,
	.flash_read_jedec_ids = amd_read_jedec_ids,
	.flash_prepare_write = amd_flash_prepare_write,
	.flash_status_check = flash_generic_status_check,
	.flash_real_protect = amd_flash_real_protect,
	.flash_fixup = amd_flash_fixup,
};

