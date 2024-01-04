// SPDX-License-Identifier: GPL-2.0+

/*
 * Derived from Freescale LSDK-19.09-update-311219
 */
#include <common.h>
#include <io.h>
#include <clock.h>
#include <asm/barebox-arm-head.h>
#include <asm/syscounter.h>
#include <asm/system.h>
#include <mach/layerscape/errata.h>
#include <mach/layerscape/lowlevel.h>
#include <mach/layerscape/fsl_epu.h>
#include <soc/fsl/immap_lsch2.h>
#include <soc/fsl/fsl_immap.h>
#include <soc/fsl/scfg.h>

void udelay(unsigned long usecs)
{
	arm_architected_timer_udelay(usecs);
}

void mdelay(unsigned long msecs)
{
	udelay(1000 * msecs);
}

enum csu_cslx_access {
	CSU_NS_SUP_R = 0x08,
	CSU_NS_SUP_W = 0x80,
	CSU_NS_SUP_RW = 0x88,
	CSU_NS_USER_R = 0x04,
	CSU_NS_USER_W = 0x40,
	CSU_NS_USER_RW = 0x44,
	CSU_S_SUP_R = 0x02,
	CSU_S_SUP_W = 0x20,
	CSU_S_SUP_RW = 0x22,
	CSU_S_USER_R = 0x01,
	CSU_S_USER_W = 0x10,
	CSU_S_USER_RW = 0x11,
	CSU_ALL_RW = 0xff,
};

struct csu_ns_dev {
	unsigned long ind;
	uint32_t val;
};

enum csu_cslx_ind {
	CSU_CSLX_PCIE2_IO = 0,
	CSU_CSLX_PCIE1_IO,
	CSU_CSLX_MG2TPR_IP,
	CSU_CSLX_IFC_MEM,
	CSU_CSLX_OCRAM,
	CSU_CSLX_GIC,
	CSU_CSLX_PCIE1,
	CSU_CSLX_OCRAM2,
	CSU_CSLX_QSPI_MEM,
	CSU_CSLX_PCIE2,
	CSU_CSLX_SATA,
	CSU_CSLX_USB3,
	CSU_CSLX_SERDES = 32,
	CSU_CSLX_QDMA,
	CSU_CSLX_LPUART2,
	CSU_CSLX_LPUART1,
	CSU_CSLX_LPUART4,
	CSU_CSLX_LPUART3,
	CSU_CSLX_LPUART6,
	CSU_CSLX_LPUART5,
	CSU_CSLX_DSPI2 = 40,
	CSU_CSLX_DSPI1,
	CSU_CSLX_QSPI,
	CSU_CSLX_ESDHC,
	CSU_CSLX_2D_ACE,
	CSU_CSLX_IFC,
	CSU_CSLX_I2C1,
	CSU_CSLX_USB2,
	CSU_CSLX_I2C3,
	CSU_CSLX_I2C2,
	CSU_CSLX_DUART2 = 50,
	CSU_CSLX_DUART1,
	CSU_CSLX_WDT2,
	CSU_CSLX_WDT1,
	CSU_CSLX_EDMA,
	CSU_CSLX_SYS_CNT,
	CSU_CSLX_DMA_MUX2,
	CSU_CSLX_DMA_MUX1,
	CSU_CSLX_DDR,
	CSU_CSLX_QUICC,
	CSU_CSLX_DCFG_CCU_RCPM = 60,
	CSU_CSLX_SECURE_BOOTROM,
	CSU_CSLX_SFP,
	CSU_CSLX_TMU,
	CSU_CSLX_SECURE_MONITOR,
	CSU_CSLX_RESERVED0,
	CSU_CSLX_ETSEC1,
	CSU_CSLX_SEC5_5,
	CSU_CSLX_ETSEC3,
	CSU_CSLX_ETSEC2,
	CSU_CSLX_GPIO2 = 70,
	CSU_CSLX_GPIO1,
	CSU_CSLX_GPIO4,
	CSU_CSLX_GPIO3,
	CSU_CSLX_PLATFORM_CONT,
	CSU_CSLX_CSU,
	CSU_CSLX_ASRC,
	CSU_CSLX_SPDIF,
	CSU_CSLX_FLEXCAN2,
	CSU_CSLX_FLEXCAN1,
	CSU_CSLX_FLEXCAN4 = 80,
	CSU_CSLX_FLEXCAN3,
	CSU_CSLX_SAI2,
	CSU_CSLX_SAI1,
	CSU_CSLX_SAI4,
	CSU_CSLX_SAI3,
	CSU_CSLX_FTM2,
	CSU_CSLX_FTM1,
	CSU_CSLX_FTM4,
	CSU_CSLX_FTM3,
	CSU_CSLX_FTM6 = 90,
	CSU_CSLX_FTM5,
	CSU_CSLX_FTM8,
	CSU_CSLX_FTM7,
	CSU_CSLX_EPU,
	CSU_CSLX_COP_DCSR,
	CSU_CSLX_DDI,
	CSU_CSLX_GDI,
	CSU_CSLX_RESERVED1,
	CSU_CSLX_USB3_PHY = 116,
	CSU_CSLX_RESERVED2,
	CSU_CSLX_MAX,
};

static struct csu_ns_dev ns_dev[] = {
	{ CSU_CSLX_PCIE2_IO, CSU_ALL_RW },
	{ CSU_CSLX_PCIE1_IO, CSU_ALL_RW },
	{ CSU_CSLX_MG2TPR_IP, CSU_ALL_RW },
	{ CSU_CSLX_IFC_MEM, CSU_ALL_RW },
	{ CSU_CSLX_OCRAM, CSU_ALL_RW },
	{ CSU_CSLX_GIC, CSU_ALL_RW },
	{ CSU_CSLX_PCIE1, CSU_ALL_RW },
	{ CSU_CSLX_OCRAM2, CSU_ALL_RW },
	{ CSU_CSLX_QSPI_MEM, CSU_ALL_RW },
	{ CSU_CSLX_PCIE2, CSU_ALL_RW },
	{ CSU_CSLX_SATA, CSU_ALL_RW },
	{ CSU_CSLX_USB3, CSU_ALL_RW },
	{ CSU_CSLX_SERDES, CSU_ALL_RW },
	{ CSU_CSLX_QDMA, CSU_ALL_RW },
	{ CSU_CSLX_LPUART2, CSU_ALL_RW },
	{ CSU_CSLX_LPUART1, CSU_ALL_RW },
	{ CSU_CSLX_LPUART4, CSU_ALL_RW },
	{ CSU_CSLX_LPUART3, CSU_ALL_RW },
	{ CSU_CSLX_LPUART6, CSU_ALL_RW },
	{ CSU_CSLX_LPUART5, CSU_ALL_RW },
	{ CSU_CSLX_DSPI2, CSU_ALL_RW },
	{ CSU_CSLX_DSPI1, CSU_ALL_RW },
	{ CSU_CSLX_QSPI, CSU_ALL_RW },
	{ CSU_CSLX_ESDHC, CSU_ALL_RW },
	{ CSU_CSLX_2D_ACE, CSU_ALL_RW },
	{ CSU_CSLX_IFC, CSU_ALL_RW },
	{ CSU_CSLX_I2C1, CSU_ALL_RW },
	{ CSU_CSLX_USB2, CSU_ALL_RW },
	{ CSU_CSLX_I2C3, CSU_ALL_RW },
	{ CSU_CSLX_I2C2, CSU_ALL_RW },
	{ CSU_CSLX_DUART2, CSU_ALL_RW },
	{ CSU_CSLX_DUART1, CSU_ALL_RW },
	{ CSU_CSLX_WDT2, CSU_ALL_RW },
	{ CSU_CSLX_WDT1, CSU_ALL_RW },
	{ CSU_CSLX_EDMA, CSU_ALL_RW },
	{ CSU_CSLX_SYS_CNT, CSU_ALL_RW },
	{ CSU_CSLX_DMA_MUX2, CSU_ALL_RW },
	{ CSU_CSLX_DMA_MUX1, CSU_ALL_RW },
	{ CSU_CSLX_DDR, CSU_ALL_RW },
	{ CSU_CSLX_QUICC, CSU_ALL_RW },
	{ CSU_CSLX_DCFG_CCU_RCPM, CSU_ALL_RW },
	{ CSU_CSLX_SECURE_BOOTROM, CSU_ALL_RW },
	{ CSU_CSLX_SFP, CSU_ALL_RW },
	{ CSU_CSLX_TMU, CSU_ALL_RW },
	{ CSU_CSLX_SECURE_MONITOR, CSU_ALL_RW },
	{ CSU_CSLX_RESERVED0, CSU_ALL_RW },
	{ CSU_CSLX_ETSEC1, CSU_ALL_RW },
	{ CSU_CSLX_SEC5_5, CSU_ALL_RW },
	{ CSU_CSLX_ETSEC3, CSU_ALL_RW },
	{ CSU_CSLX_ETSEC2, CSU_ALL_RW },
	{ CSU_CSLX_GPIO2, CSU_ALL_RW },
	{ CSU_CSLX_GPIO1, CSU_ALL_RW },
	{ CSU_CSLX_GPIO4, CSU_ALL_RW },
	{ CSU_CSLX_GPIO3, CSU_ALL_RW },
	{ CSU_CSLX_PLATFORM_CONT, CSU_ALL_RW },
	{ CSU_CSLX_CSU, CSU_ALL_RW },
	{ CSU_CSLX_ASRC, CSU_ALL_RW },
	{ CSU_CSLX_SPDIF, CSU_ALL_RW },
	{ CSU_CSLX_FLEXCAN2, CSU_ALL_RW },
	{ CSU_CSLX_FLEXCAN1, CSU_ALL_RW },
	{ CSU_CSLX_FLEXCAN4, CSU_ALL_RW },
	{ CSU_CSLX_FLEXCAN3, CSU_ALL_RW },
	{ CSU_CSLX_SAI2, CSU_ALL_RW },
	{ CSU_CSLX_SAI1, CSU_ALL_RW },
	{ CSU_CSLX_SAI4, CSU_ALL_RW },
	{ CSU_CSLX_SAI3, CSU_ALL_RW },
	{ CSU_CSLX_FTM2, CSU_ALL_RW },
	{ CSU_CSLX_FTM1, CSU_ALL_RW },
	{ CSU_CSLX_FTM4, CSU_ALL_RW },
	{ CSU_CSLX_FTM3, CSU_ALL_RW },
	{ CSU_CSLX_FTM6, CSU_ALL_RW },
	{ CSU_CSLX_FTM5, CSU_ALL_RW },
	{ CSU_CSLX_FTM8, CSU_ALL_RW },
	{ CSU_CSLX_FTM7, CSU_ALL_RW },
	{ CSU_CSLX_COP_DCSR, CSU_ALL_RW },
	{ CSU_CSLX_EPU, CSU_ALL_RW },
	{ CSU_CSLX_GDI, CSU_ALL_RW },
	{ CSU_CSLX_DDI, CSU_ALL_RW },
	{ CSU_CSLX_RESERVED1, CSU_ALL_RW },
	{ CSU_CSLX_USB3_PHY, CSU_ALL_RW },
	{ CSU_CSLX_RESERVED2, CSU_ALL_RW },
};

/* Found in U-boot but not in LS1021ARM.pdf 02/2020 */
#define DCSR_RCPM2_ADDR	0x20223000
#define DCSR_RCPM2_CPMFSMCR0	0x400
#define DCSR_RCPM2_CPMFSMSR0	0x404
#define DCSR_RCPM2_CPMFSMCR1	0x414
#define DCSR_RCPM2_CPMFSMSR1	0x418
#define CPMFSMSR_FSM_STATE_MASK	0x7f

#define DCSR_EPU_ADDR	0x20000000

static void set_devices_ns_access(unsigned long index, u16 val)
{
	u32 *base = IOMEM(LSCH2_CSU_ADDR);
	u32 *reg;
	uint32_t tmp;

	reg = base + index / 2;
	tmp = in_be32(reg);
	if (index % 2 == 0) {
		tmp &= 0x0000ffff;
		tmp |= val << 16;
	} else {
		tmp &= 0xffff0000;
		tmp |= val;
	}

	out_be32(reg, tmp);
}

static void init_csu(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ns_dev); i++)
		set_devices_ns_access(ns_dev[i].ind, ns_dev[i].val);
}

/**
 * fsl_epu_clean - Clear EPU registers
 */
static void fsl_epu_clean(void *epu_base)
{
	u32 offset;

	/* follow the exact sequence to clear the registers */
	/* Clear EPACRn */
	for (offset = EPACR0; offset <= EPACR15; offset += EPACR_STRIDE)
		out_be32(epu_base + offset, 0);

	/* Clear EPEVTCRn */
	for (offset = EPEVTCR0; offset <= EPEVTCR9; offset += EPEVTCR_STRIDE)
		out_be32(epu_base + offset, 0);

	/* Clear EPGCR */
	out_be32(epu_base + EPGCR, 0);

	/* Clear EPSMCRn */
	for (offset = EPSMCR0; offset <= EPSMCR15; offset += EPSMCR_STRIDE)
		out_be32(epu_base + offset, 0);

	/* Clear EPCCRn */
	for (offset = EPCCR0; offset <= EPCCR31; offset += EPCCR_STRIDE)
		out_be32(epu_base + offset, 0);

	/* Clear EPCMPRn */
	for (offset = EPCMPR0; offset <= EPCMPR31; offset += EPCMPR_STRIDE)
		out_be32(epu_base + offset, 0);

	/* Clear EPCTRn */
	for (offset = EPCTR0; offset <= EPCTR31; offset += EPCTR_STRIDE)
		out_be32(epu_base + offset, 0);

	/* Clear EPIMCRn */
	for (offset = EPIMCR0; offset <= EPIMCR31; offset += EPIMCR_STRIDE)
		out_be32(epu_base + offset, 0);

	/* Clear EPXTRIGCRn */
	out_be32(epu_base + EPXTRIGCR, 0);

	/* Clear EPECRn */
	for (offset = EPECR0; offset <= EPECR15; offset += EPECR_STRIDE)
		out_be32(epu_base + offset, 0);
}

#define TIMER_COMP_VAL			0xffffffffffffffffull
#define ARCH_TIMER_CTRL_ENABLE		(1 << 0)
#define SYS_COUNTER_CTRL_ENABLE		(1 << 24)
#define SCFG_QSPI_CLKSEL		0x50100000

/* ls102xa_init_lowlevel
 * Based on ls1046 and U-boot ls102xa arch_cpu_init
 */
void ls102xa_init_lowlevel(void)
{
	struct ccsr_cci400 __iomem *cci = IOMEM(LSCH2_CCI400_ADDR);
	struct ls102xa_ccsr_scfg *scfg = IOMEM(LSCH2_SCFG_ADDR);
	struct ls102xa_ccsr_gur __iomem *gur = IOMEM(LSCH2_GUTS_ADDR);
	void *rcpm2_base = IOMEM(DCSR_RCPM2_ADDR);
	void *epu_base = IOMEM(DCSR_EPU_ADDR);
	uint32_t state, major, ctrl, freq;
	uint64_t val;

	cortex_a7_lowlevel_init();
	arm_cpu_lowlevel_init();

	scfg_init(SCFG_ENDIANESS_BIG);
	init_csu();

	writel(SYS_COUNTER_CTRL_ENABLE, LSCH2_SYS_COUNTER_ADDR);
	freq = 12500000;
	asm("mcr p15, 0, %0, c14, c0, 0" : : "r" (freq));

	/* Set PL1 Physical Timer Ctrl */
	ctrl = ARCH_TIMER_CTRL_ENABLE;
	asm("mcr p15, 0, %0, c14, c2, 1" : : "r" (ctrl));

	/* Set PL1 Physical Comp Value */
	val = TIMER_COMP_VAL;
	asm("mcrr p15, 2, %Q0, %R0, c14" : : "r" (val));


	state = in_be32(rcpm2_base + DCSR_RCPM2_CPMFSMSR0) &
		CPMFSMSR_FSM_STATE_MASK;
	if (state != 0) {
		out_be32(rcpm2_base + DCSR_RCPM2_CPMFSMCR0, 0x80);
		out_be32(rcpm2_base + DCSR_RCPM2_CPMFSMCR0, 0x0);
	}
	state = in_be32(rcpm2_base + DCSR_RCPM2_CPMFSMSR1) &
		CPMFSMSR_FSM_STATE_MASK;
	if (state != 0) {
		out_be32(rcpm2_base + DCSR_RCPM2_CPMFSMCR1, 0x80);
		out_be32(rcpm2_base + DCSR_RCPM2_CPMFSMCR1, 0x0);
	}

	fsl_epu_clean(epu_base);

	/* Enable all the snoop signal for various masters */
	out_be32(&scfg->snpcnfgcr, SCFG_SNPCNFGCR_SEC_RD_WR |
				SCFG_SNPCNFGCR_DBG_RD_WR |
				SCFG_SNPCNFGCR_EDMA_SNP);

	if (IS_ENABLED(CONFIG_DRIVER_SPI_FSL_QUADSPI))
		out_be32(&scfg->qspi_cfg, SCFG_QSPI_CLKSEL);

	/* Configure Little endian for SAI, ASRC and SPDIF */
	out_be32(&scfg->endiancr, SCFG_ENDIANCR_LE);

	/*
	 * Enable snoop requests and DVM message requests for
	 * All the slave interfaces.
	 */
	out_le32(&cci->slave[0].snoop_ctrl,
		CCI400_DVM_MESSAGE_REQ_EN | CCI400_SNOOP_REQ_EN);
	out_le32(&cci->slave[1].snoop_ctrl,
		 CCI400_DVM_MESSAGE_REQ_EN | CCI400_SNOOP_REQ_EN);
	out_le32(&cci->slave[2].snoop_ctrl,
		 CCI400_DVM_MESSAGE_REQ_EN | CCI400_SNOOP_REQ_EN);
	out_le32(&cci->slave[4].snoop_ctrl,
		 CCI400_DVM_MESSAGE_REQ_EN | CCI400_SNOOP_REQ_EN);

	major = in_be32(&gur->svr);
	if (SVR_MAJ(major) == SOC_MAJOR_VER_1_0) {
		/*
		 * Set CCI-400 Slave interface S1, S2 Shareable Override
		 * Register All transactions are treated as non-shareable
		 */
		out_le32(&cci->slave[1].sha_ord, CCI400_SHAORD_NON_SHAREABLE);
		out_le32(&cci->slave[2].sha_ord, CCI400_SHAORD_NON_SHAREABLE);
	}

	/*
	 * Memory controller require a register write before being enabled.
	 * Affects: DDR
	 * Register: EDDRTQCFG
	 * Description: Memory controller performance is not optimal with
	 *		default internal target queue register values.
	 * Workaround: Write a value of 63b2_0042h to address: 157_020Ch.
	 */
	out_be32(&scfg->eddrtqcfg, 0x63b20042);

	ls1021a_errata();
}
