// SPDX-License-Identifier: GPL-2.0-or-later
/* Integrated Flash Controller NAND Machine Driver
 *
 * Copyright (c) 2012 Freescale Semiconductor, Inc
 *
 * Authors: Dipen Dudhat <Dipen.Dudhat@freescale.com>
 *
 */

#include <config.h>
#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <nand.h>
#include <errno.h>
#include <clock.h>
#include <io.h>
#include <of_address.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/rawnand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/fsl_ifc.h>
#include <asm-generic/io.h>
#include "fsl_ifc.h"

#define ERR_BYTE	0xFF
#define IFC_TIMEOUT_MS	500
/* overview of the fsl ifc controller */
struct fsl_ifc_ctrl {
	struct nand_controller controller;
	/* device info */
	void __iomem *rregs;  /* Run-time register		      */
	void __iomem *gregs;  /* Global registers		      */
	uint32_t version;
	uint32_t page;       /* Last page written to / read from      */
	uint32_t read_bytes; /* Number of bytes read during command   */
	uint32_t column;     /* Saved column from SEQIN               */
	uint32_t index;      /* Pointer to next byte to 'read'        */
	uint32_t nand_stat;  /* status read from NEESR after last op  */
	uint32_t oob;        /* Non zero if operating on OOB data     */
	uint32_t eccread;    /* Non zero for a full-page ECC read     */
	uint32_t max_bitflips; /* Saved during READ0 cmd              */
	void __iomem *addr;  /* Address of assigned IFC buffer        */
};

/* mtd information per set */
struct fsl_ifc_mtd {
	struct device *dev;
	struct nand_chip chip;
	struct fsl_ifc_ctrl *ctrl;
	uint32_t cs;		/* On which chipsel NAND is connected    */
	uint32_t bufnum_mask;	/* bufnum = page & bufnum_mask */
	void __iomem *vbase;    /* Chip select base virtual address     */
	phys_addr_t pbase;	/* Chip select physical address		*/
};

static struct fsl_ifc_ctrl *ifc_ctrl;

/* Generic flash bbt descriptors */
static uint8_t bbt_pattern[] = {'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = {'1', 't', 'b', 'B' };

static struct nand_bbt_descr bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE |
		   NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs =	2, /* 0 on 8-bit small page */
	.len = 4,
	.veroffs = 6,
	.maxblocks = 4,
	.pattern = bbt_pattern,
};

static struct nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE |
		   NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs =	2, /* 0 on 8-bit small page */
	.len = 4,
	.veroffs = 6,
	.maxblocks = 4,
	.pattern = mirror_pattern,
};

static int fsl_ifc_ooblayout_ecc(struct mtd_info *mtd, int section,
		struct mtd_oob_region *oobregion)
{
	struct nand_chip *chip = mtd_to_nand(mtd);

	if (section)
		return -ERANGE;

	oobregion->offset = 8;
	oobregion->length = chip->ecc.total;

	return 0;
}

static int fsl_ifc_ooblayout_free(struct mtd_info *mtd, int section,
		struct mtd_oob_region *oobregion)
{
	struct nand_chip *chip = mtd_to_nand(mtd);

	if (section > 1)
		return -ERANGE;

	if (mtd->writesize == 512 && !(chip->options & NAND_BUSWIDTH_16)) {
		if (!section) {
			oobregion->offset = 0;
			oobregion->length = 5;
		} else {
			oobregion->offset = 6;
			oobregion->length = 2;
		}

		return 0;
	}

	if (!section) {
		oobregion->offset = 2;
		oobregion->length = 6;
	} else {
		oobregion->offset = chip->ecc.total + 8;
		oobregion->length = mtd->oobsize - oobregion->offset;
	}

	return 0;
}

static const struct mtd_ooblayout_ops fsl_ifc_ooblayout_ops = {
	.ecc = fsl_ifc_ooblayout_ecc,
	.free = fsl_ifc_ooblayout_free,
};

/*
 * Set up the IFC hardware block and page address fields, and the ifc nand
 * structure addr field to point to the correct IFC buffer in memory
 */
static void set_addr(struct mtd_info *mtd, int column, int page_addr, int oob)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct fsl_ifc_mtd *priv = chip->priv;
	struct fsl_ifc_ctrl *ctrl = priv->ctrl;
	int buf_num;

	if (page_addr != -1) {
		ctrl->page = page_addr;
		/* Program ROW0/COL0 */
		ifc_out32(ctrl->rregs + FSL_IFC_ROW0, page_addr);
		buf_num = page_addr & priv->bufnum_mask;
		ctrl->addr = priv->vbase + buf_num * (mtd->writesize * 2);
	}

	ifc_out32(ctrl->rregs + FSL_IFC_COL0, (oob ? IFC_NAND_COL_MS : 0) |
						column);
	ctrl->index = column;

	/* for OOB data point to the second half of the buffer */
	if (oob)
		ctrl->index += mtd->writesize;
}

/* returns nonzero if entire page is blank */
static int check_read_ecc(struct mtd_info *mtd, struct fsl_ifc_ctrl *ctrl,
			  uint32_t eccstat, uint32_t bufnum)
{
	return (eccstat >> ((3 - bufnum % 4) * 8)) & 15;
}

/* execute IFC NAND command and wait for it to complete */
static void fsl_ifc_run_command(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct fsl_ifc_mtd *priv = chip->priv;
	struct fsl_ifc_ctrl *ctrl = priv->ctrl;
	uint64_t time_start;
	uint32_t eccstat;
	int i;

	/* set the chip select for NAND Transaction */
	ifc_out32(ctrl->rregs + FSL_IFC_NAND_CSEL,
			priv->cs << IFC_NAND_CSEL_SHIFT);

	/* start read/write seq */
	ifc_out32(ctrl->rregs + FSL_IFC_NANDSEQ_STRT,
			IFC_NAND_SEQ_STRT_FIR_STRT);

	ctrl->nand_stat = 0;

	/* wait for NAND Machine complete flag or timeout */
	time_start = get_time_ns();
	while (!is_timeout(time_start, IFC_TIMEOUT_MS * MSECOND)) {
		ctrl->nand_stat = ifc_in32(ctrl->rregs + FSL_IFC_NAND_EVTER_STAT);

		if (ctrl->nand_stat & IFC_NAND_EVTER_STAT_OPC)
			break;
	}

	ifc_out32(ctrl->rregs + FSL_IFC_NAND_EVTER_STAT, ctrl->nand_stat);

	if (ctrl->nand_stat & IFC_NAND_EVTER_STAT_FTOER)
		pr_err("%s: Flash Time Out Error\n", __func__);
	if (ctrl->nand_stat & IFC_NAND_EVTER_STAT_WPER)
		pr_err("%s: Write Protect Error\n", __func__);

	ctrl->max_bitflips = 0;

	if (ctrl->eccread) {
		int errors;
		int bufnum = ctrl->page & priv->bufnum_mask;
		int sector_start = bufnum * chip->ecc.steps;
		int sector_end = sector_start + chip->ecc.steps - 1;

		eccstat = ifc_in32(ctrl->rregs +
				FSL_IFC_ECCSTAT(sector_start / 4));

		for (i = sector_start; i <= sector_end; i++) {
			if ((i != sector_start) && !(i % 4)) {
				eccstat = ifc_in32(ctrl->rregs +
						FSL_IFC_ECCSTAT(i / 4));
			}
			errors = check_read_ecc(mtd, ctrl, eccstat, i);

			if (errors == 15) {
				/*
				 * Uncorrectable error.
				 * We'll check for blank pages later.
				 *
				 * We disable ECCER reporting due to erratum
				 * IFC-A002770 -- so report it now if we
				 * see an uncorrectable error in ECCSTAT.
				 */
				ctrl->nand_stat |= IFC_NAND_EVTER_STAT_ECCER;
				continue;
			}

			mtd->ecc_stats.corrected += errors;
			ctrl->max_bitflips = max_t(unsigned int,
					ctrl->max_bitflips, errors);
		}

		ctrl->eccread = 0;
	}
}

static void
fsl_ifc_do_read(struct nand_chip *chip, int oob, struct mtd_info *mtd)
{
	struct fsl_ifc_mtd *priv = chip->priv;
	struct fsl_ifc_ctrl *ctrl = priv->ctrl;

	/* Program FIR/IFC_NAND_FCR0 for Small/Large page */
	if (mtd->writesize > 512) {
		ifc_out32(ctrl->rregs + FSL_IFC_FIR0,
			  (IFC_FIR_OP_CW0 << IFC_NAND_FIR0_OP0_SHIFT) |
			  (IFC_FIR_OP_CA0 << IFC_NAND_FIR0_OP1_SHIFT) |
			  (IFC_FIR_OP_RA0 << IFC_NAND_FIR0_OP2_SHIFT) |
			  (IFC_FIR_OP_CMD1 << IFC_NAND_FIR0_OP3_SHIFT) |
			  (IFC_FIR_OP_RBCD << IFC_NAND_FIR0_OP4_SHIFT));
		ifc_out32(ctrl->rregs + FSL_IFC_FIR1, 0x0);

		ifc_out32(ctrl->rregs + FSL_IFC_FCR0,
			  (NAND_CMD_READ0 << IFC_NAND_FCR0_CMD0_SHIFT) |
			  (NAND_CMD_READSTART << IFC_NAND_FCR0_CMD1_SHIFT));
	} else {
		ifc_out32(ctrl->rregs + FSL_IFC_FIR0,
			  (IFC_FIR_OP_CW0 << IFC_NAND_FIR0_OP0_SHIFT) |
			  (IFC_FIR_OP_CA0 << IFC_NAND_FIR0_OP1_SHIFT) |
			  (IFC_FIR_OP_RA0  << IFC_NAND_FIR0_OP2_SHIFT) |
			  (IFC_FIR_OP_RBCD << IFC_NAND_FIR0_OP3_SHIFT));
		ifc_out32(ctrl->rregs + FSL_IFC_FIR1, 0);

		if (oob)
			ifc_out32(ctrl->rregs + FSL_IFC_FCR0,
				  NAND_CMD_READOOB << IFC_NAND_FCR0_CMD0_SHIFT);
		else
			ifc_out32(ctrl->rregs + FSL_IFC_FCR0,
				  NAND_CMD_READ0 << IFC_NAND_FCR0_CMD0_SHIFT);
	}
}

/* cmdfunc send commands to the IFC NAND Machine */
static void fsl_ifc_cmdfunc(struct nand_chip *chip, uint32_t command,
			     int column, int page_addr)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct fsl_ifc_mtd *priv = chip->priv;
	struct fsl_ifc_ctrl *ctrl = priv->ctrl;

	/* clear the read buffer */
	ctrl->read_bytes = 0;
	if (command != NAND_CMD_PAGEPROG)
		ctrl->index = 0;

	switch (command) {
	/* READ0 read the entire buffer to use hardware ECC. */
	case NAND_CMD_READ0: {
		ifc_out32(ctrl->rregs + FSL_IFC_NAND_BC, 0);
		set_addr(mtd, 0, page_addr, 0);

		ctrl->read_bytes = mtd->writesize + mtd->oobsize;
		ctrl->index += column;

		if (chip->ecc.mode == NAND_ECC_HW)
			ctrl->eccread = 1;

		fsl_ifc_do_read(chip, 0, mtd);
		fsl_ifc_run_command(mtd);
		return;
	}

	/* READOOB reads only the OOB because no ECC is performed. */
	case NAND_CMD_READOOB:
		ifc_out32(ctrl->rregs + FSL_IFC_NAND_BC, mtd->oobsize - column);

		set_addr(mtd, column, page_addr, 1);

		ctrl->read_bytes = mtd->writesize + mtd->oobsize;

		fsl_ifc_do_read(chip, 1, mtd);
		fsl_ifc_run_command(mtd);

		return;

	case NAND_CMD_RNDOUT:
		if (chip->ecc.mode == NAND_ECC_HW)
			break;
		ifc_out32(ctrl->rregs + FSL_IFC_NAND_BC, 0);
		set_addr(mtd, column, -1, 0);
		ctrl->read_bytes = mtd->writesize + mtd->oobsize;

		/* For write size greater than 512 */
		ifc_out32(ctrl->rregs + FSL_IFC_FIR0,
			(IFC_FIR_OP_CW0 << IFC_NAND_FIR0_OP0_SHIFT) |
			(IFC_FIR_OP_CA0 << IFC_NAND_FIR0_OP1_SHIFT) |
			(IFC_FIR_OP_CMD1 << IFC_NAND_FIR0_OP3_SHIFT));
		ifc_out32(ctrl->rregs + FSL_IFC_FIR1, 0x0);

		ifc_out32(ctrl->rregs + FSL_IFC_FCR0,
			(NAND_CMD_RNDOUT << IFC_NAND_FCR0_CMD0_SHIFT) |
			(NAND_CMD_RNDOUTSTART << IFC_NAND_FCR0_CMD1_SHIFT));

		fsl_ifc_run_command(mtd);
		return;

	case NAND_CMD_READID:
	case NAND_CMD_PARAM: {
		int timing = IFC_FIR_OP_RB;
		int len = 8;

		if (command == NAND_CMD_PARAM) {
			timing = IFC_FIR_OP_RBCD;
			len = 256 * 3;
		}

		ifc_out32(ctrl->rregs + FSL_IFC_FIR0,
			  (IFC_FIR_OP_CW0 << IFC_NAND_FIR0_OP0_SHIFT) |
			  (IFC_FIR_OP_UA  << IFC_NAND_FIR0_OP1_SHIFT) |
			  (timing << IFC_NAND_FIR0_OP2_SHIFT));
		ifc_out32(ctrl->rregs + FSL_IFC_FCR0,
			  command << IFC_NAND_FCR0_CMD0_SHIFT);
		ifc_out32(ctrl->rregs + FSL_IFC_ROW3, column);

		/*
		 * although currently it's 8 bytes for READID, we always read
		 * the maximum 256 bytes(for PARAM)
		 */
		ifc_out32(ctrl->rregs + FSL_IFC_NAND_BC, len);
		ctrl->read_bytes = len;

		set_addr(mtd, 0, 0, 0);
		fsl_ifc_run_command(mtd);
		return;
	}

	/* ERASE1 stores the block and page address */
	case NAND_CMD_ERASE1:
		set_addr(mtd, 0, page_addr, 0);
		return;

	/* ERASE2 uses the block and page address from ERASE1 */
	case NAND_CMD_ERASE2:
		ifc_out32(ctrl->rregs + FSL_IFC_FIR0,
			  (IFC_FIR_OP_CW0 << IFC_NAND_FIR0_OP0_SHIFT) |
			  (IFC_FIR_OP_RA0 << IFC_NAND_FIR0_OP1_SHIFT) |
			  (IFC_FIR_OP_CMD1 << IFC_NAND_FIR0_OP2_SHIFT));

		ifc_out32(ctrl->rregs + FSL_IFC_FCR0,
			  (NAND_CMD_ERASE1 << IFC_NAND_FCR0_CMD0_SHIFT) |
			  (NAND_CMD_ERASE2 << IFC_NAND_FCR0_CMD1_SHIFT));

		ifc_out32(ctrl->rregs + FSL_IFC_NAND_BC, 0);
		ctrl->read_bytes = 0;
		fsl_ifc_run_command(mtd);
		return;

	/* SEQIN sets up the addr buffer and all registers except the length */
	case NAND_CMD_SEQIN: {
		uint32_t nand_fcr0;

		ctrl->column = column;
		ctrl->oob = 0;

		if (mtd->writesize > 512) {
			nand_fcr0 =
				(NAND_CMD_SEQIN << IFC_NAND_FCR0_CMD0_SHIFT) |
				(NAND_CMD_STATUS << IFC_NAND_FCR0_CMD1_SHIFT) |
				(NAND_CMD_PAGEPROG << IFC_NAND_FCR0_CMD2_SHIFT);

			ifc_out32(ctrl->rregs + FSL_IFC_FIR0,
				  (IFC_FIR_OP_CW0 << IFC_NAND_FIR0_OP0_SHIFT) |
				  (IFC_FIR_OP_CA0 << IFC_NAND_FIR0_OP1_SHIFT) |
				  (IFC_FIR_OP_RA0 << IFC_NAND_FIR0_OP2_SHIFT) |
				  (IFC_FIR_OP_WBCD << IFC_NAND_FIR0_OP3_SHIFT) |
				  (IFC_FIR_OP_CMD2 << IFC_NAND_FIR0_OP4_SHIFT));
			ifc_out32(ctrl->rregs + FSL_IFC_FIR1,
				  (IFC_FIR_OP_CW1 << IFC_NAND_FIR1_OP5_SHIFT) |
				  (IFC_FIR_OP_RDSTAT <<
					IFC_NAND_FIR1_OP6_SHIFT) |
				  (IFC_FIR_OP_NOP << IFC_NAND_FIR1_OP7_SHIFT));
		} else {
			nand_fcr0 = ((NAND_CMD_PAGEPROG <<
					IFC_NAND_FCR0_CMD1_SHIFT) |
				    (NAND_CMD_SEQIN <<
					IFC_NAND_FCR0_CMD2_SHIFT) |
				    (NAND_CMD_STATUS <<
					IFC_NAND_FCR0_CMD3_SHIFT));

			ifc_out32(ctrl->rregs + FSL_IFC_FIR0,
				  (IFC_FIR_OP_CW0 << IFC_NAND_FIR0_OP0_SHIFT) |
				  (IFC_FIR_OP_CMD2 << IFC_NAND_FIR0_OP1_SHIFT) |
				  (IFC_FIR_OP_CA0 << IFC_NAND_FIR0_OP2_SHIFT) |
				  (IFC_FIR_OP_RA0 << IFC_NAND_FIR0_OP3_SHIFT) |
				  (IFC_FIR_OP_WBCD << IFC_NAND_FIR0_OP4_SHIFT));
			ifc_out32(ctrl->rregs + FSL_IFC_FIR1,
				  (IFC_FIR_OP_CMD1 << IFC_NAND_FIR1_OP5_SHIFT) |
				  (IFC_FIR_OP_CW3 << IFC_NAND_FIR1_OP6_SHIFT) |
				  (IFC_FIR_OP_RDSTAT <<
					IFC_NAND_FIR1_OP7_SHIFT) |
				  (IFC_FIR_OP_NOP << IFC_NAND_FIR1_OP8_SHIFT));

			if (column >= mtd->writesize)
				nand_fcr0 |=
				NAND_CMD_READOOB << IFC_NAND_FCR0_CMD0_SHIFT;
			else
				nand_fcr0 |=
				NAND_CMD_READ0 << IFC_NAND_FCR0_CMD0_SHIFT;
		}

		if (column >= mtd->writesize) {
			/* OOB area --> READOOB */
			column -= mtd->writesize;
			ctrl->oob = 1;
		}
		ifc_out32(ctrl->rregs + FSL_IFC_FCR0, nand_fcr0);
		set_addr(mtd, column, page_addr, ctrl->oob);
		return;
	}

	/* PAGEPROG reuses all of the setup from SEQIN and adds the length */
	case NAND_CMD_PAGEPROG:
		if (ctrl->oob)
			ifc_out32(ctrl->rregs + FSL_IFC_NAND_BC,
				  ctrl->index - ctrl->column);
		else
			ifc_out32(ctrl->rregs + FSL_IFC_NAND_BC, 0);

		fsl_ifc_run_command(mtd);
		return;

	case NAND_CMD_STATUS:
		ifc_out32(ctrl->rregs + FSL_IFC_FIR0,
			  (IFC_FIR_OP_CW0 << IFC_NAND_FIR0_OP0_SHIFT) |
			  (IFC_FIR_OP_RB << IFC_NAND_FIR0_OP1_SHIFT));
		ifc_out32(ctrl->rregs + FSL_IFC_FCR0,
			  NAND_CMD_STATUS << IFC_NAND_FCR0_CMD0_SHIFT);
		ifc_out32(ctrl->rregs + FSL_IFC_NAND_BC, 1);
		set_addr(mtd, 0, 0, 0);
		ctrl->read_bytes = 1;

		fsl_ifc_run_command(mtd);

		/*
		 * The chip always seems to report that it is
		 * write-protected, even when it is not.
		 */
		if (chip->options & NAND_BUSWIDTH_16)
			out_be16(ctrl->addr, in_be16(ctrl->addr) |
					NAND_STATUS_WP);
		else
			out_8(ctrl->addr, in_8(ctrl->addr) | NAND_STATUS_WP);
		return;

	case NAND_CMD_RESET:
		ifc_out32(ctrl->rregs + FSL_IFC_FIR0,
			  IFC_FIR_OP_CW0 << IFC_NAND_FIR0_OP0_SHIFT);
		ifc_out32(ctrl->rregs + FSL_IFC_FCR0,
			  NAND_CMD_RESET << IFC_NAND_FCR0_CMD0_SHIFT);
		fsl_ifc_run_command(mtd);
		return;

	default:
		pr_err("%s: error, unsupported command 0x%x.\n",
			__func__, command);
	}
}

/* Write buf to the IFC NAND Controller Data Buffer */
static void fsl_ifc_write_buf(struct nand_chip *chip, const uint8_t *buf, int len)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct fsl_ifc_mtd *priv = chip->priv;
	struct fsl_ifc_ctrl *ctrl = priv->ctrl;
	uint32_t bufsize = mtd->writesize + mtd->oobsize;

	if (len <= 0) {
		pr_info("%s of %d bytes", __func__, len);
		ctrl->nand_stat = 0;
		return;
	}

	if ((uint32_t)len > bufsize - ctrl->index) {
		pr_err("%s beyond end of buffer (%d requested, %u available)\n",
			__func__, len, bufsize - ctrl->index);
		len = bufsize - ctrl->index;
	}

	memcpy_toio(ctrl->addr + ctrl->index, buf, len);
	ctrl->index += len;
}

/*
 * read a byte from either the IFC hardware buffer if it has any data left
 * otherwise issue a command to read a single byte.
 */
static uint8_t fsl_ifc_read_byte(struct nand_chip *chip)
{
	struct fsl_ifc_mtd *priv = chip->priv;
	struct fsl_ifc_ctrl *ctrl = priv->ctrl;
	uint32_t offset;

	/*
	 * If there are still bytes in the IFC buffer, then use the
	 * next byte.
	 */
	if (ctrl->index < ctrl->read_bytes) {
		offset = ctrl->index++;
		return in_8(ctrl->addr + offset);
	}

	return ERR_BYTE;
}

/*
 * Read two bytes from the IFC hardware buffer
 * read function for 16-bit buswith
 */
static uint8_t fsl_ifc_read_byte16(struct nand_chip *chip)
{
	struct fsl_ifc_mtd *priv = chip->priv;
	struct fsl_ifc_ctrl *ctrl = priv->ctrl;
	uint16_t data;

	/*
	 * If there are still bytes in the IFC buffer, then use the
	 * next byte.
	 */
	if (ctrl->index < ctrl->read_bytes) {
		data = ifc_in16(ctrl->addr + ctrl->index);
		ctrl->index += 2;
		return (uint8_t)data;
	}

	return ERR_BYTE;
}

/* Read from the IFC Controller Data Buffer */
static void fsl_ifc_read_buf(struct nand_chip *chip, uint8_t *buf, int len)
{
	struct fsl_ifc_mtd *priv = chip->priv;
	struct fsl_ifc_ctrl *ctrl = priv->ctrl;
	int avail;

	if (len < 0)
		return;

	avail = min((uint32_t)len, ctrl->read_bytes - ctrl->index);
	memcpy_fromio(buf, ctrl->addr + ctrl->index, avail);

	ctrl->index += avail;

	if (len > avail)
		pr_err("%s beyond end of buffer (%d requested, %d available)\n",
		       __func__, len, avail);
}

/* This function is called after Program and Erase Operations to
 * check for success or failure.
 */
static int fsl_ifc_wait(struct nand_chip *chip)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct fsl_ifc_mtd *priv = chip->priv;
	struct fsl_ifc_ctrl *ctrl = priv->ctrl;
	uint32_t nand_fsr;
	int status;

	/* Use READ_STATUS command, but wait for the device to be ready */
	ifc_out32(ctrl->rregs + FSL_IFC_FIR0,
		  (IFC_FIR_OP_CW0 << IFC_NAND_FIR0_OP0_SHIFT) |
		  (IFC_FIR_OP_RDSTAT << IFC_NAND_FIR0_OP1_SHIFT));
	ifc_out32(ctrl->rregs + FSL_IFC_FCR0, NAND_CMD_STATUS <<
		  IFC_NAND_FCR0_CMD0_SHIFT);
	ifc_out32(ctrl->rregs + FSL_IFC_NAND_BC, 1);
	set_addr(mtd, 0, 0, 0);
	ctrl->read_bytes = 1;

	fsl_ifc_run_command(mtd);

	nand_fsr = ifc_in32(ctrl->rregs + FSL_IFC_NAND_FSR);
	status = nand_fsr >> 24;

	/* Chip sometimes reporting write protect even when it's not */
	return status | NAND_STATUS_WP;
}

/*
 * The controller does not check for bitflips in erased pages,
 * therefore software must check instead.
 */
static int
check_erased_page(struct nand_chip *chip, u8 *buf, struct mtd_info *mtd)
{
	u8 *ecc = chip->oob_poi;
	const int ecc_size = chip->ecc.bytes;
	const int pkt_size = chip->ecc.size;
	int i, res, bitflips = 0;
	struct mtd_oob_region oobregion = { };


	mtd_ooblayout_ecc(mtd, 0, &oobregion);
	ecc += oobregion.offset;
	for (i = 0; i < chip->ecc.steps; i++) {
		res = nand_check_erased_ecc_chunk(buf, pkt_size, ecc, ecc_size,
		NULL, 0, chip->ecc.strength);

		if (res < 0) {
			pr_err("fsl-ifc: NAND Flash ECC Uncorrectable Error\n");
			mtd->ecc_stats.failed++;
		} else if (res > 0) {
			mtd->ecc_stats.corrected += res;
		}
		bitflips = max(res, bitflips);
		buf += pkt_size;
		ecc += ecc_size;
	}

	return bitflips;
}

static int fsl_ifc_read_page(struct nand_chip *chip, uint8_t *buf,
		int oob_required, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);
	struct fsl_ifc_mtd *priv = chip->priv;
	struct fsl_ifc_ctrl *ctrl = priv->ctrl;

	nand_read_page_op(chip, page, 0, buf, mtd->writesize);
	/*fsl_ifc_read_buf(chip, buf, mtd->writesize); */
	if (oob_required)
		fsl_ifc_read_buf(chip, chip->oob_poi, mtd->oobsize);

	if (ctrl->nand_stat & IFC_NAND_EVTER_STAT_ECCER) {
		if (!oob_required)
			fsl_ifc_read_buf(chip, chip->oob_poi, mtd->oobsize);

		return check_erased_page(chip, buf, mtd);
	}

	if (ctrl->nand_stat != IFC_NAND_EVTER_STAT_OPC)
		mtd->ecc_stats.failed++;

	return ctrl->max_bitflips;
}

/*
 * ECC will be calculated automatically, and errors will be detected in
 * waitfunc.
 */
static int fsl_ifc_write_page(struct nand_chip *chip, const uint8_t *buf,
		int oob_required, int page)
{
	struct mtd_info *mtd = nand_to_mtd(chip);

	nand_prog_page_begin_op(chip, page, 0, buf, mtd->writesize);
	fsl_ifc_write_buf(chip, chip->oob_poi, mtd->oobsize);

	return nand_prog_page_end_op(chip);
}

static int match_bank(struct fsl_ifc_ctrl *ctrl, int bank, phys_addr_t addr)
{
	u32 cspr = get_ifc_cspr(ctrl->gregs, bank);

	if (!(cspr & CSPR_V))
		return 0;
	if ((cspr & CSPR_MSEL) != CSPR_MSEL_NAND)
		return 0;

	return (cspr & CSPR_BA) == (addr & CSPR_BA);
}

static int fsl_ifc_ctrl_init(void)
{
	struct fsl_ifc_ctrl *ctrl;
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "fsl,ifc");
	if (!np)
		return -EINVAL;

	ifc_ctrl = kzalloc(sizeof(*ifc_ctrl), GFP_KERNEL);
	if (!ifc_ctrl)
		return -ENOMEM;

	ctrl = ifc_ctrl;
	ctrl->read_bytes = 0;
	ctrl->index = 0;
	ctrl->addr = NULL;

	ctrl->gregs = of_iomap(np, 0);

	ctrl->version = ifc_in32(ctrl->gregs + FSL_IFC_REV);
	if (ctrl->version >= FSL_IFC_V2_0_0)
		ctrl->rregs = ctrl->gregs + 0x10000;
	else
		ctrl->rregs = ctrl->gregs + 0x1000;

	/* clear event registers */
	ifc_out32(ctrl->rregs + FSL_IFC_NAND_EVTER_STAT, ~0U);
	ifc_out32(ctrl->rregs + FSL_IFC_PGRDCMPL_EVT_STAT, ~0U);

	/* Enable error and event for any detected errors */
	ifc_out32(ctrl->rregs + FSL_IFC_EVTER_EN,
		  IFC_NAND_EVTER_EN_OPC_EN |
		  IFC_NAND_EVTER_EN_PGRDCMPL_EN |
		  IFC_NAND_EVTER_EN_FTOER_EN |
		  IFC_NAND_EVTER_EN_WPER_EN);

	ifc_out32(ctrl->rregs + FSL_IFC_NCFGR, 0x0);

	return 0;
}

static void fsl_ifc_select_chip(struct nand_chip *chip, int cs)
{
}

static int fsl_ifc_sram_init(struct fsl_ifc_mtd *priv, uint32_t ver)
{
	struct fsl_ifc_ctrl *ctrl = priv->ctrl;
	uint32_t cs = 0, csor = 0, csor_8k = 0, csor_ext = 0;
	uint32_t ncfgr = 0;
	uint32_t time_start;

	if (ctrl->version > FSL_IFC_V1_1_0) {
		ncfgr = ifc_in32(ctrl->rregs + FSL_IFC_NCFGR);
		ifc_out32(ctrl->rregs + FSL_IFC_NCFGR,
				ncfgr | IFC_NAND_SRAM_INIT_EN);

		/* wait for  SRAM_INIT bit to be clear or timeout */
		time_start = get_time_ns();
		while (!is_timeout(time_start, IFC_TIMEOUT_MS * MSECOND)) {
			ifc_ctrl->nand_stat =
				ifc_in32(ctrl->rregs + FSL_IFC_NAND_EVTER_STAT);

			if (!(ifc_ctrl->nand_stat & IFC_NAND_SRAM_INIT_EN))
				return 0;
		}
		pr_err("fsl-ifc: Failed to Initialise SRAM\n");
		return -EIO;
	}

	cs = priv->cs;
	/* Save CSOR and CSOR_ext */
	csor = get_ifc_csor(ctrl->gregs, cs);
	csor_ext = get_ifc_csor_ext(ctrl->gregs, cs);

	/* change PageSize 8K and SpareSize 1K*/
	csor_8k = (csor & ~(CSOR_NAND_PGS_MASK)) | 0x0018C000;
	set_ifc_csor(ctrl->gregs, cs, csor_8k);
	set_ifc_csor_ext(ctrl->gregs, cs, 0x0000400);

	/* READID */
	ifc_out32(ctrl->rregs + FSL_IFC_FIR0,
		  (IFC_FIR_OP_CW0 << IFC_NAND_FIR0_OP0_SHIFT) |
		  (IFC_FIR_OP_UA  << IFC_NAND_FIR0_OP1_SHIFT) |
		  (IFC_FIR_OP_RB << IFC_NAND_FIR0_OP2_SHIFT));
	ifc_out32(ctrl->rregs + FSL_IFC_FCR0,
		  NAND_CMD_READID << IFC_NAND_FCR0_CMD0_SHIFT);
	ifc_out32(ctrl->rregs + FSL_IFC_ROW3, 0x0);

	ifc_out32(ctrl->rregs + FSL_IFC_NAND_BC, 0x0);

	/* Program ROW0/COL0 */
	ifc_out32(ctrl->rregs + FSL_IFC_ROW0, 0x0);
	ifc_out32(ctrl->rregs + FSL_IFC_COL0, 0x0);

	/* set the chip select for NAND Transaction */
	ifc_out32(ctrl->rregs + FSL_IFC_NAND_CSEL,
			priv->cs << IFC_NAND_CSEL_SHIFT);

	/* start read seq */
	ifc_out32(ctrl->rregs + FSL_IFC_NANDSEQ_STRT,
			IFC_NAND_SEQ_STRT_FIR_STRT);

	time_start = get_time_ns();
	while (!is_timeout(time_start, IFC_TIMEOUT_MS * MSECOND)) {
		ifc_ctrl->nand_stat =
			ifc_in32(ctrl->rregs + FSL_IFC_NAND_EVTER_STAT);

		if (ifc_ctrl->nand_stat & IFC_NAND_EVTER_STAT_OPC)
			break;
	}

	if (ifc_ctrl->nand_stat != IFC_NAND_EVTER_STAT_OPC) {
		pr_err("fsl-ifc: Failed to Initialise SRAM\n");
		return -EIO;
	}

	ifc_out32(ctrl->rregs + FSL_IFC_NAND_EVTER_STAT, ifc_ctrl->nand_stat);

	/* Restore CSOR and CSOR_ext */
	set_ifc_csor(ctrl->gregs, priv->cs, csor);
	set_ifc_csor_ext(ctrl->gregs, priv->cs, csor_ext);

	return 0;
}

static int fsl_ifc_chip_init(struct fsl_ifc_mtd *priv)
{
	struct fsl_ifc_ctrl *ctrl;
	struct nand_chip *nand = &priv->chip;
	struct mtd_info *mtd = nand_to_mtd(&priv->chip);
	uint32_t cspr = 0, csor = 0;
	int ret = 0;

	if (!ifc_ctrl) {
		ret = fsl_ifc_ctrl_init();
		if (ret)
			return ret;
	}
	ctrl = priv->ctrl = ifc_ctrl;

	if (priv->dev->of_node) {
		int bank, banks;

		 /* find which chip select it is connected to */
		banks = (ctrl->version == FSL_IFC_V1_1_0) ? 4 : 8;
		for (bank = 0; bank < banks; bank++) {
			if (match_bank(ifc_ctrl, bank, priv->pbase))
				break;
		}
		priv->cs = bank;
		if (bank >= banks) {
			pr_err("%s: address did not match any chip selects\n",
				__func__);
			return -ENODEV;
		}
	}

	/*mtd->priv = nand; */
	mtd->dev.parent = priv->dev;

	/*
	 * Fill in nand_chip structure
	 * set up function call table
	 */
	nand->legacy.write_buf = fsl_ifc_write_buf;
	nand->legacy.read_buf = fsl_ifc_read_buf;
	nand->legacy.select_chip = fsl_ifc_select_chip;
	nand->legacy.cmdfunc = fsl_ifc_cmdfunc;
	nand->legacy.waitfunc = fsl_ifc_wait;

	/* set up nand options */
	nand->bbt_td = &bbt_main_descr;
	nand->bbt_md = &bbt_mirror_descr;

	/* set up nand options */
	nand->options = NAND_NO_SUBPAGE_WRITE;
	nand->bbt_options = NAND_BBT_USE_FLASH;

	cspr = get_ifc_cspr(ctrl->gregs, priv->cs);
	csor = get_ifc_csor(ctrl->gregs, priv->cs);

	if (cspr & CSPR_PORT_SIZE_16) {
		nand->legacy.read_byte = fsl_ifc_read_byte16;
		nand->options |= NAND_BUSWIDTH_16;
	} else {
		nand->legacy.read_byte = fsl_ifc_read_byte;
	}

	nand->controller = &ifc_ctrl->controller;
	nand->priv = priv;

	nand->ecc.read_page = fsl_ifc_read_page;
	nand->ecc.write_page = fsl_ifc_write_page;

	/* Hardware generates ECC per 512 Bytes */
	nand->ecc.size = 512;
	nand->ecc.bytes = 8;

	nand->legacy.chip_delay = 30;

	switch (csor & CSOR_NAND_PGS_MASK) {
	case CSOR_NAND_PGS_512:
		if (!(nand->options & NAND_BUSWIDTH_16)) {
			/* Avoid conflict with bad block marker */
			bbt_main_descr.offs = 0;
			bbt_mirror_descr.offs = 0;
		}

		nand->ecc.strength = 4;
		priv->bufnum_mask = 15;
		break;

	case CSOR_NAND_PGS_2K:
		nand->ecc.strength = 4;
		priv->bufnum_mask = 3;
		break;

	case CSOR_NAND_PGS_4K:
		if ((csor & CSOR_NAND_ECC_MODE_MASK) ==
		    CSOR_NAND_ECC_MODE_4) {
			nand->ecc.strength = 4;
		} else {
			nand->ecc.strength = 8;
			nand->ecc.bytes = 16;
		}

		priv->bufnum_mask = 1;
		break;

	case CSOR_NAND_PGS_8K:
		if ((csor & CSOR_NAND_ECC_MODE_MASK) ==
		    CSOR_NAND_ECC_MODE_4) {
			nand->ecc.strength = 4;
		} else {
			nand->ecc.strength = 8;
			nand->ecc.bytes = 16;
		}

		priv->bufnum_mask = 0;
		break;


	default:
		pr_err("ifc nand: bad csor %#x: bad page size\n", csor);
		return -ENODEV;
	}

	/* Must also set CSOR_NAND_ECC_ENC_EN if DEC_EN set */
	if (csor & CSOR_NAND_ECC_DEC_EN) {
		nand->ecc.mode = NAND_ECC_HW;
		mtd_set_ooblayout(mtd, &fsl_ifc_ooblayout_ops);
	} else {
		nand->ecc.mode = NAND_ECC_SOFT;
		nand->ecc.algo = NAND_ECC_ALGO_HAMMING;
	}

	if (ctrl->version >= FSL_IFC_V1_1_0) {
		ret = fsl_ifc_sram_init(priv, ctrl->version);
		if (ret)
			return ret;
	}

	if (ctrl->version >= FSL_IFC_V2_0_0)
		priv->bufnum_mask = (priv->bufnum_mask * 2) + 1;

	return 0;
}

static int fsl_ifc_nand_probe(struct device *dev)
{
	struct fsl_ifc_mtd *priv;
	struct resource *iores;
	struct mtd_info *mtd;
	int ret = 0;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = -ENOMEM;
		goto bailout;
	}
	priv->pbase = iores->start;
	priv->vbase = IOMEM(iores->start);

	if (fsl_ifc_chip_init(priv)) {
		ret = -ENOMEM;
		goto bailout;
	}

	ret = nand_scan_ident(&priv->chip, 1, NULL);
	if (ret)
		goto bailout;

	ret = nand_scan_tail(&priv->chip);
	if (ret)
		goto bailout;

	mtd = nand_to_mtd(&priv->chip);
	return add_mtd_nand_device(mtd, "nand");
bailout:
	kfree(priv);
	return ret;
}

static __maybe_unused struct of_device_id fsl_nand_compatible[] = {
	{
		.compatible = "fsl,ifc-nand",
	}, {
	}
};
MODULE_DEVICE_TABLE(of, fsl_nand_compatible);

static struct driver fsl_ifc_driver = {
	.name = "fsl_nand",
	.probe = fsl_ifc_nand_probe,
	.of_compatible = DRV_OF_COMPAT(fsl_nand_compatible),
};
device_platform_driver(fsl_ifc_driver);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("FSL IFC NAND driver");
MODULE_LICENSE("GPL");
