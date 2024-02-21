// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2014-2023 TQ-Systems GmbH <u-boot@ew.tq-group.com>,
 * D-82229 Seefeld, Germany.
 * Author: Markus Niebel
 */

#include <common.h>
#include <net.h>
#include <linux/ctype.h>
#include <crc.h>
#include <pbl/i2c.h>
#include <pbl/eeprom.h>
#include <boards/tq/tq_eeprom.h>

/*
 * static EEPROM layout
 */
#define TQ_EE_HRCW_BYTES		0x20
#define TQ_EE_RSV1_BYTES		10
#define TQ_EE_RSV2_BYTES		8

struct __packed tq_eeprom_data {
	union {
		struct tq_vard vard;
		_Static_assert(sizeof(struct tq_vard) == TQ_EE_HRCW_BYTES, \
			"struct tq_vard has incorrect size");
		u8 hrcw_primary[TQ_EE_HRCW_BYTES];
	} tq_hw_data;
	u8 mac[TQ_EE_MAC_BYTES];		/* 0x20 ... 0x25 */
	u8 rsv1[TQ_EE_RSV1_BYTES];
	u8 serial[TQ_EE_SERIAL_BYTES];		/* 0x30 ... 0x37 */
	u8 rsv2[TQ_EE_RSV2_BYTES];
	u8 id[TQ_EE_BDID_BYTES];		/* 0x40 ... 0x7f */
};

static bool tq_vard_valid(const struct tq_vard *vard)
{
	const unsigned char *start = (const unsigned char *)(vard) +
		sizeof(vard->crc);
	u16 crc;

	crc = crc_itu_t(0, start, sizeof(*vard) - sizeof(vard->crc));

	return vard->crc == crc;
}

phys_size_t tq_vard_memsize(u8 val, unsigned int multiply, unsigned int tmask)
{
	phys_size_t result = 0;

	if (val != VARD_MEMSIZE_DEFAULT) {
		result = 1 << (size_t)(val & VARD_MEMSIZE_MASK_EXP);
		if (val & tmask)
			result *= 3;
		result *= multiply;
	}

	return result;
}

void tq_vard_show(const struct tq_vard *vard)
{
	/* display data anyway to support developer */
	printf("HW\tREV.%02uxx\n", (unsigned int)vard->hwrev);
	printf("RAM\ttype %u, %lu MiB, %s\n",
	       (unsigned int)(vard->memtype & VARD_MEMTYPE_MASK_TYPE),
	       (unsigned long)(tq_vard_ramsize(vard) / (SZ_1M)),
	       (tq_vard_has_ramecc(vard) ? "ECC" : "no ECC"));
	printf("RTC\t%c\nSPINOR\t%c\ne-MMC\t%c\nSE\t%c\nEEPROM\t%c\n",
	       (tq_vard_has_rtc(vard) ? 'y' : 'n'),
	       (tq_vard_has_spinor(vard) ? 'y' : 'n'),
	       (tq_vard_has_emmc(vard) ? 'y' : 'n'),
	       (tq_vard_has_secelem(vard) ? 'y' : 'n'),
	       (tq_vard_has_eeprom(vard) ? 'y' : 'n'));

	if (tq_vard_has_eeprom(vard))
		printf("EEPROM\ttype %u, %lu KiB, page %zu\n",
		       (unsigned int)(vard->eepromtype & VARD_EETYPE_MASK_MFR) >> 4,
		       (unsigned long)(tq_vard_eepromsize(vard) / (SZ_1K)),
		       tq_vard_eeprom_pgsize(vard));

	printf("FORMFACTOR: ");

	switch (tq_vard_get_formfactor(vard)) {
	case VARD_FORMFACTOR_TYPE_LGA:
		printf("LGA\n");
		break;
	case VARD_FORMFACTOR_TYPE_CONNECTOR:
		printf("CONNECTOR\n");
		break;
	case VARD_FORMFACTOR_TYPE_SMARC2:
		printf("SMARC-2\n");
		break;
	case VARD_FORMFACTOR_TYPE_NONE:
		/*
		 * applies to boards with no variants or older boards
		 * where this field is not written
		 */
		printf("UNSPECIFIED\n");
		break;
	default:
		/*
		 * generic fall trough
		 * unhandled form factor or invalid data
		 */
		printf("UNKNOWN\n");
		break;
	}
}

static void tq_read_string(const char *src, char *dst, int len)
{
	int i;

	for (i = 0; i < len && isprint(src[i]) && isascii(src[i]); ++i)
		dst[i] = src[i];
	dst[i] = '\0';
}

struct tq_eeprom *pbl_tq_read_eeprom(struct pbl_i2c *i2c, u8 addr)
{
	struct tq_eeprom_data raw;
	static struct tq_eeprom eeprom;
	int ret;

	ret = eeprom_read(i2c, addr, 0, &raw, sizeof(raw));
	if (ret)
		return NULL;

	if (tq_vard_valid(&raw.tq_hw_data.vard))
		eeprom.vard = raw.tq_hw_data.vard;

	memcpy(eeprom.mac, raw.mac, TQ_EE_MAC_BYTES);
	tq_read_string(raw.serial, eeprom.serial, TQ_EE_SERIAL_BYTES);
	tq_read_string(raw.id, eeprom.id, TQ_EE_BDID_BYTES);

	return &eeprom;
}
