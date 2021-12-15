// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
 */

#define pr_fmt(fmt) "fintek-superio: " fmt

#include <superio.h>
#include <init.h>
#include <asm/io.h>
#include <common.h>

#define SIO_UNLOCK_KEY		0x87	/* Key to enable Super-I/O */
#define SIO_LOCK_KEY		0xAA	/* Key to disable Super-I/O */

#define SIO_REG_LDSEL		0x07	/* Logical device select */

#define SIO_FINTEK_ID		0x1934	/* Manufacturers ID */

#define SIO_F71808_ID		0x0901
#define SIO_F71858_ID		0x0507
#define SIO_F71862_ID		0x0601
#define SIO_F71868_ID		0x1106
#define SIO_F71869_ID		0x0814
#define SIO_F71869A_ID		0x1007
#define SIO_F71882_ID		0x0541
#define SIO_F71889_ID		0x0723
#define SIO_F71889A_ID		0x1005
#define SIO_F81865_ID		0x0704
#define SIO_F81866_ID		0x1010

static void superio_enter(u16 sioaddr)
{
	/* according to the datasheet the key must be sent twice! */
	outb(SIO_UNLOCK_KEY, sioaddr);
	outb(SIO_UNLOCK_KEY, sioaddr);
}

static void superio_exit(u16 sioaddr)
{
	outb(SIO_LOCK_KEY, sioaddr);
}

static void fintek_superio_find(u16 sioaddr)
{
	struct superio_chip *chip;
	u16 vid;

	superio_enter(sioaddr);

	vid = superio_inw(sioaddr, SIO_REG_MANID);
	if (vid != SIO_FINTEK_ID) {
		pr_debug("Not a Fintek device (port=0x%02x, vid=0x%04x)\n",
			 sioaddr, vid);
		return;
	}

	chip = xzalloc(sizeof(*chip));

	chip->devid = superio_inw(sioaddr, SIO_REG_DEVID);
	chip->vid = vid;
	chip->sioaddr = sioaddr;
	chip->enter = superio_enter;
	chip->exit = superio_exit;

	superio_chip_add(chip);

	switch (chip->devid) {
	case SIO_F71808_ID:
		superio_func_add(chip, "f71808fg_wdt");
		break;
	case SIO_F71862_ID:
		superio_func_add(chip, "f71862fg_wdt");
		break;
	case SIO_F71868_ID:
		superio_func_add(chip, "f71868_wdt");
		break;
	case SIO_F71869_ID:
		superio_func_add(chip, "f71869_wdt");
		superio_func_add(chip, "gpio-f71869");
		break;
	case SIO_F71869A_ID:
		superio_func_add(chip, "f71869_wdt");
		superio_func_add(chip, "gpio-f71869a");
		break;
	case SIO_F71882_ID:
		superio_func_add(chip, "f71882fg_wdt");
		superio_func_add(chip, "gpio-f71882fg");
		break;
	case SIO_F71889_ID:
		superio_func_add(chip, "f71889fg_wdt");
		superio_func_add(chip, "gpio-f71889f");
		break;
	case SIO_F71889A_ID:
		superio_func_add(chip, "f71889fg_wdt");
		superio_func_add(chip, "gpio-f71889a");
		break;
	case SIO_F71858_ID:
		/* Confirmed (by datasheet) not to have a watchdog. */
		break;
	case SIO_F81865_ID:
		superio_func_add(chip, "f81865_wdt");
		break;
	case SIO_F81866_ID:
		superio_func_add(chip, "f81866_wdt");
		superio_func_add(chip, "gpio-f81866");
		break;
	default:
		pr_info("Unrecognized Fintek device: 0x%04x\n", chip->devid);
	}

	superio_exit(sioaddr);
}

static int fintek_superio_detect(void)
{
	fintek_superio_find(0x2e);
	fintek_superio_find(0x4e);

	return 0;
}
coredevice_initcall(fintek_superio_detect);
