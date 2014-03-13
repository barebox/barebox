/*
 * Copyright 2008 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#include <common.h>
#include <crc.h>
#include <ddr_spd.h>

uint32_t ddr2_spd_checksum_pass(const struct ddr2_spd_eeprom_s *spd)
{
	uint32_t i, cksum = 0;
	const uint8_t *buf = (const uint8_t *)spd;
	uint8_t rev, spd_cksum;

	rev = spd->spd_rev;
	spd_cksum = spd->cksum;

	/* Rev 1.X or less supported by this code */
	if (rev >= 0x20)
		goto error;

	/*
	 * The checksum is calculated on the first 64 bytes
	 * of the SPD as per JEDEC specification.
	 */
	for (i = 0; i < 63; i++)
		cksum += *buf++;
	cksum &= 0xFF;

	if (cksum != spd_cksum)
		goto error;

	return 0;
error:
	return 1;
}

uint32_t ddr3_spd_checksum_pass(const struct ddr3_spd_eeprom_s *spd)
{
	char crc_lsb, crc_msb;
	int csum16, len;

	/*
	 * SPD byte0[7] - CRC coverage
	 * 0 = CRC covers bytes 0~125
	 * 1 = CRC covers bytes 0~116
	 */

	len = !(spd->info_size_crc & 0x80) ? 126 : 117;
	csum16 = cyg_crc16((char *)spd, len);

	crc_lsb = (char) (csum16 & 0xff);
	crc_msb = (char) (csum16 >> 8);

	if (spd->crc[0] != crc_lsb || spd->crc[1] != crc_msb)
		return 1;

	return 0;
}
