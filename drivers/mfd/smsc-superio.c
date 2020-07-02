// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
 */

#define pr_fmt(fmt) "smsc-superio: " fmt

#include <superio.h>
#include <init.h>
#include <asm/io.h>
#include <common.h>

#define SIO_UNLOCK_KEY		0x55	/* Key to enable Super-I/O */
#define SIO_LOCK_KEY		0xAA	/* Key to disable Super-I/O */

#define SMSC_ID			0x10b8  /* Standard Microsystems Corp PCI ID */

static void superio_enter(u16 sioaddr)
{
	outb(SIO_UNLOCK_KEY, sioaddr);
	mdelay(1);
	outb(SIO_UNLOCK_KEY, sioaddr);
}

static void superio_exit(u16 sioaddr)
{
	outb(SIO_LOCK_KEY, sioaddr);
}

static void smsc_superio_find(u16 sioaddr, u16 id_reg)
{
	struct superio_chip *chip;
	u16 devid;

	superio_enter(sioaddr);

	devid = superio_inw(sioaddr, id_reg);
	switch(devid >> 8) {
	case 0x02:
	case 0x03:
	case 0x07:
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x0e:
	case 0x14:
	case 0x30:
	case 0x40:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x46:
	case 0x47:
	case 0x4c:
	case 0x4d:
	case 0x51:
	case 0x52:
	case 0x54:
	case 0x56:
	case 0x57:
	case 0x59:
	case 0x5d:
	case 0x5f:
	case 0x60:
	case 0x62:
	case 0x67:
	case 0x6b:
	case 0x6e:
	case 0x6f:
	case 0x74:
	case 0x76:
	case 0x77:
	case 0x78:
	case 0x79:
	case 0x7a:
	case 0x7c:
	case 0x7d:
	case 0x7f:
	case 0x81:
	case 0x83:
	case 0x85:
	case 0x86:
	case 0x89:
	case 0x8c:
	case 0x90:
		break;
	default:
		pr_debug("Not a SMSC device (port=0x%02x, devid=0x%04x)\n",
			 sioaddr, devid);
		return;
	}

	chip = xzalloc(sizeof(*chip));

	chip->devid = devid;
	chip->vid = SMSC_ID;
	chip->sioaddr = sioaddr;
	chip->enter = superio_enter;
	chip->exit = superio_exit;

	superio_chip_add(chip);

	superio_exit(sioaddr);
}

static int smsc_superio_detect(void)
{
	u16 ports[] = { 0x2e, 0x4e, 0x162e, 0x164e, 0x3f0, 0x370 };
	int i;

	for (i = 0; i < ARRAY_SIZE(ports); i++)
		smsc_superio_find(ports[i], SIO_REG_DEVID);

	return 0;
}
coredevice_initcall(smsc_superio_detect);
