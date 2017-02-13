/* linux/drivers/mtd/nand/s3c2410.c
 *
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
 *
 * Copyright © 2004-2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * Samsung S3C2410 NAND driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
*/

#include <config.h>
#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <init.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <mach/s3c-generic.h>
#include <mach/s3c-iomap.h>
#include <mach/s3c24xx-nand.h>
#include <io.h>
#include <asm-generic/errno.h>
#include <asm/sections.h>

#ifdef CONFIG_S3C_NAND_BOOT
# define __nand_boot_init __bare_init
# ifndef BOARD_DEFAULT_NAND_TIMING
#  define BOARD_DEFAULT_NAND_TIMING 0x0737
# endif
#else
# define __nand_boot_init
#endif

/**
 * Define this symbol for testing purpose. It will add a command to read an
 * image from the NAND like it the boot strap code will do.
 */
#define CONFIG_NAND_S3C_BOOT_DEBUG

/* NAND controller's register */

#define NFCONF 0x00

#ifdef CONFIG_CPU_S3C2410

#define NFCMD 0x04
#define NFADDR 0x08
#define NFDATA 0x0c
#define NFSTAT 0x10
#define NFECC 0x14

/* S3C2410 specific bits */
#define NFSTAT_BUSY (1)
#define NFCONF_nFCE (1 << 11)
#define NFCONF_INITECC (1 << 12)
#define NFCONF_EN (1 << 15)

#endif	/* CONFIG_CPU_S3C2410 */

#ifdef CONFIG_CPU_S3C2440

#define NFCONT 0x04
#define NFCMD 0x08
#define NFADDR 0x0C
#define NFDATA 0x10
#define NFSTAT 0x20
#define NFECC 0x2C

/* S3C2440 specific bits */
#define NFSTAT_BUSY (1)
#define NFCONT_nFCE (1 << 1)
#define NFCONT_INITECC (1 << 4)
#define NFCONT_EN (1)

#endif	/* CONFIG_CPU_S3C2440 */


struct s3c24x0_nand_host {
	struct mtd_info		mtd;
	struct nand_chip	nand;
	struct mtd_partition	*parts;
	struct device_d		*dev;

	void __iomem		*base;
};

/**
 * oob placement block for use with hardware ecc generation on small page
 */
static struct nand_ecclayout nand_hw_eccoob = {
	.eccbytes = 3,
	.eccpos = { 0, 1, 2},
	.oobfree = {
		{
			.offset = 8,
			.length = 8
		}
	}
};

/* - Functions shared between the boot strap code and the regular driver - */

/**
 * Issue the specified command to the NAND device
 * @param[in] host Base address of the NAND controller
 * @param[in] cmd Command for NAND flash
 */
static void __nand_boot_init send_cmd(void __iomem *host, uint8_t cmd)
{
	writeb(cmd, host + NFCMD);
}

/**
 * Issue the specified address to the NAND device
 * @param[in] host Base address of the NAND controller
 * @param[in] addr Address for the NAND flash
 */
static void __nand_boot_init send_addr(void __iomem *host, uint8_t addr)
{
	writeb(addr, host + NFADDR);
}

/**
 * Enable the NAND flash access
 * @param[in] host Base address of the NAND controller
 */
static void __nand_boot_init enable_cs(void __iomem *host)
{
#ifdef CONFIG_CPU_S3C2410
	writew(readw(host + NFCONF) & ~NFCONF_nFCE, host + NFCONF);
#endif
#ifdef CONFIG_CPU_S3C2440
	writew(readw(host + NFCONT) & ~NFCONT_nFCE, host + NFCONT);
#endif
}

/**
 * Disable the NAND flash access
 * @param[in] host Base address of the NAND controller
 */
static void __nand_boot_init disable_cs(void __iomem *host)
{
#ifdef CONFIG_CPU_S3C2410
	writew(readw(host + NFCONF) | NFCONF_nFCE, host + NFCONF);
#endif
#ifdef CONFIG_CPU_S3C2440
	writew(readw(host + NFCONT) | NFCONT_nFCE, host + NFCONT);
#endif
}

/**
 * Enable the NAND flash controller
 * @param[in] host Base address of the NAND controller
 * @param[in] timing Timing to access the NAND memory
 */
static void __nand_boot_init enable_nand_controller(void __iomem *host, uint32_t timing)
{
#ifdef CONFIG_CPU_S3C2410
	writew(timing + NFCONF_EN + NFCONF_nFCE, host + NFCONF);
#endif
#ifdef CONFIG_CPU_S3C2440
	writew(NFCONT_EN + NFCONT_nFCE, host + NFCONT);
	writew(timing, host + NFCONF);
#endif
}

/**
 * Diable the NAND flash controller
 * @param[in] host Base address of the NAND controller
 */
static void __nand_boot_init disable_nand_controller(void __iomem *host)
{
#ifdef CONFIG_CPU_S3C2410
	writew(NFCONF_nFCE, host + NFCONF);
#endif
#ifdef CONFIG_CPU_S3C2440
	writew(NFCONT_nFCE, host + NFCONT);
#endif
}

/* ----------------------------------------------------------------------- */

#ifdef CONFIG_CPU_S3C2440
/**
 * Read one block of data from the NAND port
 * @param[in] mtd Instance data
 * @param[out] buf buffer to write data to
 * @param[in] len byte count
 *
 * This is a special block read variant for the S3C2440 CPU.
 */
static void s3c2440_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct s3c24x0_nand_host *host = nand_chip->priv;

	readsl(host->base + NFDATA, buf, len >> 2);

	/* cleanup any fractional read */
	if (len & 3) {
		buf += len & ~3;

		for (; len & 3; len--)
			*buf++ = readb(host->base + NFDATA);
	}
}

/**
 * Write one block of data to the NAND port
 * @param[in] mtd Instance data
 * @param[out] buf buffer to read data from
 * @param[in] len byte count
 *
 * This is a special block write variant for the S3C2440 CPU.
 */
static void s3c2440_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf,
					int len)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct s3c24x0_nand_host *host = nand_chip->priv;

	writesl(host->base + NFDATA, buf, len >> 2);

	/* cleanup any fractional write */
	if (len & 3) {
		buf += len & ~3;

		for (; len & 3; len--, buf++)
			writeb(*buf, host->base + NFDATA);
	}
}
#endif

/**
 * Check the ECC and try to repair the data if possible
 * @param[in] mtd_info Not used
 * @param[inout] dat Pointer to the data buffer that might contain a bit error
 * @param[in] read_ecc ECC data from the OOB space
 * @param[in] calc_ecc ECC data calculated from the data
 * @return 0 no error, 1 repaired error, -1 no way...
 *
 * @note: This routine works always on a 24 bit ECC
 */
static int s3c2410_nand_correct_data(struct mtd_info *mtd, uint8_t *dat,
				uint8_t *read_ecc, uint8_t *calc_ecc)
{
	unsigned int diff0, diff1, diff2;
	unsigned int bit, byte;

	diff0 = read_ecc[0] ^ calc_ecc[0];
	diff1 = read_ecc[1] ^ calc_ecc[1];
	diff2 = read_ecc[2] ^ calc_ecc[2];

	if (diff0 == 0 && diff1 == 0 && diff2 == 0)
		return 0;		/* ECC is ok */

	/* sometimes people do not think about using the ECC, so check
	 * to see if we have an 0xff,0xff,0xff read ECC and then ignore
	 * the error, on the assumption that this is an un-eccd page.
	 */
	if (read_ecc[0] == 0xff && read_ecc[1] == 0xff && read_ecc[2] == 0xff)
		return 0;

	/* Can we correct this ECC (ie, one row and column change).
	 * Note, this is similar to the 256 error code on smartmedia */

	if (((diff0 ^ (diff0 >> 1)) & 0x55) == 0x55 &&
	    ((diff1 ^ (diff1 >> 1)) & 0x55) == 0x55 &&
	    ((diff2 ^ (diff2 >> 1)) & 0x55) == 0x55) {
		/* calculate the bit position of the error */

		bit  = ((diff2 >> 3) & 1) |
		       ((diff2 >> 4) & 2) |
		       ((diff2 >> 5) & 4);

		/* calculate the byte position of the error */

		byte = ((diff2 << 7) & 0x100) |
		       ((diff1 << 0) & 0x80)  |
		       ((diff1 << 1) & 0x40)  |
		       ((diff1 << 2) & 0x20)  |
		       ((diff1 << 3) & 0x10)  |
		       ((diff0 >> 4) & 0x08)  |
		       ((diff0 >> 3) & 0x04)  |
		       ((diff0 >> 2) & 0x02)  |
		       ((diff0 >> 1) & 0x01);

		dat[byte] ^= (1 << bit);
		return 1;
	}

	/* if there is only one bit difference in the ECC, then
	 * one of only a row or column parity has changed, which
	 * means the error is most probably in the ECC itself */

	diff0 |= (diff1 << 8);
	diff0 |= (diff2 << 16);

	if ((diff0 & ~(1<<fls(diff0))) == 0)
		return 1;

	return -1;
}

static void s3c2410_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct s3c24x0_nand_host *host = nand_chip->priv;

#ifdef CONFIG_CPU_S3C2410
	writel(readl(host->base + NFCONF) | NFCONF_INITECC , host->base + NFCONF);
#endif
#ifdef CONFIG_CPU_S3C2440
	writel(readl(host->base + NFCONT) | NFCONT_INITECC , host->base + NFCONT);
#endif
}

static int s3c2410_nand_calculate_ecc(struct mtd_info *mtd, const uint8_t *dat, uint8_t *ecc_code)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct s3c24x0_nand_host *host = nand_chip->priv;

#ifdef CONFIG_CPU_S3C2410
	ecc_code[0] = readb(host->base + NFECC);
	ecc_code[1] = readb(host->base + NFECC + 1);
	ecc_code[2] = readb(host->base + NFECC + 2);
#endif
#ifdef CONFIG_CPU_S3C2440
	unsigned long ecc = readl(host->base + NFECC);

	ecc_code[0] = ecc;
	ecc_code[1] = ecc >> 8;
	ecc_code[2] = ecc >> 16;
#endif
	return 0;
}

static void s3c24x0_nand_select_chip(struct mtd_info *mtd, int chip)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct s3c24x0_nand_host *host = nand_chip->priv;

	if (chip == -1)
		disable_cs(host->base);
	else
		enable_cs(host->base);
}

static int s3c24x0_nand_devready(struct mtd_info *mtd)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct s3c24x0_nand_host *host = nand_chip->priv;

	return readw(host->base + NFSTAT) & NFSTAT_BUSY;
}

static void s3c24x0_nand_hwcontrol(struct mtd_info *mtd, int cmd,
					unsigned int ctrl)
{
	struct nand_chip *nand_chip = mtd->priv;
	struct s3c24x0_nand_host *host = nand_chip->priv;

	if (cmd == NAND_CMD_NONE)
		return;
	/*
	* If the CLE should be active, this call is a NAND command
	*/
	if (ctrl & NAND_CLE)
		send_cmd(host->base, cmd);
	/*
	* If the ALE should be active, this call is a NAND address
	*/
	if (ctrl & NAND_ALE)
		send_addr(host->base, cmd);
}

static int s3c24x0_nand_inithw(struct s3c24x0_nand_host *host)
{
	struct s3c24x0_nand_platform_data *pdata = host->dev->platform_data;
	uint32_t tmp;

	/* reset the NAND controller */
	disable_nand_controller(host->base);

	if (pdata != NULL)
		tmp = pdata->nand_timing;
	else
		/* else slowest possible timing */
		tmp = CALC_NFCONF_TIMING(4, 8, 8);

	/* reenable the NAND controller */
	enable_nand_controller(host->base, tmp);

	return 0;
}

static int s3c24x0_nand_probe(struct device_d *dev)
{
	struct resource *iores;
	struct nand_chip *chip;
	struct s3c24x0_nand_platform_data *pdata = dev->platform_data;
	struct mtd_info *mtd;
	struct s3c24x0_nand_host *host;
	int ret;

	/* Allocate memory for MTD device structure and private data */
	host = kzalloc(sizeof(struct s3c24x0_nand_host), GFP_KERNEL);
	if (!host)
		return -ENOMEM;

	host->dev = dev;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	host->base = IOMEM(iores->start);

	/* structures must be linked */
	chip = &host->nand;
	mtd = &host->mtd;
	mtd->priv = chip;
	mtd->parent = dev;

	/* init the default settings */

	/* 50 us command delay time */
	chip->chip_delay = 50;
	chip->priv = host;

	chip->IO_ADDR_R = chip->IO_ADDR_W = host->base + NFDATA;

#ifdef CONFIG_CPU_S3C2440
	chip->read_buf = s3c2440_nand_read_buf;
	chip->write_buf = s3c2440_nand_write_buf;
#endif
	chip->cmd_ctrl = s3c24x0_nand_hwcontrol;
	chip->dev_ready = s3c24x0_nand_devready;
	chip->select_chip = s3c24x0_nand_select_chip;

	/* we are using the hardware ECC feature of this device */
	chip->ecc.calculate = s3c2410_nand_calculate_ecc;
	chip->ecc.correct = s3c2410_nand_correct_data;
	chip->ecc.hwctl = s3c2410_nand_enable_hwecc;

	/*
	 * Setup ECC handling in accordance to the kernel
	 * - 1 times 512 bytes with 24 bit ECC for small page
	 * - 8 times 256 bytes with 24 bit ECC each for large page
	 */
	chip->ecc.mode = NAND_ECC_HW;
	chip->ecc.bytes = 3;	/* always 24 bit ECC per turn */
	chip->ecc.strength  = 1;

#ifdef CONFIG_CPU_S3C2440
	if (readl(host->base) & 0x8) {
		/* large page (2048 bytes per page) */
		chip->ecc.size = 256;
	} else
#endif
	{
		/* small page (512 bytes per page) */
		chip->ecc.size = 512;
		chip->ecc.layout = &nand_hw_eccoob;
	}

	if (pdata->flash_bbt) {
		/* use a flash based bbt */
		chip->bbt_options |= NAND_BBT_USE_FLASH;
	}

	ret = s3c24x0_nand_inithw(host);
	if (ret != 0)
		goto on_error;

	/* Scan to find existence of the device */
	ret = nand_scan(mtd, 1);
	if (ret != 0) {
		ret = -ENXIO;
		goto on_error;
	}

	return add_mtd_nand_device(mtd, "nand");

on_error:
	free(host);
	return ret;
}

static struct driver_d s3c24x0_nand_driver = {
	.name  = "s3c24x0_nand",
	.probe = s3c24x0_nand_probe,
};
device_platform_driver(s3c24x0_nand_driver);

#ifdef CONFIG_S3C_NAND_BOOT

static void __nand_boot_init wait_for_completion(void __iomem *host)
{
	while (!(readw(host + NFSTAT) & NFSTAT_BUSY))
		;
}

/**
 * Convert a page offset into a page address for the NAND
 * @param host Where to write the address to
 * @param offs Page's offset in the NAND
 * @param ps Page size (512 or 2048)
 * @param c Address cycle count (3, 4 or 5)
 *
 * Uses the offset of the page to generate an page address into the NAND. This
 * differs when using a 512 byte or 2048 bytes per page NAND.
 * The column part of the page address to be generated is always forced to '0'.
 */
static void __nand_boot_init nfc_addr(void __iomem *host, uint32_t offs,
					int ps, int c)
{
	send_addr(host, 0); /* column part 1 */

	if (ps == 512) {
		send_addr(host, offs >> 9);
		send_addr(host, offs >> 17);
		if (c > 3)
			send_addr(host, offs >> 25);
	} else {
		send_addr(host, 0); /* column part 2 */
		send_addr(host, offs >> 11);
		send_addr(host, offs >> 19);
		if (c > 4)
			send_addr(host, offs >> 27);
		send_cmd(host, NAND_CMD_READSTART);
	}
}

/**
 * Load a sequential count of pages from the NAND into memory
 * @param[out] dest Pointer to target area (in SDRAM)
 * @param[in] size Bytes to read from NAND device
 * @param[in] page Start page to read from
 *
 * This function must be located in the first 4kiB of the barebox image
 * (guess why).
 */
void __nand_boot_init s3c24x0_nand_load_image(void *dest, int size, int page)
{
	void __iomem *host = (void __iomem *)S3C24X0_NAND_BASE;
	unsigned pagesize;
	int i, cycle;

	/*
	 * Reenable the NFC and use the default (but slow) access
	 * timing or the board specific setting if provided.
	 */
	enable_nand_controller(host, BOARD_DEFAULT_NAND_TIMING);

	/* use the current NAND hardware configuration */
	switch (readl(S3C24X0_NAND_BASE) & 0xf) {
	case 0x6:	/* 8 bit, 4 addr cycles, 512 bpp, normal NAND */
		pagesize = 512;
		cycle = 4;
		break;
	case 0xc:	/* 8 bit, 4 addr cycles, 2048 bpp, advanced NAND */
		pagesize = 2048;
		cycle = 4;
		break;
	case 0xe:	/* 8 bit, 5 addr cycles, 2048 bpp, advanced NAND */
		pagesize = 2048;
		cycle = 5;
		break;
	default:
		/* we cannot output an error message here :-( */
		disable_nand_controller(host);
		return;
	}

	enable_cs(host);

	/* Reset the NAND device */
	send_cmd(host, NAND_CMD_RESET);
	wait_for_completion(host);
	disable_cs(host);

	do {
		enable_cs(host);
		send_cmd(host, NAND_CMD_READ0);
		nfc_addr(host, page * pagesize, pagesize, cycle);
		wait_for_completion(host);
		/* copy one page (do *not* use readsb() here!)*/
		for (i = 0; i < pagesize; i++)
			writeb(readb(host + NFDATA), (void __iomem *)(dest + i));
		disable_cs(host);

		page++;
		dest += pagesize;
		size -= pagesize;
	} while (size >= 0);

	/* disable the controller again */
	disable_nand_controller(host);
}

#include <asm/sections.h>

void __nand_boot_init nand_boot(void)
{
	void *dest = _text;
	int size = barebox_image_size;
	int page = 0;

	s3c24x0_nand_load_image(dest, size, page);
}
#ifdef CONFIG_NAND_S3C_BOOT_DEBUG
#include <command.h>

static int do_nand_boot_test(int argc, char *argv[])
{
	void *dest;
	int size;

	if (argc < 3)
		return COMMAND_ERROR_USAGE;

	dest = (void *)strtoul_suffix(argv[1], NULL, 0);
	size = strtoul_suffix(argv[2], NULL, 0);

	s3c24x0_nand_load_image(dest, size, 0);

	/* re-enable the controller again, as this was a test only */
	enable_nand_controller((void *)S3C24X0_NAND_BASE,
				BOARD_DEFAULT_NAND_TIMING);

	return 0;
}

BAREBOX_CMD_START(nand_boot_test)
	.cmd		= do_nand_boot_test,
	BAREBOX_CMD_DESC("load an image from NAND")
	BAREBOX_CMD_OPTS("DEST SIZE")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
BAREBOX_CMD_END
#endif

#endif /* CONFIG_S3C_NAND_BOOT */

/**
 * @file
 * @brief Support for various kinds of NAND devices
 *
 * ECC handling in this driver (in accordance to the current 2.6.38 kernel):
 * - for small page NANDs it generates 3 ECC bytes out of 512 data bytes
 * - for large page NANDs it generates 24 ECC bytes out of 2048 data bytes
 *
 * As small page NANDs are using 48 bits ECC per default, this driver uses a
 * local OOB layout description, to shrink it down to 24 bits. This is a bad
 * idea, but we cannot change it here, as the kernel is using this layout.
 *
 * For large page NANDs this driver uses the default layout, as the kernel does.
 */
