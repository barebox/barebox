/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * LayerScape Internal Memory Map
 *
 * Copyright 2017-2020 NXP
 * Copyright 2014 Freescale Semiconductor, Inc.
 */

#ifndef __ARCH_FSL_LSCH3_IMMAP_H_
#define __ARCH_FSL_LSCH3_IMMAP_H_

#define LSCH3_IMMR	0x01000000

// LSCH3_2: ls1028a, lx2162a, lx2160a
#define LSCH3_DDR_ADDR				(LSCH3_IMMR + 0x00080000)
#define LSCH3_DDR2_ADDR				(LSCH3_IMMR + 0x00090000)
#define LSCH3_DDR3_ADDR				0x08210000
#define LSCH3_GUTS_ADDR				(LSCH3_IMMR + 0x00E00000)
#define LSCH3_PMU_ADDR				(LSCH3_IMMR + 0x00E30000)
#define LSCH3_RST_ADDR_LX21XXA			(LSCH3_IMMR + 0x00e88180)
#define LSCH3_RST_ADDR				(LSCH3_IMMR + 0x00E60000)
#define LSCH3_CH3_CLK_GRPA_ADDR			(LSCH3_IMMR + 0x00300000)
#define LSCH3_CH3_CLK_GRPB_ADDR			(LSCH3_IMMR + 0x00310000)
#define LSCH3_CH3_CLK_CTRL_ADDR			(LSCH3_IMMR + 0x00370000)
#define LSCH3_QSPI_ADDR_LSCH3			(LSCH3_IMMR + 0x010c0000)
#define LSCH3_FSPI_ADDR				(LSCH3_IMMR + 0x010c0000)
#define LSCH3_ESDHC1_BASE_ADDR			(LSCH3_IMMR + 0x01140000)
#define LSCH3_ESDHC2_BASE_ADDR			(LSCH3_IMMR + 0x01150000)
#define LSCH3_IFC_ADDR				(LSCH3_IMMR + 0x01240000)
#define LSCH3_NS16550_COM1			(LSCH3_IMMR + 0x011C0500)
#define LSCH3_NS16550_COM2			(LSCH3_IMMR + 0x011C0600)
#define LSCH3_EDMA_ADDR				(LSCH3_IMMR + 0x012c0000)
#define LSCH3_TIMER_ADDR			(LSCH3_IMMR + 0x013e0000)
#define LSCH3_XHCI_USB1_ADDR			(LSCH3_IMMR + 0x02100000)
#define LSCH3_XHCI_USB2_ADDR			(LSCH3_IMMR + 0x02110000)
#define LSCH3_AHCI1_ADDR			(LSCH3_IMMR + 0x02200000)
#define LSCH3_AHCI2_ADDR			(LSCH3_IMMR + 0x02210000)
#define LSCH3_AHCI3_ADDR			(LSCH3_IMMR + 0x02220000)
#define LSCH3_AHCI4_ADDR			(LSCH3_IMMR + 0x02230000)
#define LSCH3_CCI400_ADDR			(LSCH3_IMMR + 0x03090000)
#define LSCH3_SEC_ADDR				(LSCH3_IMMR + 0x07000000)
#define LSCH3_SEC_JR0_ADDR			(LSCH3_IMMR + 0x07010000)
#define LSCH3_SEC_JR1_ADDR			(LSCH3_IMMR + 0x07020000)
#define LSCH3_SEC_JR2_ADDR			(LSCH3_IMMR + 0x07030000)
#define LSCH3_SEC_JR3_ADDR			(LSCH3_IMMR + 0x07040000)
#define LSCH3_QDMA_ADDR				(LSCH3_IMMR + 0x07380000)
#define LSCH3_DISPLAY_ADDR			(LSCH3_IMMR + 0x0e080000)
#define LSCH3_GPU_ADDR				(LSCH3_IMMR + 0x0e0c0000)
#define LSCH3_PMU_CLTBENR			(LSCH3_PMU_ADDR + 0x18A0)
#define LSCH3_PCTBENR_OFFSET			(LSCH3_PMU_ADDR + 0x8A0)
#define LSCH3_SVR				(LSCH3_GUTS_ADDR + 0xA4)

#define LSCH3_WRIOP1_ADDR			(LSCH3_IMMR + 0x7B80000)
#define LSCH3_WRIOP1_MDIO1			(LSCH3_WRIOP1_ADDR + 0x16000)
#define LSCH3_WRIOP1_MDIO2			(LSCH3_WRIOP1_ADDR + 0x17000)
#define LSCH3_SERDES_ADDR			(LSCH3_IMMR + 0xEA0000)

#define LSCH3_DCSR_DDR_ADDR			0x70012c000ULL
#define LSCH3_DCSR_DDR2_ADDR			0x70012d000ULL
#define LSCH3_DCSR_DDR3_ADDR			0x700132000ULL

#define LSCH3_I2C1_BASE_ADDR			(LSCH3_IMMR + 0x01000000)
#define LSCH3_I2C2_BASE_ADDR			(LSCH3_IMMR + 0x01010000)
#define LSCH3_I2C3_BASE_ADDR			(LSCH3_IMMR + 0x01020000)
#define LSCH3_I2C4_BASE_ADDR			(LSCH3_IMMR + 0x01030000)
#define LSCH3_I2C5_BASE_ADDR			(LSCH3_IMMR + 0x01040000)
#define LSCH3_I2C6_BASE_ADDR			(LSCH3_IMMR + 0x01050000)
#define LSCH3_I2C7_BASE_ADDR			(LSCH3_IMMR + 0x01060000)
#define LSCH3_I2C8_BASE_ADDR			(LSCH3_IMMR + 0x01070000)

/* EDMA */
#define LSCH3_EDMA_BASE_ADDR			(LSCH3_IMMR + 0x012c0000)

/* MMU 500 */
#define LSCH3_SMMU_SCR0				(SMMU_BASE + 0x0)
#define LSCH3_SMMU_SCR1				(SMMU_BASE + 0x4)
#define LSCH3_SMMU_SCR2				(SMMU_BASE + 0x8)
#define LSCH3_SMMU_SACR				(SMMU_BASE + 0x10)
#define LSCH3_SMMU_IDR0				(SMMU_BASE + 0x20)
#define LSCH3_SMMU_IDR1				(SMMU_BASE + 0x24)

#define LSCH3_SMMU_NSCR0			(SMMU_BASE + 0x400)
#define LSCH3_SMMU_NSCR2			(SMMU_BASE + 0x408)
#define LSCH3_SMMU_NSACR			(SMMU_BASE + 0x410)

/* Device Configuration */
#define LSCH3_DCFG_BASE				0x01e00000
#define LSCH3_DCFG_PORSR1			0x000
#define LSCH3_DCFG_PORSR1_RCW_SRC		0xff800000
#define LSCH3_DCFG_PORSR1_RCW_SRC_SDHC1		0x04000000
#define LSCH3_DCFG_PORSR1_RCW_SRC_SDHC2		0x04800000
#define LSCH3_DCFG_PORSR1_RCW_SRC_I2C		0x05000000
#define LSCH3_DCFG_PORSR1_RCW_SRC_FSPI_NOR	0x07800000
#define LSCH3_DCFG_PORSR1_RCW_SRC_NOR		0x12f00000
#define LSCH3_DCFG_RCWSR12			0x12c
#define LSCH3_DCFG_RCWSR12_SDHC_SHIFT		24
#define LSCH3_DCFG_RCWSR12_SDHC_MASK		0x7
#define LSCH3_DCFG_RCWSR13			0x130
#define LSCH3_DCFG_RCWSR13_SDHC_SHIFT		3
#define LSCH3_DCFG_RCWSR13_SDHC_MASK		0x7
#define LSCH3_DCFG_RCWSR13_DSPI			(0 << 8)
#define LSCH3_DCFG_RCWSR15			0x138
#define LSCH3_DCFG_RCWSR15_IFCGRPABASE_QSPI	0x3

#define LSCH3_DCFG_DCSR_BASE			0X700100000ULL
#define LSCH3_DCFG_DCSR_PORCR1			0x000

/* Supplemental Configuration */
#define LSCH3_SCFG_BASE				0x01fc0000
#define LSCH3_SCFG_USB3PRM1CR			0x000
#define LSCH3_SCFG_USB3PRM1CR_INIT		0x27672b2a
#define LSCH3_SCFG_USB_TXVREFTUNE		0x9
#define LSCH3_SCFG_USB_SQRXTUNE_MASK		0x7
#define LSCH3_SCFG_QSPICLKCTLR			0x10

#define LSCH3_DCSR_BASE				0x700000000ULL
#define LSCH3_DCSR_USB_PHY1			0x4600000
#define LSCH3_DCSR_USB_PHY2			0x4610000
#define LSCH3_DCSR_USB_PHY_RX_OVRD_IN_HI	0x200C
#define LSCH3_DCSR_USB_IOCR1			0x108004
#define LSCH3_DCSR_USB_PCSTXSWINGFULL		0x71

#ifndef __ASSEMBLY__

/* Global Utilities Block */
struct lsch3_ccsr_gur {
	u32	porsr1;		/* POR status 1 */
	u32	porsr2;		/* POR status 2 */
	u8	res_008[0x20-0x8];
	u32	gpporcr1;	/* General-purpose POR configuration */
	u32	gpporcr2;	/* General-purpose POR configuration 2 */
	u32	gpporcr3;
	u32	gpporcr4;
	u8	res_030[0x60-0x30];
	u32	dcfg_fusesr;	/* Fuse status register */
	u8	res_064[0x70-0x64];
	u32	devdisr;	/* Device disable control 1 */
	u32	devdisr2;	/* Device disable control 2 */
	u32	devdisr3;	/* Device disable control 3 */
	u32	devdisr4;	/* Device disable control 4 */
	u32	devdisr5;	/* Device disable control 5 */
	u32	devdisr6;	/* Device disable control 6 */
	u8	res_088[0x94-0x88];
	u32	coredisr;	/* Device disable control 7 */
	u8	res_098[0xa0-0x98];
	u32	pvr;		/* Processor version */
	u32	svr;		/* System version */
	u8	res_0a8[0x100-0xa8];
	u32	rcwsr[30];	/* Reset control word status */
	u8	res_178[0x200-0x178];
	u32	scratchrw[16];	/* Scratch Read/Write */
	u8	res_240[0x300-0x240];
	u32	scratchw1r[4];	/* Scratch Read (Write once) */
	u8	res_310[0x400-0x310];
	u32	bootlocptrl;	/* Boot location pointer low-order addr */
	u32	bootlocptrh;	/* Boot location pointer high-order addr */
	u8	res_408[0x520-0x408];
	u32	usb1_amqr;
	u32	usb2_amqr;
	u8	res_528[0x530-0x528];	/* add more registers when needed */
	u32	sdmm1_amqr;
	u32	sdmm2_amqr;
	u8	res_538[0x550 - 0x538];	/* add more registers when needed */
	u32	sata1_amqr;
	u32	sata2_amqr;
	u32	sata3_amqr;
	u32	sata4_amqr;
	u8	res_560[0x570 - 0x560];	/* add more registers when needed */
	u32	misc1_amqr;
	u8	res_574[0x590-0x574];	/* add more registers when needed */
	u32	spare1_amqr;
	u32	spare2_amqr;
	u32	spare3_amqr;
	u8	res_59c[0x620 - 0x59c];	/* add more registers when needed */
	u32	gencr[7];	/* General Control Registers */
	u8	res_63c[0x640-0x63c];	/* add more registers when needed */
	u32	cgensr1;	/* Core General Status Register */
	u8	res_644[0x660-0x644];	/* add more registers when needed */
	u32	cgencr1;	/* Core General Control Register */
	u8	res_664[0x740-0x664];	/* add more registers when needed */
	u32	tp_ityp[64];	/* Topology Initiator Type Register */
	struct {
		u32	upper;
		u32	lower;
	} tp_cluster[4];	/* Core cluster n Topology Register */
	u8	res_864[0x920-0x864];	/* add more registers when needed */
	u32 ioqoscr[8];	/*I/O Quality of Services Register */
	u32 uccr;
	u8	res_944[0x960-0x944];	/* add more registers when needed */
	u32 ftmcr;
	u8	res_964[0x990-0x964];	/* add more registers when needed */
	u32 coredisablesr;
	u8	res_994[0xa00-0x994];	/* add more registers when needed */
	u32 sdbgcr; /*Secure Debug Confifuration Register */
	u8	res_a04[0xbf8-0xa04];	/* add more registers when needed */
	u32 ipbrr1;
	u32 ipbrr2;
	u8	res_858[0x1000-0xc00];
};

struct rng4tst {
	u32 rtmctl;		/* misc. control register */
	u32 rtscmisc;		/* statistical check misc. register */
	u32 rtpkrrng;		/* poker range register */
	union {
		u32 rtpkrmax;	/* PRGM=1: poker max. limit register */
		u32 rtpkrsq;	/* PRGM=0: poker square calc. result register */
	};
	u32 rtsdctl;		/* seed control register */
	union {
		u32 rtsblim;	/* PRGM=1: sparse bit limit register */
		u32 rttotsam;	/* PRGM=0: total samples register */
	};
	u32 rtfreqmin;		/* frequency count min. limit register */
	union {
		u32 rtfreqmax;	/* PRGM=1: freq. count max. limit register */
		u32 rtfreqcnt;	/* PRGM=0: freq. count register */
	};
	u32 rsvd1[40];
	u32 rdsta;		/*RNG DRNG Status Register*/
	u32 rsvd2[15];
};

struct version_regs {
	u32 crca;	/* CRCA_VERSION */
	u32 afha;	/* AFHA_VERSION */
	u32 kfha;	/* KFHA_VERSION */
	u32 pkha;	/* PKHA_VERSION */
	u32 aesa;	/* AESA_VERSION */
	u32 mdha;	/* MDHA_VERSION */
	u32 desa;	/* DESA_VERSION */
	u32 snw8a;	/* SNW8A_VERSION */
	u32 snw9a;	/* SNW9A_VERSION */
	u32 zuce;	/* ZUCE_VERSION */
	u32 zuca;	/* ZUCA_VERSION */
	u32 ccha;	/* CCHA_VERSION */
	u32 ptha;	/* PTHA_VERSION */
	u32 rng;	/* RNG_VERSION */
	u32 trng;	/* TRNG_VERSION */
	u32 aaha;	/* AAHA_VERSION */
	u32 rsvd[10];
	u32 sr;		/* SR_VERSION */
	u32 dma;	/* DMA_VERSION */
	u32 ai;		/* AI_VERSION */
	u32 qi;		/* QI_VERSION */
	u32 jr;		/* JR_VERSION */
	u32 deco;	/* DECO_VERSION */
};

struct ccsr_sec {
	u32	res0;
	u32	mcfgr;		/* Master CFG Register */
	u8	res1[0x4];
	u32	scfgr;
	struct {
		u32	ms;	/* Job Ring LIODN Register, MS */
		u32	ls;	/* Job Ring LIODN Register, LS */
	} jrliodnr[4];
	u8	res2[0x2c];
	u32	jrstartr;	/* Job Ring Start Register */
	struct {
		u32	ms;	/* RTIC LIODN Register, MS */
		u32	ls;	/* RTIC LIODN Register, LS */
	} rticliodnr[4];
	u8	res3[0x1c];
	u32	decorr;		/* DECO Request Register */
	struct {
		u32	ms;	/* DECO LIODN Register, MS */
		u32	ls;	/* DECO LIODN Register, LS */
	} decoliodnr[16];
	u32	dar;		/* DECO Avail Register */
	u32	drr;		/* DECO Reset Register */
	u8	res5[0x4d8];
	struct rng4tst rng;	/* RNG Registers */
	u8	res6[0x780];
	struct version_regs vreg; /* version registers since era 10 */
	u8	res7[0xa0];
	u32	crnr_ms;	/* CHA Revision Number Register, MS */
	u32	crnr_ls;	/* CHA Revision Number Register, LS */
	u32	ctpr_ms;	/* Compile Time Parameters Register, MS */
	u32	ctpr_ls;	/* Compile Time Parameters Register, LS */
	u8	res8[0x10];
	u32	far_ms;		/* Fault Address Register, MS */
	u32	far_ls;		/* Fault Address Register, LS */
	u32	falr;		/* Fault Address LIODN Register */
	u32	fadr;		/* Fault Address Detail Register */
	u8	res9[0x4];
	u32	csta;		/* CAAM Status Register */
	u32	smpart;		/* Secure Memory Partition Parameters */
	u32	smvid;		/* Secure Memory Version ID */
	u32	rvid;		/* Run Time Integrity Checking Version ID Reg.*/
	u32	ccbvid;		/* CHA Cluster Block Version ID Register */
	u32	chavid_ms;	/* CHA Version ID Register, MS */
	u32	chavid_ls;	/* CHA Version ID Register, LS */
	u32	chanum_ms;	/* CHA Number Register, MS */
	u32	chanum_ls;	/* CHA Number Register, LS */
	u32	secvid_ms;	/* SEC Version ID Register, MS */
	u32	secvid_ls;	/* SEC Version ID Register, LS */
	u8	res10[0x6f020];
	u32	qilcr_ms;	/* Queue Interface LIODN CFG Register, MS */
	u32	qilcr_ls;	/* Queue Interface LIODN CFG Register, LS */
	u8	res11[0x8ffd8];
};

#endif /*__ASSEMBLY__ */
#endif /* __ARCH_FSL_LSCH3_IMMAP_H_ */
