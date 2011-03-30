/**
 * @file
 * @brief Provide Generic GPMC NAND implementation for OMAP platforms
 *
 * FileName: arch/arm/mach-omap/gpmc_nand.c
 *
 * GPMC has a NAND controller inbuilt. This provides a generic implementation
 * for board files to register a nand device. drivers/nand/nand_base.c takes
 * care of identifing the type of device, size etc.
 *
 * A typical device registration is as follows:
 *
 * @code
 * static struct device_d my_nand_device = {
 *	.name = "gpmc_nand",
 *	.id = some identifier you need to show.. e.g. "gpmc_nand0"
 *	.map_base = GPMC base address
 *	.size = GPMC address map size.
 *	.platform_data = platform data - required - explained below
 * };
 * platform data required:
 * static struct gpmc_nand_platform_data nand_plat = {
 *	.cs = give the chip select of the device
 *	.device_width = what is the width of the device 8 or 16?
 *	.max_timeout = delay desired for operation
 *	.wait_mon_pin = do you use wait monitoring? if so wait pin
 *	.plat_options = platform options.
 *		NAND_HWECC_ENABLE/DISABLE - hw ecc enable/disable
 *		NAND_WAITPOL_LOW/HIGH - wait pin polarity
 *	.oob = if you would like to replace oob with a custom OOB.
 *	.nand_setup  = if you would like a special setup function to be called
 *	.priv = any params you'd like to save(e.g. like nand_setup to use)
 *};
 * then in your code, you'd device_register(&my_nand_device);
 * @endcode
 *
 * Note:
 * @li Enable CONFIG_NAND_OMAP_GPMC_HWECC in menuconfig to get H/w ECC support
 * @li You may choose to register two "devices" for the same CS to get BOTH
 * hwecc and swecc devices.
 * @li You can choose to have your own OOB definition for compliance with ROM
 * code organization - only if you dont want to use NAND's default oob layout.
 * see GPMC_NAND_ECC_LP_x8_LAYOUT etc..
 *
 * @see gpmc_nand_platform_data
 * @warning Remember to initialize GPMC before initializing the nand dev.
 */
/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
 *
 * Based on:
 * drivers/mtd/nand/omap2.c from linux kernel
 *
 * Copyright (c) 2004 Texas Instruments, Jian Zhang <jzhang@ti.com>
 * Copyright (c) 2004 Micron Technology Inc.
 * Copyright (c) 2004 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <errno.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <clock.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <asm/io.h>
#include <mach/silicon.h>
#include <mach/gpmc.h>
#include <mach/gpmc_nand.h>

/* Enable me to get tons of debug messages -for use without jtag */
#if 0
#define gpmcnand_dbg(FORMAT, ARGS...) fprintf(stdout,\
		"gpmc_nand:%s:%d:Entry:"FORMAT"\n",\
		__func__, __LINE__, ARGS)
#else
#define gpmcnand_dbg(FORMAT, ARGS...)
#endif
#define gpmcnand_err(ARGS...) fprintf(stderr, "omapnand: " ARGS);

/** internal structure maintained for nand information */
struct gpmc_nand_info {
	struct nand_hw_control controller;
	struct device_d *pdev;
	struct gpmc_nand_platform_data *pdata;
	struct nand_chip nand;
	struct mtd_info minfo;
	int gpmc_cs;
	void *gpmc_command;
	void *gpmc_address;
	void *gpmc_data;
	unsigned long gpmc_base;
	unsigned char wait_mon_mask;
	uint64_t timeout;
	unsigned inuse:1;
	unsigned wait_pol:1;
	unsigned char ecc_parity_pairs;
	unsigned int ecc_config;
};

/* Typical BOOTROM oob layouts-requires hwecc **/

/** Large Page x8 NAND device Layout */
static struct nand_ecclayout ecc_lp_x8 = {
	.eccbytes = 12,
	.eccpos = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
	.oobfree = {
		{
			.offset = 60,
			.length = 2,
		}
	}
};

/** Large Page x16 NAND device Layout */
static struct nand_ecclayout ecc_lp_x16 = {
	.eccbytes = 12,
	.eccpos = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13},
	.oobfree = {
		{
			.offset = 60,
			.length = 2,
		}
	}
};

/** Small Page x8 NAND device Layout */
static struct nand_ecclayout ecc_sp_x8 = {
	.eccbytes = 3,
	.eccpos = {1, 2, 3},
	.oobfree = {
		{
			.offset = 14,
			.length = 2,
		}
	}
};

/** Small Page x16 NAND device Layout */
static struct nand_ecclayout ecc_sp_x16 = {
	.eccbytes = 3,
	.eccpos = {2, 3, 4},
	.oobfree = {
		{
			.offset = 14,
			.length = 2 }
	}
};

/**
 * @brief calls the platform specific dev_ready functionds
 *
 * @param mtd - mtd info structure
 *
 * @return
 */
static int omap_dev_ready(struct mtd_info *mtd)
{
	struct nand_chip *nand = (struct nand_chip *)(mtd->priv);
	struct gpmc_nand_info *oinfo = (struct gpmc_nand_info *)(nand->priv);
	uint64_t start = get_time_ns();
	unsigned long comp;

	gpmcnand_dbg("mtd=%x", (unsigned int)mtd);
	/* What do we mean by assert and de-assert? */
	comp = (oinfo->wait_pol == NAND_WAITPOL_HIGH) ?
	    oinfo->wait_mon_mask : 0x0;
	while (1) {
		/* Breakout condition */
		if (is_timeout(start, oinfo->timeout)) {
			gpmcnand_err("timedout\n");
			return -ETIMEDOUT;
		}
		/* if the wait is released, we are good to go */
		if (comp ==
		    (readl(oinfo->gpmc_base + GPMC_STATUS) &&
		     oinfo->wait_mon_mask))
			break;
	}
	return 0;
}

/**
 * @brief This function will enable or disable the Write Protect feature on
 * NAND device. GPMC has a single WP bit for all CS devices..
 *
 * @param oinfo  omap nand info
 * @param mode 0-disable else enable
 *
 * @return none
 */
static void gpmc_nand_wp(struct gpmc_nand_info *oinfo, int mode)
{
	unsigned long config = readl(oinfo->gpmc_base + GPMC_CFG);

	gpmcnand_dbg("mode=%x", mode);
	if (mode)
		config &= ~(NAND_WP_BIT);	/* WP is ON */
	else
		config |= (NAND_WP_BIT);	/* WP is OFF */

	writel(config, oinfo->gpmc_base + GPMC_CFG);
}

/**
 * @brief respond to hw event change request
 *
 * MTD layer uses NAND_CTRL_CLE etc to control selection of the latch
 * we hoodwink by changing the R and W registers according to the state
 * we are requested.
 *
 * @param mtd - mtd info structure
 * @param cmd command mtd layer is requesting
 *
 * @return none
 */
static void omap_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *nand = (struct nand_chip *)(mtd->priv);
	struct gpmc_nand_info *oinfo = (struct gpmc_nand_info *)(nand->priv);
	gpmcnand_dbg("mtd=%x nand=%x cmd=%x ctrl = %x", (unsigned int)mtd, nand,
		  cmd, ctrl);
	switch (ctrl) {
	case NAND_CTRL_CHANGE | NAND_CTRL_CLE:
		nand->IO_ADDR_W = oinfo->gpmc_command;
		nand->IO_ADDR_R = oinfo->gpmc_data;
		break;

	case NAND_CTRL_CHANGE | NAND_CTRL_ALE:
		nand->IO_ADDR_W = oinfo->gpmc_address;
		nand->IO_ADDR_R = oinfo->gpmc_data;
		break;

	case NAND_CTRL_CHANGE | NAND_NCE:
		nand->IO_ADDR_W = oinfo->gpmc_data;
		nand->IO_ADDR_R = oinfo->gpmc_data;
		break;
	}

	if (cmd != NAND_CMD_NONE)
		writeb(cmd, nand->IO_ADDR_W);
	return;
}

/**
 * @brief This function will generate true ECC value, which can be used
 * when correcting data read from NAND flash memory core
 *
 * @param ecc_buf buffer to store ecc code
 *
 * @return re-formatted ECC value
 */
static unsigned int gen_true_ecc(u8 *ecc_buf)
{
	gpmcnand_dbg("ecc_buf=%x 1, 2 3 = %x %x %x", (unsigned int)ecc_buf,
		  ecc_buf[0], ecc_buf[1], ecc_buf[2]);
	return ecc_buf[0] | (ecc_buf[1] << 16) | ((ecc_buf[2] & 0xF0) << 20) |
	    ((ecc_buf[2] & 0x0F) << 8);
}

/**
 * @brief Compares the ecc read from nand spare area with ECC
 * registers values and corrects one bit error if it has occured
 * Further details can be had from OMAP TRM and the following selected links:
 * http://en.wikipedia.org/wiki/Hamming_code
 * http://www.cs.utexas.edu/users/plaxton/c/337/05f/slides/ErrorCorrection-4.pdf
 *
 * @param mtd - mtd info structure
 * @param dat  page data
 * @param read_ecc ecc readback
 * @param calc_ecc calculated ecc (from reg)
 *
 * @return 0 if data is OK or corrected, else returns -1
 */
static int omap_correct_data(struct mtd_info *mtd, uint8_t *dat,
			     uint8_t *read_ecc, uint8_t *calc_ecc)
{
	unsigned int orig_ecc, new_ecc, res, hm;
	unsigned short parity_bits, byte;
	unsigned char bit;
	struct nand_chip *nand = (struct nand_chip *)(mtd->priv);
	struct gpmc_nand_info *oinfo = (struct gpmc_nand_info *)(nand->priv);

	gpmcnand_dbg("mtd=%x dat=%x read_ecc=%x calc_ecc=%x", (unsigned int)mtd,
		  (unsigned int)dat, (unsigned int)read_ecc,
		  (unsigned int)calc_ecc);

	/* Regenerate the orginal ECC */
	orig_ecc = gen_true_ecc(read_ecc);
	new_ecc = gen_true_ecc(calc_ecc);
	/* Get the XOR of real ecc */
	res = orig_ecc ^ new_ecc;
	if (res) {
		/* Get the hamming width */
		hm = hweight32(res);
		/* Single bit errors can be corrected! */
		if (hm == oinfo->ecc_parity_pairs) {
			/* Correctable data! */
			parity_bits = res >> 16;
			bit = (parity_bits & 0x7);
			byte = (parity_bits >> 3) & 0x1FF;
			/* Flip the bit to correct */
			dat[byte] ^= (0x1 << bit);

		} else if (hm == 1) {
			gpmcnand_err("Ecc is wrong\n");
			/* ECC itself is corrupted */
			return 2;
		} else {
			gpmcnand_err("bad compare! failed\n");
			/* detected 2 bit error */
			return -1;
		}
	}
	return 0;
}

/**
 * @brief Using noninverted ECC can be considered ugly since writing a blank
 * page ie. padding will clear the ECC bytes. This is no problem as long
 * nobody is trying to write data on the seemingly unused page. Reading
 * an erased page will produce an ECC mismatch between generated and read
 * ECC bytes that has to be dealt with separately.
 *
 * @param mtd - mtd info structure
 * @param dat data being written
 * @param ecc_code ecc code returned back to nand layer
 *
 * @return 0
 */
static int omap_calculate_ecc(struct mtd_info *mtd, const uint8_t *dat,
			      uint8_t *ecc_code)
{
	struct nand_chip *nand = (struct nand_chip *)(mtd->priv);
	struct gpmc_nand_info *oinfo = (struct gpmc_nand_info *)(nand->priv);
	unsigned int val;
	gpmcnand_dbg("mtd=%x dat=%x ecc_code=%x", (unsigned int)mtd,
		  (unsigned int)dat, (unsigned int)ecc_code);
	debug("ecc 0 1 2 = %x %x %x", ecc_code[0], ecc_code[1], ecc_code[2]);

	/* Since we smartly tell mtd driver to use eccsize of 512, only
	 * ECC Reg1 will be used.. we just read that */
	val = readl(oinfo->gpmc_base + GPMC_ECC1_RESULT);
	ecc_code[0] = val & 0xFF;
	ecc_code[1] = (val >> 16) & 0xFF;
	ecc_code[2] = ((val >> 8) & 0x0f) | ((val >> 20) & 0xf0);

	/* Stop reading anymore ECC vals and clear old results
	 * enable will be called if more reads are required */
	writel(0x000, oinfo->gpmc_base + GPMC_ECC_CONFIG);
	return 0;
}

/*
 * omap_enable_ecc - This function enables the hardware ecc functionality
 * @param mtd - mtd info structure
 * @param mode - Read/Write mode
 */
static void omap_enable_hwecc(struct mtd_info *mtd, int mode)
{
	struct nand_chip *nand = (struct nand_chip *)(mtd->priv);
	struct gpmc_nand_info *oinfo = (struct gpmc_nand_info *)(nand->priv);
	gpmcnand_dbg("mtd=%x mode=%x", (unsigned int)mtd, mode);
	switch (mode) {
	case NAND_ECC_READ:
	case NAND_ECC_WRITE:
		/* Clear the ecc result registers
		 * select ecc reg as 1
		 */
		writel(0x101, oinfo->gpmc_base + GPMC_ECC_CONTROL);
		/* Size 0 = 0xFF, Size1 is 0xFF - both are 512 bytes
		 * tell all regs to generate size0 sized regs
		 * we just have a single ECC engine for all CS
		 */
		writel(0x3FCFF000, oinfo->gpmc_base +
				GPMC_ECC_SIZE_CONFIG);
		writel(oinfo->ecc_config, oinfo->gpmc_base +
				GPMC_ECC_CONFIG);
		break;
	default:
		gpmcnand_err("Error: Unrecognized Mode[%d]!\n", mode);
		break;
	}
}

/**
 * @brief nand device probe.
 *
 * @param pdev -matching device
 *
 * @return -failure reason or give 0
 */
static int gpmc_nand_probe(struct device_d *pdev)
{
	struct gpmc_nand_info *oinfo;
	struct gpmc_nand_platform_data *pdata;
	struct nand_chip *nand;
	struct mtd_info *minfo;
	unsigned long cs_base;
	int err;
	struct nand_ecclayout *layout, *lsp, *llp;

	gpmcnand_dbg("pdev=%x", (unsigned int)pdev);
	pdata = (struct gpmc_nand_platform_data *)pdev->platform_data;
	if (pdata == NULL) {
		gpmcnand_err("platform data missing\n");
		return -ENODEV;
	}

	oinfo = calloc(1, sizeof(struct gpmc_nand_info));
	if (!oinfo) {
		gpmcnand_err("oinfo alloc failed!\n");
		return -ENOMEM;
	}

	/* fill up my data structures */
	oinfo->pdev = pdev;
	oinfo->pdata = pdata;
	pdev->platform_data = (void *)oinfo;

	nand = &oinfo->nand;
	nand->priv = (void *)oinfo;

	minfo = &oinfo->minfo;
	minfo->priv = (void *)nand;

	if (pdata->cs >= GPMC_NUM_CS) {
		gpmcnand_err("Invalid CS!\n");
		err = -EINVAL;
		goto out_release_mem;
	}
	/* Setup register specific data */
	oinfo->gpmc_cs = pdata->cs;
	oinfo->gpmc_base = pdev->map_base;
	cs_base = oinfo->gpmc_base + GPMC_CONFIG1_0 +
		(pdata->cs * GPMC_CONFIG_CS_SIZE);
	oinfo->gpmc_command = (void *)(cs_base + GPMC_CS_NAND_COMMAND);
	oinfo->gpmc_address = (void *)(cs_base + GPMC_CS_NAND_ADDRESS);
	oinfo->gpmc_data = (void *)(cs_base + GPMC_CS_NAND_DATA);
	oinfo->timeout = pdata->max_timeout;
	debug("GPMC Details:\n"
		"GPMC BASE=%x\n"
		"CMD=%x\n"
		"ADDRESS=%x\n"
		"DATA=%x\n"
		"CS_BASE=%x\n",
		oinfo->gpmc_base, oinfo->gpmc_command,
		oinfo->gpmc_address, oinfo->gpmc_data, cs_base);

	/* If we are 16 bit dev, our gpmc config tells us that */
	if ((readl(cs_base) & 0x3000) == 0x1000) {
		debug("16 bit dev\n");
		nand->options |= NAND_BUSWIDTH_16;
	}

	/* Same data register for in and out */
	nand->IO_ADDR_W = nand->IO_ADDR_R = (void *)oinfo->gpmc_data;
	/*
	 * If RDY/BSY line is connected to OMAP then use the omap ready
	 * function and the generic nand_wait function which reads the
	 * status register after monitoring the RDY/BSY line. Otherwise
	 * use a standard chip delay which is slightly more than tR
	 * (AC Timing) of the NAND device and read the status register
	 * until you get a failure or success
	 */
	if (pdata->wait_mon_pin > 4) {
		gpmcnand_err("Invalid wait monitoring pin\n");
		err = -EINVAL;
		goto out_release_mem;
	}
	if (pdata->wait_mon_pin) {
		/* Set up the wait monitoring mask
		 * This is GPMC_STATUS reg relevant */
		oinfo->wait_mon_mask = (0x1 << (pdata->wait_mon_pin - 1)) << 8;
		oinfo->wait_pol = (pdata->plat_options & NAND_WAITPOL_MASK);
		nand->dev_ready = omap_dev_ready;
		nand->chip_delay = 0;
	} else {
		/* use the default nand_wait function */
		nand->chip_delay = 50;
	}

	/* Use default cmdfunc */
	/* nand cmd control */
	nand->cmd_ctrl = omap_hwcontrol;

	/* Dont do a bbt scan at the start */
	nand->options |= NAND_SKIP_BBTSCAN;

	/* State my controller */
	nand->controller = &oinfo->controller;

	/* All information is ready.. now lets call setup, if present */
	if (pdata->nand_setup) {
		err = pdata->nand_setup(pdata);
		if (err) {
			gpmcnand_err("pdataform setup failed\n");
			goto out_release_mem;
		}
	}
	/* Remove write protection */
	gpmc_nand_wp(oinfo, 0);

	/* we do not know what state of device we have is, so
	 * Send a reset to the device
	 * 8 bit write will work on 16 and 8 bit devices
	 */
	writeb(NAND_CMD_RESET, oinfo->gpmc_command);
	mdelay(1);

	/* first scan to find the device and get the page size */
	if (nand_scan_ident(minfo, 1)) {
		err = -ENXIO;
		goto out_release_mem;
	}

	switch (pdata->device_width) {
	case 8:
		lsp = &ecc_sp_x8;
		llp = &ecc_lp_x8;
		break;
	case 16:
		lsp = &ecc_sp_x16;
		llp = &ecc_lp_x16;
		break;
	default:
		err = -EINVAL;
		goto out_release_mem;
	}

	switch (minfo->writesize) {
	case 512:
		layout = lsp;
		break;
	case 2048:
		layout = llp;
		break;
	default:
		err = -EINVAL;
		goto out_release_mem;
	}

	if (pdata->ecc_mode == OMAP_ECC_HAMMING_CODE_HW_ROMCODE) {
		nand->ecc.layout = layout;

		/* Program how many columns we expect+
		 * enable the cs we want and enable the engine
		 */
		oinfo->ecc_config = (pdata->cs << 1) |
		    ((nand->options & NAND_BUSWIDTH_16) ?
		     (0x1 << 7) : 0x0) | 0x1;
		nand->ecc.hwctl = omap_enable_hwecc;
		nand->ecc.calculate = omap_calculate_ecc;
		nand->ecc.correct = omap_correct_data;
		nand->ecc.mode = NAND_ECC_HW;
		nand->ecc.size = 512;
		nand->ecc.bytes = 3;
		nand->ecc.steps = nand->ecc.layout->eccbytes / nand->ecc.bytes;
		oinfo->ecc_parity_pairs = 12;
	} else
		nand->ecc.mode = NAND_ECC_SOFT;

	/* second phase scan */
	if (nand_scan_tail(minfo)) {
		err = -ENXIO;
		goto out_release_mem;
	}

	/* We are all set to register with the system now! */
	err = add_mtd_device(minfo);
	if (err) {
		gpmcnand_err("device registration failed\n");
		goto out_release_mem;
	}
	return 0;

out_release_mem:
	if (oinfo)
		free(oinfo);

	gpmcnand_err("Failed!!\n");
	return err;
}

/** GMPC nand driver -> device registered by platforms */
static struct driver_d gpmc_nand_driver = {
	.name = "gpmc_nand",
	.probe = gpmc_nand_probe,
};

static int gpmc_nand_init(void)
{
	return register_driver(&gpmc_nand_driver);
}

device_initcall(gpmc_nand_init);
