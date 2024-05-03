// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2009-2012 Freescale Semiconductor, Inc.
 *	Dave Liu <daveliu@freescale.com>
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <of_net.h>
#include <dma.h>
#include <firmware.h>
#include <soc/fsl/qe.h>
#include <soc/fsl/fsl_fman.h>
#include <soc/fsl/fsl_memac.h>

/* Port ID */
#define OH_PORT_ID_BASE		0x01
#define MAX_NUM_OH_PORT		7
#define RX_PORT_1G_BASE		0x08
#define MAX_NUM_RX_PORT_1G	8
#define RX_PORT_10G_BASE	0x10
#define RX_PORT_10G_BASE2	0x08
#define TX_PORT_1G_BASE		0x28
#define MAX_NUM_TX_PORT_1G	8
#define TX_PORT_10G_BASE	0x30
#define TX_PORT_10G_BASE2	0x28
#define MIIM_TIMEOUT    0xFFFF

struct fm_muram {
	void *base;
	void *top;
	size_t size;
	void *alloc;
};
#define FM_MURAM_RES_SIZE	0x01000

/* Rx/Tx buffer descriptor */
struct fm_port_bd {
	u16 status;
	u16 len;
	u32 res0;
	u16 res1;
	u16 buf_ptr_hi;
	u32 buf_ptr_lo;
};

/* Common BD flags */
#define BD_LAST			0x0800

/* Rx BD status flags */
#define RxBD_EMPTY		0x8000
#define RxBD_LAST		BD_LAST
#define RxBD_FIRST		0x0400
#define RxBD_PHYS_ERR		0x0008
#define RxBD_SIZE_ERR		0x0004
#define RxBD_ERROR		(RxBD_PHYS_ERR | RxBD_SIZE_ERR)

/* Tx BD status flags */
#define TxBD_READY		0x8000
#define TxBD_LAST		BD_LAST

/* Rx/Tx queue descriptor */
struct fm_port_qd {
	u16 gen;
	u16 bd_ring_base_hi;
	u32 bd_ring_base_lo;
	u16 bd_ring_size;
	u16 offset_in;
	u16 offset_out;
	u16 res0;
	u32 res1[0x4];
};

/* IM global parameter RAM */
struct fm_port_global_pram {
	u32 mode;	/* independent mode register */
	u32 rxqd_ptr;	/* Rx queue descriptor pointer */
	u32 txqd_ptr;	/* Tx queue descriptor pointer */
	u16 mrblr;	/* max Rx buffer length */
	u16 rxqd_bsy_cnt;	/* RxQD busy counter, should be cleared */
	u32 res0[0x4];
	struct fm_port_qd rxqd;	/* Rx queue descriptor */
	struct fm_port_qd txqd;	/* Tx queue descriptor */
	u32 res1[0x28];
};

#define FM_PRAM_SIZE		sizeof(struct fm_port_global_pram)
#define FM_PRAM_ALIGN		256
#define PRAM_MODE_GLOBAL	0x20000000
#define PRAM_MODE_GRACEFUL_STOP	0x00800000

#define FM_FREE_POOL_SIZE	0x20000 /* 128K bytes */
#define FM_FREE_POOL_ALIGN	256

/* Fman ethernet private struct */
struct fm_eth {
	struct fm_bmi_tx_port *tx_port;
	struct fm_bmi_rx_port *rx_port;
	phy_interface_t enet_if;
	struct eth_device edev;
	struct device *dev;
	struct fm_port_global_pram *rx_pram; /* Rx parameter table */
	struct fm_port_global_pram *tx_pram; /* Tx parameter table */
	struct fm_port_bd *rx_bd_ring;	/* Rx BD ring base */
	int cur_rxbd_idx;		/* current Rx BD index */
	void *rx_buf;			/* Rx buffer base */
	struct fm_port_bd *tx_bd_ring;	/* Tx BD ring base */
	int cur_txbd_idx;		/* current Tx BD index */
	struct memac *regs;
};

#define RX_BD_RING_SIZE		8
#define TX_BD_RING_SIZE		8
#define MAX_RXBUF_LOG2		11
#define MAX_RXBUF_LEN		(1 << MAX_RXBUF_LOG2)

struct fsl_fman_mdio {
	struct mii_bus bus;
	struct memac_mdio_controller *regs;
};

enum fman_port_type {
	FMAN_PORT_TYPE_RX = 0,	/* RX Port */
	FMAN_PORT_TYPE_TX,	/* TX Port */
};

struct fsl_fman_port {
	void *regs;
	enum fman_port_type type;
	struct fm_bmi_rx_port *rxport;
	struct fm_bmi_tx_port *txport;
};

static struct fm_eth *to_fm_eth(struct eth_device *edev)
{
	return container_of(edev, struct fm_eth, edev);
}

static struct fm_muram muram;

static void *fm_muram_base(void)
{
	return muram.base;
}

static void *fm_muram_alloc(size_t size, unsigned long align)
{
	void *ret;
	unsigned long align_mask;
	size_t off;
	void *save;

	align_mask = align - 1;
	save = muram.alloc;

	off = (unsigned long)save & align_mask;
	if (off != 0)
		muram.alloc += (align - off);
	off = size & align_mask;
	if (off != 0)
		size += (align - off);
	if ((muram.alloc + size) >= muram.top) {
		muram.alloc = save;
		printf("%s: run out of ram.\n", __func__);
		return NULL;
	}

	ret = muram.alloc;
	muram.alloc += size;

	return ret;
}

/*
 * fm_upload_ucode - Fman microcode upload worker function
 *
 * This function does the actual uploading of an Fman microcode
 * to an Fman.
 */
static int fm_upload_ucode(struct fm_imem *imem,
			    u32 *ucode, unsigned int size)
{
	unsigned int i;
	unsigned int timeout = 1000000;

	/* enable address auto increase */
	out_be32(&imem->iadd, IRAM_IADD_AIE);
	/* write microcode to IRAM */
	for (i = 0; i < size / 4; i++)
		out_be32(&imem->idata, (be32_to_cpu(ucode[i])));

	/* verify if the writing is over */
	out_be32(&imem->iadd, 0);
	while ((in_be32(&imem->idata) != be32_to_cpu(ucode[0])) && --timeout)
		;
	if (!timeout) {
		printf("microcode upload timeout\n");
		return -ETIMEDOUT;
	}

	/* enable microcode from IRAM */
	out_be32(&imem->iready, IRAM_READY);

	return 0;
}

static int fman_upload_firmware(struct device *dev, struct fm_imem *fm_imem)
{
	int i, size, ret;
	const struct qe_firmware *firmware;

	get_builtin_firmware(fsl_fman_ucode_ls1046_r1_0_106_4_18_bin, &firmware, &size);
	if (!size) {
		dev_err(dev, "FMan Firmware was not included in build\n");
		return -ENOSYS;
	}

	ret = qe_validate_firmware(firmware, size);
	if (ret)
		return ret;

	if (firmware->count != 1) {
		dev_err(dev, "Invalid data in firmware header\n");
		return -EINVAL;
	}

	/* Loop through each microcode. */
	for (i = 0; i < firmware->count; i++) {
		const struct qe_microcode *ucode = &firmware->microcode[i];

		/* Upload a microcode if it's present */
		if (be32_to_cpu(ucode->code_offset)) {
			u32 ucode_size;
			u32 *code;
			dev_info(dev, "Uploading microcode version %u.%u.%u\n",
			       ucode->major, ucode->minor,
			       ucode->revision);
			code = (void *)firmware +
			       be32_to_cpu(ucode->code_offset);
			ucode_size = sizeof(u32) * be32_to_cpu(ucode->count);
			ret = fm_upload_ucode(fm_imem, code, ucode_size);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static u32 fm_assign_risc(int port_id)
{
	u32 risc_sel, val;
	risc_sel = (port_id & 0x1) ? FMFPPRC_RISC2 : FMFPPRC_RISC1;
	val = (port_id << FMFPPRC_PORTID_SHIFT) & FMFPPRC_PORTID_MASK;
	val |= ((risc_sel << FMFPPRC_ORA_SHIFT) | risc_sel);

	return val;
}

static void fm_init_fpm(struct fm_fpm *fpm)
{
	int i, port_id;
	u32 val;

	setbits_be32(&fpm->fmfpee, FMFPEE_EHM | FMFPEE_UEC |
				   FMFPEE_CER | FMFPEE_DER);

	/* IM mode, each even port ID to RISC#1, each odd port ID to RISC#2 */

	/* offline/parser port */
	for (i = 0; i < MAX_NUM_OH_PORT; i++) {
		port_id = OH_PORT_ID_BASE + i;
		val = fm_assign_risc(port_id);
		out_be32(&fpm->fpmprc, val);
	}
	/* Rx 1G port */
	for (i = 0; i < MAX_NUM_RX_PORT_1G; i++) {
		port_id = RX_PORT_1G_BASE + i;
		val = fm_assign_risc(port_id);
		out_be32(&fpm->fpmprc, val);
	}
	/* Tx 1G port */
	for (i = 0; i < MAX_NUM_TX_PORT_1G; i++) {
		port_id = TX_PORT_1G_BASE + i;
		val = fm_assign_risc(port_id);
		out_be32(&fpm->fpmprc, val);
	}
	/* Rx 10G port */
	port_id = RX_PORT_10G_BASE;
	val = fm_assign_risc(port_id);
	out_be32(&fpm->fpmprc, val);
	/* Tx 10G port */
	port_id = TX_PORT_10G_BASE;
	val = fm_assign_risc(port_id);
	out_be32(&fpm->fpmprc, val);

	/* disable the dispatch limit in IM case */
	out_be32(&fpm->fpmflc, FMFP_FLC_DISP_LIM_NONE);
	/* clear events */
	out_be32(&fpm->fmfpee, FMFPEE_CLEAR_EVENT);

	/* clear risc events */
	for (i = 0; i < 4; i++)
		out_be32(&fpm->fpmcev[i], 0xffffffff);

	/* clear error */
	out_be32(&fpm->fpmrcr, FMFP_RCR_MDEC | FMFP_RCR_IDEC);
}

static int fm_init_bmi(struct fm_bmi_common *bmi)
{
	int blk, i, port_id;
	u32 val;
	size_t offset;
	void *base;

	/* alloc free buffer pool in MURAM */
	base = fm_muram_alloc(FM_FREE_POOL_SIZE, FM_FREE_POOL_ALIGN);
	if (!base) {
		printf("%s: no muram for free buffer pool\n", __func__);
		return -ENOMEM;
	}
	offset = base - fm_muram_base();

	/* Need 128KB total free buffer pool size */
	val = offset / 256;
	blk = FM_FREE_POOL_SIZE / 256;
	/* in IM, we must not begin from offset 0 in MURAM */
	val |= ((blk - 1) << FMBM_CFG1_FBPS_SHIFT);
	out_be32(&bmi->fmbm_cfg1, val);

	/* disable all BMI interrupt */
	out_be32(&bmi->fmbm_ier, FMBM_IER_DISABLE_ALL);

	/* clear all events */
	out_be32(&bmi->fmbm_ievr, FMBM_IEVR_CLEAR_ALL);

	/*
	 * set port parameters - FMBM_PP_x
	 * max tasks 10G Rx/Tx=12, 1G Rx/Tx 4, others is 1
	 * max dma 10G Rx/Tx=3, others is 1
	 * set port FIFO size - FMBM_PFS_x
	 * 4KB for all Rx and Tx ports
	 */
	/* offline/parser port */
	for (i = 0; i < MAX_NUM_OH_PORT; i++) {
		port_id = OH_PORT_ID_BASE + i - 1;
		/* max tasks=1, max dma=1, no extra */
		out_be32(&bmi->fmbm_pp[port_id], 0);
		/* port FIFO size - 256 bytes, no extra */
		out_be32(&bmi->fmbm_pfs[port_id], 0);
	}
	/* Rx 1G port */
	for (i = 0; i < MAX_NUM_RX_PORT_1G; i++) {
		port_id = RX_PORT_1G_BASE + i - 1;
		/* max tasks=4, max dma=1, no extra */
		out_be32(&bmi->fmbm_pp[port_id], FMBM_PP_MXT(4));
		/* FIFO size - 4KB, no extra */
		out_be32(&bmi->fmbm_pfs[port_id], FMBM_PFS_IFSZ(0xf));
	}
	/* Tx 1G port FIFO size - 4KB, no extra */
	for (i = 0; i < MAX_NUM_TX_PORT_1G; i++) {
		port_id = TX_PORT_1G_BASE + i - 1;
		/* max tasks=4, max dma=1, no extra */
		out_be32(&bmi->fmbm_pp[port_id], FMBM_PP_MXT(4));
		/* FIFO size - 4KB, no extra */
		out_be32(&bmi->fmbm_pfs[port_id], FMBM_PFS_IFSZ(0xf));
	}
	/* Rx 10G port */
	port_id = RX_PORT_10G_BASE - 1;
	/* max tasks=12, max dma=3, no extra */
	out_be32(&bmi->fmbm_pp[port_id], FMBM_PP_MXT(12) | FMBM_PP_MXD(3));
	/* FIFO size - 4KB, no extra */
	out_be32(&bmi->fmbm_pfs[port_id], FMBM_PFS_IFSZ(0xf));

	/* Tx 10G port */
	port_id = TX_PORT_10G_BASE - 1;
	/* max tasks=12, max dma=3, no extra */
	out_be32(&bmi->fmbm_pp[port_id], FMBM_PP_MXT(12) | FMBM_PP_MXD(3));
	/* FIFO size - 4KB, no extra */
	out_be32(&bmi->fmbm_pfs[port_id], FMBM_PFS_IFSZ(0xf));

	/* initialize internal buffers data base (linked list) */
	out_be32(&bmi->fmbm_init, FMBM_INIT_START);

	return 0;
}

static void fm_init_qmi(struct fm_qmi_common *qmi)
{
	/* disable all error interrupts */
	out_be32(&qmi->fmqm_eien, FMQM_EIEN_DISABLE_ALL);
	/* clear all error events */
	out_be32(&qmi->fmqm_eie, FMQM_EIE_CLEAR_ALL);

	/* disable all interrupts */
	out_be32(&qmi->fmqm_ien, FMQM_IEN_DISABLE_ALL);
	/* clear all interrupts */
	out_be32(&qmi->fmqm_ie, FMQM_IE_CLEAR_ALL);
}

static int fm_init_common(struct device *dev, struct ccsr_fman *reg)
{
	int ret;

	/* Upload the Fman microcode if it's present */
	ret = fman_upload_firmware(dev, &reg->fm_imem);
	if (ret)
		return ret;

	fm_init_qmi(&reg->fm_qmi_common);
	fm_init_fpm(&reg->fm_fpm);

	/* clear DMA status */
	setbits_be32(&reg->fm_dma.fmdmsr, FMDMSR_CLEAR_ALL);

	/* set DMA mode */
	setbits_be32(&reg->fm_dma.fmdmmr, FMDMMR_SBER);

	return fm_init_bmi(&reg->fm_bmi_common);
}

#define memac_out_32(a, v)	out_be32(a, v)
#define memac_in_32(a)		in_be32(a)
#define memac_clrbits_32(a, v)	clrbits_be32(a, v)
#define memac_setbits_32(a, v)	setbits_be32(a, v)

static int memac_mdio_write(struct mii_bus *bus, int port_addr, int regnum, u16 value)
{
	struct fsl_fman_mdio *priv = container_of(bus, struct fsl_fman_mdio, bus);
	struct memac_mdio_controller *regs = priv->regs;
	u32 mdio_ctl;

	memac_clrbits_32(&regs->mdio_stat, MDIO_STAT_ENC);

	/* Wait till the bus is free */
	while ((memac_in_32(&regs->mdio_stat)) & MDIO_STAT_BSY)
		;

	/* Set the port and dev addr */
	mdio_ctl = MDIO_CTL_PORT_ADDR(port_addr) | MDIO_CTL_DEV_ADDR(regnum);
	memac_out_32(&regs->mdio_ctl, mdio_ctl);

	/* Wait till the bus is free */
	while ((memac_in_32(&regs->mdio_stat)) & MDIO_STAT_BSY)
		;

	/* Write the value to the register */
	memac_out_32(&regs->mdio_data, MDIO_DATA(value));

	/* Wait till the MDIO write is complete */
	while ((memac_in_32(&regs->mdio_data)) & MDIO_DATA_BSY)
		;

	return 0;
}

static int memac_mdio_read(struct mii_bus *bus, int port_addr, int regnum)
{
	struct fsl_fman_mdio *priv = container_of(bus, struct fsl_fman_mdio, bus);
	struct memac_mdio_controller *regs = priv->regs;
	u32 mdio_ctl;

	memac_clrbits_32(&regs->mdio_stat, MDIO_STAT_ENC);

	/* Wait till the bus is free */
	while ((memac_in_32(&regs->mdio_stat)) & MDIO_STAT_BSY)
		;

	/* Set the Port and Device Addrs */
	mdio_ctl = MDIO_CTL_PORT_ADDR(port_addr) | MDIO_CTL_DEV_ADDR(regnum);
	memac_out_32(&regs->mdio_ctl, mdio_ctl);

	/* Wait till the bus is free */
	while ((memac_in_32(&regs->mdio_stat)) & MDIO_STAT_BSY)
		;

	/* Initiate the read */
	mdio_ctl |= MDIO_CTL_READ;
	memac_out_32(&regs->mdio_ctl, mdio_ctl);

	/* Wait till the MDIO write is complete */
	while ((memac_in_32(&regs->mdio_data)) & MDIO_DATA_BSY)
		;

	/* Return all Fs if nothing was there */
	if (memac_in_32(&regs->mdio_stat) & MDIO_STAT_RD_ER)
		return 0xffff;

	return memac_in_32(&regs->mdio_data) & 0xffff;
}

static u16 muram_readw(u16 *addr)
{
	unsigned long base = (unsigned long)addr & ~0x3UL;
	u32 val32 = in_be32((void *)base);
	int byte_pos;
	u16 ret;

	byte_pos = (unsigned long)addr & 0x3UL;
	if (byte_pos)
		ret = (u16)(val32 & 0x0000ffff);
	else
		ret = (u16)((val32 & 0xffff0000) >> 16);

	return ret;
}

static void muram_writew(u16 *addr, u16 val)
{
	unsigned long base = (unsigned long)addr & ~0x3UL;
	u32 org32 = in_be32((void *)base);
	u32 val32;
	int byte_pos;

	byte_pos = (unsigned long)addr & 0x3UL;
	if (byte_pos)
		val32 = (org32 & 0xffff0000) | val;
	else
		val32 = (org32 & 0x0000ffff) | ((u32)val << 16);

	out_be32((void *)base, val32);
}

static void bmi_rx_port_disable(struct fm_bmi_rx_port *rx_port)
{
	int timeout = 1000000;

	clrbits_be32(&rx_port->fmbm_rcfg, FMBM_RCFG_EN);

	/* wait until the rx port is not busy */
	while ((in_be32(&rx_port->fmbm_rst) & FMBM_RST_BSY) && timeout--)
		;
}

static void bmi_rx_port_init(struct fm_bmi_rx_port *rx_port)
{
	/* set BMI to independent mode, Rx port disable */
	out_be32(&rx_port->fmbm_rcfg, FMBM_RCFG_IM);
	/* clear FOF in IM case */
	out_be32(&rx_port->fmbm_rim, 0);
	/* Rx frame next engine -RISC */
	out_be32(&rx_port->fmbm_rfne, NIA_ENG_RISC | NIA_RISC_AC_IM_RX);
	/* Rx command attribute - no order, MR[3] = 1 */
	clrbits_be32(&rx_port->fmbm_rfca, FMBM_RFCA_ORDER | FMBM_RFCA_MR_MASK);
	setbits_be32(&rx_port->fmbm_rfca, FMBM_RFCA_MR(4));
	/* enable Rx statistic counters */
	out_be32(&rx_port->fmbm_rstc, FMBM_RSTC_EN);
	/* disable Rx performance counters */
	out_be32(&rx_port->fmbm_rpc, 0);
}

static void bmi_tx_port_disable(struct fm_bmi_tx_port *tx_port)
{
	int timeout = 1000000;

	clrbits_be32(&tx_port->fmbm_tcfg, FMBM_TCFG_EN);

	/* wait until the tx port is not busy */
	while ((in_be32(&tx_port->fmbm_tst) & FMBM_TST_BSY) && timeout--)
		;
}

static void bmi_tx_port_init(struct fm_bmi_tx_port *tx_port)
{
	/* set BMI to independent mode, Tx port disable */
	out_be32(&tx_port->fmbm_tcfg, FMBM_TCFG_IM);
	/* Tx frame next engine -RISC */
	out_be32(&tx_port->fmbm_tfne, NIA_ENG_RISC | NIA_RISC_AC_IM_TX);
	out_be32(&tx_port->fmbm_tfene, NIA_ENG_RISC | NIA_RISC_AC_IM_TX);
	/* Tx command attribute - no order, MR[3] = 1 */
	clrbits_be32(&tx_port->fmbm_tfca, FMBM_TFCA_ORDER | FMBM_TFCA_MR_MASK);
	setbits_be32(&tx_port->fmbm_tfca, FMBM_TFCA_MR(4));
	/* enable Tx statistic counters */
	out_be32(&tx_port->fmbm_tstc, FMBM_TSTC_EN);
	/* disable Tx performance counters */
	out_be32(&tx_port->fmbm_tpc, 0);
}

static int fm_eth_rx_port_parameter_init(struct fm_eth *fm_eth)
{
	struct fm_port_global_pram *pram;
	u32 pram_page_offset;
	void *rx_bd_ring_base;
	void *rx_buf_pool;
	u32 bd_ring_base_lo, bd_ring_base_hi;
	struct fm_port_bd *rxbd;
	struct fm_port_qd *rxqd;
	struct fm_bmi_rx_port *bmi_rx_port = fm_eth->rx_port;
	int i;

	/* alloc global parameter ram at MURAM */
	pram = fm_muram_alloc(FM_PRAM_SIZE, FM_PRAM_ALIGN);
	if (!pram) {
		printf("%s: No muram for Rx global parameter\n", __func__);
		return -ENOMEM;
	}

	fm_eth->rx_pram = pram;

	/* parameter page offset to MURAM */
	pram_page_offset = (void *)pram - fm_muram_base();

	/* enable global mode- snooping data buffers and BDs */
	out_be32(&pram->mode, PRAM_MODE_GLOBAL);

	/* init the Rx queue descriptor pointer */
	out_be32(&pram->rxqd_ptr, pram_page_offset + 0x20);

	/* set the max receive buffer length, power of 2 */
	muram_writew(&pram->mrblr, MAX_RXBUF_LOG2);

	/* alloc Rx buffer descriptors from main memory */
	rx_bd_ring_base = dma_alloc_coherent(sizeof(struct fm_port_bd)
			* RX_BD_RING_SIZE, DMA_ADDRESS_BROKEN);
	if (!rx_bd_ring_base)
		return -ENOMEM;

	/* alloc Rx buffer from main memory */
	rx_buf_pool = dma_zalloc(MAX_RXBUF_LEN * RX_BD_RING_SIZE);

	/* save them to fm_eth */
	fm_eth->rx_bd_ring = rx_bd_ring_base;
	fm_eth->cur_rxbd_idx = 0;
	fm_eth->rx_buf = rx_buf_pool;

	/* init Rx BDs ring */
	for (i = 0; i < RX_BD_RING_SIZE; i++) {
		dma_addr_t dma;

		rxbd = &fm_eth->rx_bd_ring[i];

		muram_writew(&rxbd->status, RxBD_EMPTY);
		muram_writew(&rxbd->len, 0);

		dma = dma_map_single(fm_eth->dev,
				     rx_buf_pool + i * MAX_RXBUF_LEN,
				     MAX_RXBUF_LEN, DMA_FROM_DEVICE);
		if (dma_mapping_error(fm_eth->dev, dma))
			return -EFAULT;

		muram_writew(&rxbd->buf_ptr_hi, (u16)upper_32_bits(dma));
		out_be32(&rxbd->buf_ptr_lo, lower_32_bits(dma));
	}

	/* set the Rx queue descriptor */
	rxqd = &pram->rxqd;
	muram_writew(&rxqd->gen, 0);
	bd_ring_base_hi = upper_32_bits(virt_to_phys(rx_bd_ring_base));
	bd_ring_base_lo = lower_32_bits(virt_to_phys(rx_bd_ring_base));
	muram_writew(&rxqd->bd_ring_base_hi, (u16)bd_ring_base_hi);
	out_be32(&rxqd->bd_ring_base_lo, bd_ring_base_lo);
	muram_writew(&rxqd->bd_ring_size, sizeof(struct fm_port_bd)
			* RX_BD_RING_SIZE);
	muram_writew(&rxqd->offset_in, 0);
	muram_writew(&rxqd->offset_out, 0);

	/* set IM parameter ram pointer to Rx Frame Queue ID */
	out_be32(&bmi_rx_port->fmbm_rfqid, pram_page_offset);

	return 0;
}

static int fm_eth_tx_port_parameter_init(struct fm_eth *fm_eth)
{
	struct fm_port_global_pram *pram;
	u32 pram_page_offset;
	void *tx_bd_ring_base;
	u32 bd_ring_base_lo, bd_ring_base_hi;
	struct fm_port_bd *txbd;
	struct fm_port_qd *txqd;
	struct fm_bmi_tx_port *bmi_tx_port = fm_eth->tx_port;
	int i;

	/* alloc global parameter ram at MURAM */
	pram = fm_muram_alloc(FM_PRAM_SIZE, FM_PRAM_ALIGN);
	if (!pram)
		return -ENOMEM;

	fm_eth->tx_pram = pram;

	/* parameter page offset to MURAM */
	pram_page_offset = (void *)pram - fm_muram_base();

	/* enable global mode- snooping data buffers and BDs */
	out_be32(&pram->mode, PRAM_MODE_GLOBAL);

	/* init the Tx queue descriptor pionter */
	out_be32(&pram->txqd_ptr, pram_page_offset + 0x40);

	/* alloc Tx buffer descriptors from main memory */
	tx_bd_ring_base = dma_alloc_coherent(sizeof(struct fm_port_bd)
			* TX_BD_RING_SIZE, DMA_ADDRESS_BROKEN);
	if (!tx_bd_ring_base)
		return -ENOMEM;

	/* save it to fm_eth */
	fm_eth->tx_bd_ring = tx_bd_ring_base;
	fm_eth->cur_txbd_idx = 0;

	/* init Tx BDs ring */
	for (i = 0; i < TX_BD_RING_SIZE; i++) {
		txbd = &fm_eth->tx_bd_ring[i];

		muram_writew(&txbd->status, TxBD_LAST);
		muram_writew(&txbd->len, 0);
		muram_writew(&txbd->buf_ptr_hi, 0);
		out_be32(&txbd->buf_ptr_lo, 0);
	}

	/* set the Tx queue decriptor */
	txqd = &pram->txqd;
	bd_ring_base_hi = upper_32_bits(virt_to_phys(tx_bd_ring_base));
	bd_ring_base_lo = lower_32_bits(virt_to_phys(tx_bd_ring_base));
	muram_writew(&txqd->bd_ring_base_hi, (u16)bd_ring_base_hi);
	out_be32(&txqd->bd_ring_base_lo, bd_ring_base_lo);
	muram_writew(&txqd->bd_ring_size, sizeof(struct fm_port_bd)
			* TX_BD_RING_SIZE);
	muram_writew(&txqd->offset_in, 0);
	muram_writew(&txqd->offset_out, 0);

	/* set IM parameter ram pointer to Tx Confirmation Frame Queue ID */
	out_be32(&bmi_tx_port->fmbm_tcfqid, pram_page_offset);

	return 0;
}

static void fmc_tx_port_graceful_stop_enable(struct fm_eth *fm_eth)
{
	struct fm_port_global_pram *pram;

	pram = fm_eth->tx_pram;
	/* graceful stop transmission of frames */
	setbits_be32(&pram->mode, PRAM_MODE_GRACEFUL_STOP);
}

static void fmc_tx_port_graceful_stop_disable(struct fm_eth *fm_eth)
{
	struct fm_port_global_pram *pram;

	pram = fm_eth->tx_pram;
	/* re-enable transmission of frames */
	clrbits_be32(&pram->mode, PRAM_MODE_GRACEFUL_STOP);
}

static void memac_adjust_link_speed(struct eth_device *edev)
{
	struct fm_eth *fm_eth = to_fm_eth(edev);
	struct memac *regs = fm_eth->regs;
	int speed = edev->phydev->speed;
	u32 if_mode;
	phy_interface_t type = fm_eth->enet_if;

	if_mode = in_be32(&regs->if_mode);

	if (type == PHY_INTERFACE_MODE_RGMII ||
	    type == PHY_INTERFACE_MODE_RGMII_ID ||
	    type == PHY_INTERFACE_MODE_RGMII_TXID) {
		if_mode &= ~IF_MODE_EN_AUTO;
		if_mode &= ~IF_MODE_SETSP_MASK;

		switch (speed) {
		case SPEED_1000:
			if_mode |= IF_MODE_SETSP_1000M;
			break;
		case SPEED_100:
			if_mode |= IF_MODE_SETSP_100M;
			break;
		case SPEED_10:
			if_mode |= IF_MODE_SETSP_10M;
		default:
			break;
		}
	}

	out_be32(&regs->if_mode, if_mode);

	return;
}

static int fm_eth_open(struct eth_device *edev)
{
	struct fm_eth *fm_eth = to_fm_eth(edev);
	struct memac *regs = fm_eth->regs;
	int ret;

	ret = phy_device_connect(edev, NULL, -1, memac_adjust_link_speed, 0,
				 fm_eth->enet_if);
	if (ret)
		return ret;

	/* enable bmi Rx port */
	setbits_be32(&fm_eth->rx_port->fmbm_rcfg, FMBM_RCFG_EN);
	/* enable MAC rx/tx port */
	setbits_be32(&regs->command_config,
		     MEMAC_CMD_CFG_RXTX_EN | MEMAC_CMD_CFG_NO_LEN_CHK);

	/* enable bmi Tx port */
	setbits_be32(&fm_eth->tx_port->fmbm_tcfg, FMBM_TCFG_EN);
	/* re-enable transmission of frame */
	fmc_tx_port_graceful_stop_disable(fm_eth);

	return 0;
}

static void memac_disable_mac(struct fm_eth *fm_eth)
{
	struct memac *regs = fm_eth->regs;

	clrbits_be32(&regs->command_config, MEMAC_CMD_CFG_RXTX_EN);
}

static void fm_eth_halt(struct eth_device *edev)
{
	struct fm_eth *fm_eth = to_fm_eth(edev);

	/* graceful stop the transmission of frames */
	fmc_tx_port_graceful_stop_enable(fm_eth);
	/* disable bmi Tx port */
	bmi_tx_port_disable(fm_eth->tx_port);
	/* disable MAC rx/tx port */
	memac_disable_mac(fm_eth);
	/* disable bmi Rx port */
	bmi_rx_port_disable(fm_eth->rx_port);
}

static int fm_eth_send(struct eth_device *edev, void *buf, int len)
{
	struct fm_eth *fm_eth = to_fm_eth(edev);
	struct fm_port_global_pram *pram;
	struct fm_port_bd *txbd;
	int i, ret;
	dma_addr_t dma;

	pram = fm_eth->tx_pram;
	txbd = &fm_eth->tx_bd_ring[fm_eth->cur_txbd_idx];

	/* find one empty TxBD */
	for (i = 0; muram_readw(&txbd->status) & TxBD_READY; i++) {
		udelay(100);
		if (i > 0x1000) {
			dev_err(&edev->dev, "Tx buffer not ready, txbd->status = 0x%x\n",
			       muram_readw(&txbd->status));
			return -EIO;
		}
	}

	dma = dma_map_single(fm_eth->dev, buf, len, DMA_TO_DEVICE);
	if (dma_mapping_error(fm_eth->dev, dma))
		return -EFAULT;

	/* setup TxBD */
	muram_writew(&txbd->buf_ptr_hi, (u16)upper_32_bits(dma));
	out_be32(&txbd->buf_ptr_lo, lower_32_bits(dma));
	muram_writew(&txbd->len, len);
	muram_writew(&txbd->status, TxBD_READY | TxBD_LAST);

	/* advance the TxBD */
	fm_eth->cur_txbd_idx = (fm_eth->cur_txbd_idx + 1) % TX_BD_RING_SIZE;

	/* update TxQD, let RISC to send the packet */
	muram_writew(&pram->txqd.offset_in,
		     fm_eth->cur_txbd_idx * sizeof(struct fm_port_bd));

	/* wait for buffer to be transmitted */
	ret = 0;
	for (i = 0; muram_readw(&txbd->status) & TxBD_READY; i++) {
		udelay(10);
		if (i > 0x10000) {
			dev_err(&edev->dev, "Tx error, txbd->status = 0x%x\n",
			       muram_readw(&txbd->status));
			ret = -EIO;
			break;
		}
	}

	dma_unmap_single(fm_eth->dev, dma, len, DMA_TO_DEVICE);

	return ret;
}

static int fm_eth_recv(struct eth_device *edev)
{
	struct fm_eth *fm_eth = to_fm_eth(edev);
	struct fm_port_global_pram *pram;
	struct fm_port_bd *rxbd;
	u16 status, len;
	u32 buf_lo, buf_hi;
	u8 *data;
	u16 offset_out;
	int ret = 1;

	pram = fm_eth->rx_pram;

	while (1) {
		rxbd = &fm_eth->rx_bd_ring[fm_eth->cur_rxbd_idx];

		status = muram_readw(&rxbd->status);
		if (status & RxBD_EMPTY)
			break;

		if (!(status & RxBD_ERROR)) {
			buf_hi = muram_readw(&rxbd->buf_ptr_hi);
			buf_lo = in_be32(&rxbd->buf_ptr_lo);
			data = (u8 *)((unsigned long)(buf_hi << 16) << 16 | buf_lo);
			len = muram_readw(&rxbd->len);

			dma_sync_single_for_cpu(fm_eth->dev, (unsigned long)data,
						len,
						DMA_FROM_DEVICE);

			net_receive(edev, data, len);

			dma_sync_single_for_device(fm_eth->dev, (unsigned long)data,
						len,
						DMA_FROM_DEVICE);
		} else {
			dev_err(&edev->dev, "Rx error\n");
			ret = 0;
		}

		/* clear the RxBDs */
		muram_writew(&rxbd->status, RxBD_EMPTY);
		muram_writew(&rxbd->len, 0);

		/* advance RxBD */
		fm_eth->cur_rxbd_idx = (fm_eth->cur_rxbd_idx + 1) % RX_BD_RING_SIZE;

		/* update RxQD */
		offset_out = muram_readw(&pram->rxqd.offset_out);
		offset_out += sizeof(struct fm_port_bd);
		if (offset_out >= muram_readw(&pram->rxqd.bd_ring_size))
			offset_out = 0;
		muram_writew(&pram->rxqd.offset_out, offset_out);
	}

	return ret;
}

static void memac_init_mac(struct fm_eth *fm_eth)
{
	struct memac *regs = fm_eth->regs;

	/* mask all interrupt */
	out_be32(&regs->imask, IMASK_MASK_ALL);

	/* clear all events */
	out_be32(&regs->ievent, IEVENT_CLEAR_ALL);

	/* set the max receive length */
	out_be32(&regs->maxfrm, MAX_RXBUF_LEN);

	/* multicast frame reception for the hash entry disable */
	out_be32(&regs->hashtable_ctrl, 0);
}

static int memac_set_ethaddr(struct eth_device *edev, const unsigned char *adr)
{
	struct fm_eth *fm_eth = to_fm_eth(edev);
	struct memac *regs = fm_eth->regs;
	u32 mac_addr0, mac_addr1;

	/*
	 * if a station address of 0x12345678ABCD, perform a write to
	 * MAC_ADDR0 of 0x78563412, MAC_ADDR1 of 0x0000CDAB
	 */
	mac_addr0 = (adr[3] << 24) | (adr[2] << 16) | \
			(adr[1] << 8)  | (adr[0]);
	out_be32(&regs->mac_addr_0, mac_addr0);

	mac_addr1 = ((adr[5] << 8) | adr[4]) & 0x0000ffff;
	out_be32(&regs->mac_addr_1, mac_addr1);

	return 0;
}

static int memac_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct fm_eth *fm_eth = to_fm_eth(edev);
	struct memac *regs = fm_eth->regs;
	u32 mac_addr0, mac_addr1;

	mac_addr0 = in_be32(&regs->mac_addr_0);
	mac_addr1 = in_be32(&regs->mac_addr_1);

	adr[0] = mac_addr0 & 0xff;
	adr[1] = (mac_addr0 >> 8) & 0xff;
	adr[2] = (mac_addr0 >> 16) & 0xff;
	adr[3] = (mac_addr0 >> 24) & 0xff;
	adr[4] = mac_addr1 & 0xff;
	adr[5] = (mac_addr1 >> 8) & 0xff;

	return 0;
}

static void memac_set_interface_mode(struct fm_eth *fm_eth,
					phy_interface_t type)
{
	struct memac *regs = fm_eth->regs;
	u32 if_mode;

	/* clear all bits relative with interface mode */
	if_mode = in_be32(&regs->if_mode) & ~IF_MODE_MASK;

	/* set interface mode */
	switch (type) {
	case PHY_INTERFACE_MODE_GMII:
		if_mode |= IF_MODE_GMII | IF_MODE_EN_AUTO;
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		if_mode |= IF_MODE_GMII | IF_MODE_RG | IF_MODE_EN_AUTO;
		break;
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_QSGMII:
		if_mode |= IF_MODE_GMII | IF_MODE_EN_AUTO;
		break;
	case PHY_INTERFACE_MODE_XGMII:
		if_mode |= IF_MODE_XGMII;
		break;
	default:
		break;
	}

	out_be32(&regs->if_mode, if_mode);
}

static int fm_eth_startup(struct fm_eth *fm_eth)
{
	int ret;

	ret = fm_eth_rx_port_parameter_init(fm_eth);
	if (ret)
		return ret;

	ret = fm_eth_tx_port_parameter_init(fm_eth);
	if (ret)
		return ret;

	/* setup the MAC controller */
	memac_init_mac(fm_eth);

	/* init bmi rx port, IM mode and disable */
	bmi_rx_port_init(fm_eth->rx_port);

	/* init bmi tx port, IM mode and disable */
	bmi_tx_port_init(fm_eth->tx_port);

	memac_set_interface_mode(fm_eth, fm_eth->enet_if);

	return 0;
}

static int fsl_fman_mdio_probe(struct device *dev)
{
	struct resource *iores;
	int ret;
	struct fsl_fman_mdio *priv;

	dev_dbg(dev, "probe\n");

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	priv = xzalloc(sizeof(*priv));

	priv->bus.read = memac_mdio_read;
	priv->bus.write = memac_mdio_write;
	priv->bus.parent = dev;
	priv->regs = IOMEM(iores->start);

	memac_setbits_32(&priv->regs->mdio_stat,
			 MDIO_STAT_CLKDIV(258) | MDIO_STAT_NEG);

	ret = mdiobus_register(&priv->bus);
	if (ret)
		return ret;

	return 0;
}

static int fsl_fman_port_probe(struct device *dev)
{
	struct resource *iores;
	int ret;
	struct fsl_fman_port *port;
	unsigned long type;

	dev_dbg(dev, "probe\n");

	ret = dev_get_drvdata(dev, (const void **)&type);
	if (ret)
		return ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	port = xzalloc(sizeof(*port));

	port->regs = IOMEM(iores->start);
	port->type = type;

	if (type == FMAN_PORT_TYPE_RX)
		port->rxport = port->regs;
	else
		port->txport = port->regs;

	dev->priv = port;

	return 0;
}

static int fsl_fman_memac_port_bind(struct fm_eth *fm_eth, enum fman_port_type type)
{
	struct device_node *macnp = fm_eth->dev->of_node;
	struct device_node *portnp;
	struct device *portdev;
	struct fsl_fman_port *port;

	portnp = of_parse_phandle(macnp, "fsl,fman-ports", type);
	if (!portnp) {
		dev_err(fm_eth->dev, "of_parse_phandle(%pOF, fsl,fman-ports) failed\n",
			macnp);
		return -EINVAL;
	}

	portdev = of_find_device_by_node(portnp);
	if (!portdev)
		return -ENOENT;

	port = portdev->priv;
	if (!port)
		return -EINVAL;

	if (type == FMAN_PORT_TYPE_TX)
		fm_eth->tx_port = port->txport;
	else
		fm_eth->rx_port = port->rxport;

	return 0;
}

static int fsl_fman_memac_probe(struct device *dev)
{
	struct resource *iores;
	struct fm_eth *fm_eth;
	struct eth_device *edev;
	int ret;
	int phy_mode;

	dev_dbg(dev, "probe\n");

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	/* alloc the FMan ethernet private struct */
	fm_eth = xzalloc(sizeof(*fm_eth));

	dev->priv = fm_eth;

	fm_eth->dev = dev;

	ret = fsl_fman_memac_port_bind(fm_eth, FMAN_PORT_TYPE_TX);
	if (ret)
		return ret;

	ret = fsl_fman_memac_port_bind(fm_eth, FMAN_PORT_TYPE_RX);
	if (ret)
		return ret;

	phy_mode = of_get_phy_mode(dev->of_node);
	if (phy_mode < 0)
		return phy_mode;

	fm_eth->enet_if = phy_mode;

	fm_eth->regs = IOMEM(iores->start);

	dev->priv = fm_eth;

	edev = &fm_eth->edev;
	edev->open = fm_eth_open;
	edev->halt = fm_eth_halt;
	edev->send = fm_eth_send;
	edev->recv = fm_eth_recv;
	edev->get_ethaddr = memac_get_ethaddr;
	edev->set_ethaddr = memac_set_ethaddr;
	edev->parent = dev;

	/* startup the FM im */
	ret = fm_eth_startup(fm_eth);
	if (ret)
		return ret;

	ret = eth_register(edev);
	if (ret)
		return ret;

	return 0;
}

static void fsl_fman_memac_remove(struct device *dev)
{
	struct fm_eth *fm_eth = dev->priv;

	fm_eth_halt(&fm_eth->edev);
}

static int fsl_fman_muram_probe(struct device *dev)
{
	struct resource *iores;

	dev_dbg(dev, "probe\n");

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	muram.base = IOMEM(iores->start);
	muram.size = resource_size(iores);
	muram.alloc = muram.base + FM_MURAM_RES_SIZE;
	muram.top = muram.base + muram.size;

	return 0;
}

static struct of_device_id fsl_fman_mdio_dt_ids[] = {
	{
		.compatible = "fsl,fman-memac-mdio",
	}, {
	}
};
MODULE_DEVICE_TABLE(of, fsl_fman_mdio_dt_ids);

static struct driver fman_mdio_driver = {
	.name   = "fsl-fman-mdio",
	.probe  = fsl_fman_mdio_probe,
	.of_compatible = DRV_OF_COMPAT(fsl_fman_mdio_dt_ids),
};

static struct of_device_id fsl_fman_port_dt_ids[] = {
	{
		.compatible = "fsl,fman-v3-port-rx",
		.data = (void *)FMAN_PORT_TYPE_RX,
	}, {
		.compatible = "fsl,fman-v3-port-tx",
		.data = (void *)FMAN_PORT_TYPE_TX,
	}, {
	}
};
MODULE_DEVICE_TABLE(of, fsl_fman_port_dt_ids);

static struct driver fman_port_driver = {
	.name   = "fsl-fman-port",
	.probe  = fsl_fman_port_probe,
	.of_compatible = DRV_OF_COMPAT(fsl_fman_port_dt_ids),
};

static struct of_device_id fsl_fman_memac_dt_ids[] = {
	{
		.compatible = "fsl,fman-memac",
	}, {
	}
};
MODULE_DEVICE_TABLE(of, fsl_fman_memac_dt_ids);

static struct driver fman_memac_driver = {
	.name   = "fsl-fman-memac",
	.probe  = fsl_fman_memac_probe,
	.remove = fsl_fman_memac_remove,
	.of_compatible = DRV_OF_COMPAT(fsl_fman_memac_dt_ids),
};

static struct of_device_id fsl_fman_muram_dt_ids[] = {
	{
		.compatible = "fsl,fman-muram",
	}, {
	}
};
MODULE_DEVICE_TABLE(of, fsl_fman_muram_dt_ids);

static struct driver fman_muram_driver = {
	.name   = "fsl-fman-muram",
	.probe  = fsl_fman_muram_probe,
	.of_compatible = DRV_OF_COMPAT(fsl_fman_muram_dt_ids),
};

static int fsl_fman_probe(struct device *dev)
{
	struct resource *iores;
	struct ccsr_fman *reg;
	int ret;

	dev_dbg(dev, "----------------> probe\n");

	iores = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	reg = IOMEM(iores->start);
	dev->priv = reg;

	ret = of_platform_populate(dev->of_node, NULL, dev);
	if (ret)
		return ret;

	platform_driver_register(&fman_muram_driver);
	platform_driver_register(&fman_mdio_driver);
	platform_driver_register(&fman_port_driver);
	platform_driver_register(&fman_memac_driver);

	ret = fm_init_common(dev, reg);
	if (ret)
		return ret;

	return 0;
}

static struct of_device_id fsl_fman_dt_ids[] = {
	{
		.compatible = "fsl,fman",
	}, {
	}
};
MODULE_DEVICE_TABLE(of, fsl_fman_dt_ids);

static struct driver fman_driver = {
	.name   = "fsl-fman",
	.probe  = fsl_fman_probe,
	.of_compatible = DRV_OF_COMPAT(fsl_fman_dt_ids),
};
device_platform_driver(fman_driver);

static int fman_of_fixup(struct device_node *root, void *context)
{
	struct device_node *fman, *fman_bb;
	struct device_node *child, *child_bb;

	fman_bb = of_find_compatible_node(NULL, NULL, "fsl,fman");
	if (!fman_bb)
		return 0;

	fman = of_find_compatible_node(root, NULL, "fsl,fman");
	if (!fman)
		return 0;

	/*
	 * The dts files in the Linux tree have all network interfaces
	 * enabled. Disable the ones that are disabled under barebox
	 * as well.
	 */
	for_each_child_of_node(fman, child) {
		if (!of_device_is_compatible(child, "fsl,fman-memac"))
			continue;

		child_bb = of_get_child_by_name(fman_bb, child->name);
		if (!child_bb)
			continue;

		if (of_device_is_available(child_bb))
			of_device_enable(child);
		else
			of_device_disable(child);
	}

	return 0;
}

static int fman_register_of_fixup(void)
{
	of_register_fixup(fman_of_fixup, NULL);

	return 0;
}
late_initcall(fman_register_of_fixup);
