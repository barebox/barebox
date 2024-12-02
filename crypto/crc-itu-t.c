/*
 *      crc-itu-t.c
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2. See the file COPYING for more details.
 */

#include <crc.h>

static u16 crc_itu_t_table[256];

/*
 * Implements the standard CRC ITU-T V.41:
 *   Width 16
 *   Poly  0x1021 (x^16 + x^12 + x^15 + 1)
 *   Init  0
 */
static u16 crc_itu_t_byte(u16 crc, const u8 data)
{
	int i;

	crc = crc ^ data << 8;
	for (i = 0; i < 8; ++i) {
		if (crc & 0x8000)
			crc = crc << 1 ^ 0x1021;
		else
			crc = crc << 1;
	}
	return crc;
}

static void make_crc_table(void)
{
	if (crc_itu_t_table[1])
		return;

	for (int n = 0; n < 256; n++)
		crc_itu_t_table[n] = crc_itu_t_byte(0, n);
}

/**
 * crc_itu_t - Compute the CRC-ITU-T for the data buffer
 *
 * @crc:     previous CRC value
 * @buffer:  data pointer
 * @len:     number of bytes in the buffer
 *
 * Returns the updated CRC value
 */
u16 crc_itu_t(u16 crc, const u8 *buffer, size_t len)
{
	make_crc_table();

	while (len--)
		crc = (crc << 8) ^ crc_itu_t_table[((crc >> 8) ^ *buffer++) & 0xff];
	return crc;
}
