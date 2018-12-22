/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Based on Linux driver:
 *  Copyright (C) 2003 Atheros Communications, Inc.,  All Rights Reserved.
 *  Copyright (C) 2006 Imre Kaloz <kaloz@openwrt.org>
 *  Copyright (C) 2006-2009 Felix Fietkau <nbd@openwrt.org>
 * Ported to Barebox:
 *  Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
 */

#ifndef AR2312_H
#define AR2312_H

#include <asm/addrspace.h>

/* Address Map */
#define AR2312_SDRAM0		0x00000000
#define AR2312_SDRAM1		0x08000000
#define AR2312_WLAN0		0x18000000
#define AR2312_WLAN1		0x18500000
#define AR2312_ENET0		0x18100000
#define AR2312_ENET1		0x18200000
#define AR2312_SDRAMCTL		0x18300000
#define AR2312_FLASHCTL		0x18400000
#define AR2312_APBBASE		0x1c000000
#define AR2312_FLASH		0x1e000000

#define AR2312_CPU_CLOCK_RATE	180000000
/* Used by romSizeMemory to set SDRAM Memory Refresh */
#define AR2312_SDRAM_CLOCK_RATE	(AR2312_CPU_CLOCK_RATE / 2)
/*
 * SDRAM Memory Refresh (MEM_REF) value is computed as:
 *    15.625us * SDRAM_CLOCK_RATE (in MHZ)
 */
#define DESIRED_MEMORY_REFRESH_NSECS	15625
#define AR2312_SDRAM_MEMORY_REFRESH_VALUE \
	((DESIRED_MEMORY_REFRESH_NSECS * AR2312_SDRAM_CLOCK_RATE/1000000) / 1000)

/*
 * APB Address Map
 */
#define AR2312_UART0		(AR2312_APBBASE + 0x0003) /* high speed uart */
#define AR2312_UART_SHIFT	2
#define AR2312_UART1		(AR2312_APBBASE + 0x1000) /* ar2312 only */
#define AR2312_GPIO		(AR2312_APBBASE + 0x2000)
#define AR2312_RESETTMR		(AR2312_APBBASE + 0x3000)
#define AR2312_APB2AHB		(AR2312_APBBASE + 0x4000)

/*
 * AR2312_NUM_ENET_MAC defines the number of ethernet MACs that
 * should be considered available.  The AR2312 supports 2 enet MACS,
 * even though many reference boards only actually use 1 of them
 * (i.e. Only MAC 0 is actually connected to an enet PHY or PHY switch.
 * The AR2312 supports 1 enet MAC.
 */
#define AR2312_NUM_ENET_MAC	2

/*
 * Need these defines to determine true number of ethernet MACs
 */
#define AR5212_AR2312_REV2	0x52	/* AR2312 WMAC (AP31) */
#define AR5212_AR2312_REV7	0x57	/* AR2312 WMAC (AP30-040) */
#define AR5212_AR2313_REV8	0x58	/* AR2313 WMAC (AP43-030) */

/* Reset/Timer Block Address Map */
#define AR2312_RESETTMR		(AR2312_APBBASE  + 0x3000)
#define AR2312_TIMER		(AR2312_RESETTMR + 0x0000) /* countdown timer */
#define AR2312_WD_CTRL		(AR2312_RESETTMR + 0x0008) /* watchdog cntrl */
#define AR2312_WD_TIMER		(AR2312_RESETTMR + 0x000c) /* watchdog timer */
#define AR2312_ISR		(AR2312_RESETTMR + 0x0010) /* Intr Status Reg */
#define AR2312_IMR		(AR2312_RESETTMR + 0x0014) /* Intr Mask Reg */
#define AR2312_RESET		(AR2312_RESETTMR + 0x0020)
#define AR2312_CLOCKCTL0	(AR2312_RESETTMR + 0x0060)
#define AR2312_CLOCKCTL1	(AR2312_RESETTMR + 0x0064)
#define AR2312_CLOCKCTL2	(AR2312_RESETTMR + 0x0068)
#define AR2312_SCRATCH		(AR2312_RESETTMR + 0x006c)
#define AR2312_PROCADDR		(AR2312_RESETTMR + 0x0070)
#define AR2312_PROC1		(AR2312_RESETTMR + 0x0074)
#define AR2312_DMAADDR		(AR2312_RESETTMR + 0x0078)
#define AR2312_DMA1		(AR2312_RESETTMR + 0x007c)
#define AR2312_ENABLE		(AR2312_RESETTMR + 0x0080) /* interface enb */
#define AR2312_REV		(AR2312_RESETTMR + 0x0090) /* revision */

/* AR2312_WD_CTRL register bit field definitions */
#define AR2312_WD_CTRL_IGNORE_EXPIRATION	0x0000
#define AR2312_WD_CTRL_NMI			0x0001
#define AR2312_WD_CTRL_RESET			0x0002

/* AR2312_ISR register bit field definitions */
#define AR2312_ISR_NONE		0x0000
#define AR2312_ISR_TIMER	0x0001
#define AR2312_ISR_AHBPROC	0x0002
#define AR2312_ISR_AHBDMA	0x0004
#define AR2312_ISR_GPIO		0x0008
#define AR2312_ISR_UART0	0x0010
#define AR2312_ISR_UART0DMA	0x0020
#define AR2312_ISR_WD		0x0040
#define AR2312_ISR_LOCAL	0x0080

/* AR2312_RESET register bit field definitions */
#define AR2312_RESET_SYSTEM	0x00000001  /* cold reset full system */
#define AR2312_RESET_PROC	0x00000002  /* cold reset MIPS core */
#define AR2312_RESET_WLAN0	0x00000004  /* cold reset WLAN MAC and BB */
#define AR2312_RESET_EPHY0	0x00000008  /* cold reset ENET0 phy */
#define AR2312_RESET_EPHY1	0x00000010  /* cold reset ENET1 phy */
#define AR2312_RESET_ENET0	0x00000020  /* cold reset ENET0 mac */
#define AR2312_RESET_ENET1	0x00000040  /* cold reset ENET1 mac */
#define AR2312_RESET_UART0	0x00000100  /* cold reset UART0 (high speed) */
#define AR2312_RESET_WLAN1	0x00000200  /* cold reset WLAN MAC/BB */
#define AR2312_RESET_APB	0x00000400  /* cold reset APB (ar2312) */
#define AR2312_RESET_WARM_PROC	0x00001000  /* warm reset MIPS core */
#define AR2312_RESET_WARM_WLAN0_MAC	0x00002000  /* warm reset WLAN0 MAC */
#define AR2312_RESET_WARM_WLAN0_BB	0x00004000  /* warm reset WLAN0 BaseBand */
#define AR2312_RESET_NMI	0x00010000  /* send an NMI to the processor */
#define AR2312_RESET_WARM_WLAN1_MAC	0x00020000  /* warm reset WLAN1 mac */
#define AR2312_RESET_WARM_WLAN1_BB	0x00040000  /* warm reset WLAN1 baseband */
#define AR2312_RESET_LOCAL_BUS	0x00080000  /* reset local bus */
#define AR2312_RESET_WDOG	0x00100000  /* last reset was a watchdog */

/* Values for AR2312_CLOCKCTL1
 *
 * The AR2312_CLOCKCTL1 register is loaded based on the speed of
 * our incoming clock.  Currently, all valid configurations
 * for an AR2312 use an ar5112 radio clocked at 40MHz.  Until
 * there are other configurations available, we'll hardcode
 * this 40MHz assumption.
 */
#define AR2312_INPUT_CLOCK		40000000
#define AR2312_CLOCKCTL1_IN40_OUT160MHZ	0x0405 /* 40MHz in, 160Mhz out */
#define AR2312_CLOCKCTL1_IN40_OUT180MHZ	0x0915 /* 40MHz in, 180Mhz out */
#define AR2312_CLOCKCTL1_IN40_OUT200MHZ	0x1935 /* 40MHz in, 200Mhz out */
#define AR2312_CLOCKCTL1_IN40_OUT220MHZ	0x0b15 /* 40MHz in, 220Mhz out */
#define AR2312_CLOCKCTL1_IN40_OUT240MHZ	0x0605 /* 40MHz in, 240Mhz out */

#define AR2312_CLOCKCTL1_SELECTION AR2312_CLOCKCTL1_IN40_OUT180MHZ
#define AR2312_CPU_CLOCK_RATE		180000000

/*
 * Special values for AR2313 'VIPER' PLL.
 *
 * These values do not match the latest datasheet for the AR2313,
 * which appears to be an exact copy of the AR2312 in this area.
 * The values were derived from the ECOS code provided in the Atheros
 * LSDK-1.0 (and confirmed by checking values on an AR2313 reference
 * design).
 */
#define AR2313_CLOCKCTL1_SELECTION	0x91245

/* Bit fields for AR2312_CLOCKCTL2 */
#define AR2312_CLOCKCTL2_WANT_RESET	0x00000001   /* reset with new vals */
#define AR2312_CLOCKCTL2_WANT_DIV2	0x00000010   /* request /2 clock */
#define AR2312_CLOCKCTL2_WANT_DIV4	0x00000020   /* request /4 clock */
#define AR2312_CLOCKCTL2_WANT_PLL_BYPASS 0x00000080   /* request PLL bypass */
#define AR2312_CLOCKCTL2_STATUS_DIV2	0x10000000   /* have /2 clock */
#define AR2312_CLOCKCTL2_STATUS_DIV4	0x20000000   /* have /4 clock */
#define AR2312_CLOCKCTL2_STATUS_PLL_BYPASS 0x80000000   /* PLL is bypassed */

/* AR2312_CLOCKCTL1 register bit field definitions */
#define AR2312_CLOCKCTL1_PREDIVIDE_MASK		0x00000030
#define AR2312_CLOCKCTL1_PREDIVIDE_SHIFT	4
#define AR2312_CLOCKCTL1_MULTIPLIER_MASK	0x00001f00
#define AR2312_CLOCKCTL1_MULTIPLIER_SHIFT	8
#define AR2312_CLOCKCTL1_DOUBLER_MASK		0x00010000

/* Valid for AR2313 */
#define AR2313_CLOCKCTL1_PREDIVIDE_MASK		0x00003000
#define AR2313_CLOCKCTL1_PREDIVIDE_SHIFT	12
#define AR2313_CLOCKCTL1_MULTIPLIER_MASK	0x001f0000
#define AR2313_CLOCKCTL1_MULTIPLIER_SHIFT	16
#define AR2313_CLOCKCTL1_DOUBLER_MASK		0x00000000

/* Values for AR2312_CLOCKCTL */
#define AR2312_CLOCKCTL_ETH0	0x0004	/* enable eth0 clock */
#define AR2312_CLOCKCTL_ETH1	0x0008	/* enable eth1 clock */
#define AR2312_CLOCKCTL_UART0	0x0010	/* enable UART0 external clock */


/* AR2312_ENABLE register bit field definitions */
#define AR2312_ENABLE_ENET0		0x0002
#define AR2312_ENABLE_ENET1		0x0004

/* AR2312_REV register bit field definitions */
#define AR2312_REV_WMAC_MAJ	0xf000
#define AR2312_REV_WMAC_MAJ_S	12
#define AR2312_REV_WMAC_MIN	0x0f00
#define AR2312_REV_WMAC_MIN_S	8
#define AR2312_REV_MAJ		0x00f0
#define AR2312_REV_MAJ_S	4
#define AR2312_REV_MIN		0x000f
#define AR2312_REV_MIN_S	0
#define AR2312_REV_CHIP	(AR2312_REV_MAJ|AR2312_REV_MIN)

/* Major revision numbers, bits 7..4 of Revision ID register */
#define AR2312_REV_MAJ_AR2312	0x4
#define AR2312_REV_MAJ_AR2313	0x5

/* Minor revision numbers, bits 3..0 of Revision ID register */
#define AR2312_REV_MIN_DUAL	0x0	/* Dual WLAN version */
#define AR2312_REV_MIN_SINGLE	0x1	/* Single WLAN version */

/* AR2312_FLASHCTL register bit field definitions */
#define FLASHCTL_IDCY	0x0000000f	/* Idle cycle turn around time */
#define FLASHCTL_IDCY_S	0
#define FLASHCTL_WST1	0x000003e0	/* Wait state 1 */
#define FLASHCTL_WST1_S	5
#define FLASHCTL_RBLE	0x00000400	/* Read byte lane enable */
#define FLASHCTL_WST2	0x0000f800	/* Wait state 2 */
#define FLASHCTL_WST2_S	11
#define FLASHCTL_AC	0x00070000	/* Flash address check (added) */
#define FLASHCTL_AC_S	16
#define FLASHCTL_AC_128K	0x00000000
#define FLASHCTL_AC_256K	0x00010000
#define FLASHCTL_AC_512K	0x00020000
#define FLASHCTL_AC_1M	0x00030000
#define FLASHCTL_AC_2M	0x00040000
#define FLASHCTL_AC_4M	0x00050000
#define FLASHCTL_AC_8M	0x00060000
#define FLASHCTL_AC_RES	0x00070000	/* 16MB is not supported */
#define FLASHCTL_E	0x00080000	/* Flash bank enable (added) */
#define FLASHCTL_BUSERR	0x01000000	/* Bus transfer error status flag */
#define FLASHCTL_WPERR	0x02000000	/* Write protect error status flag */
#define FLASHCTL_WP	0x04000000	/* Write protect */
#define FLASHCTL_BM	0x08000000	/* Burst mode */
#define FLASHCTL_MW	0x30000000	/* Memory width */
#define FLASHCTL_MWx8	0x00000000	/* Memory width x8 */
#define FLASHCTL_MWx16	0x10000000	/* Memory width x16 */
#define FLASHCTL_MWx32	0x20000000	/* Memory width x32 (not supported) */
#define FLASHCTL_ATNR	0x00000000	/* Access type == no retry */
#define FLASHCTL_ATR	0x80000000	/* Access type == retry every */
#define FLASHCTL_ATR4	0xc0000000	/* Access type == retry every 4 */

#define AR2312_MAX_FLASH_SIZE	0x800000

/* ARM Flash Controller -- 3 flash banks with either x8 or x16 devices.  */
#define AR2312_FLASHCTL0	(AR2312_FLASHCTL + 0x00)
#define AR2312_FLASHCTL1	(AR2312_FLASHCTL + 0x04)
#define AR2312_FLASHCTL2	(AR2312_FLASHCTL + 0x08)

/* ARM SDRAM Controller -- just enough to determine memory size */
#define AR2312_MEM_CFG0	(AR2312_SDRAMCTL + 0x00)
#define AR2312_MEM_CFG1	(AR2312_SDRAMCTL + 0x04)
#define AR2312_MEM_REF	(AR2312_SDRAMCTL + 0x08)	/* 16 bit value */

#define MEM_CFG0_F0	0x00000002	/* bank 0: 256Mb support */
#define MEM_CFG0_T0	0x00000004	/* bank 0: chip width */
#define MEM_CFG0_B0	0x00000008	/* bank 0: 2 vs 4 bank */
#define MEM_CFG0_F1	0x00000020	/* bank 1: 256Mb support */
#define MEM_CFG0_T1	0x00000040	/* bank 1: chip width */
#define MEM_CFG0_B1	0x00000080	/* bank 1: 2 vs 4 bank */
					/* bank 2 and 3 are not supported */
#define MEM_CFG0_E	0x00020000	/* SDRAM clock control */
#define MEM_CFG0_C	0x00040000	/* SDRAM clock enable */
#define MEM_CFG0_X	0x00080000	/* bus width (0 == 32b) */
#define MEM_CFG0_CAS	0x00300000	/* CAS latency (1-3) */
#define MEM_CFG0_C1	0x00100000
#define MEM_CFG0_C2	0x00200000
#define MEM_CFG0_C3	0x00300000
#define MEM_CFG0_R	0x00c00000	/* RAS to CAS latency (1-3) */
#define MEM_CFG0_R1	0x00400000
#define MEM_CFG0_R2	0x00800000
#define MEM_CFG0_R3	0x00c00000
#define MEM_CFG0_A	0x01000000	/* AHB auto pre-charge */

#define MEM_CFG1_I	0x0001	/* memory init control */
#define MEM_CFG1_M	0x0002	/* memory init control */
#define MEM_CFG1_R	0x0004	/* read buffer enable (unused) */
#define MEM_CFG1_W	0x0008	/* write buffer enable (unused) */
#define MEM_CFG1_B	0x0010	/* SDRAM engine busy */
#define MEM_CFG1_AC_2	0	/* AC of 2MB */
#define MEM_CFG1_AC_4	1	/* AC of 4MB */
#define MEM_CFG1_AC_8	2	/* AC of 8MB */
#define MEM_CFG1_AC_16	3	/* AC of 16MB */
#define MEM_CFG1_AC_32	4	/* AC of 32MB */
#define MEM_CFG1_AC_64	5	/* AC of 64MB */
#define MEM_CFG1_AC_128	6	/* AC of 128MB */

#define MEM_CFG1_AC0_S	8
#define MEM_CFG1_AC0	0x0700	/* bank 0: SDRAM addr check (added) */
#define MEM_CFG1_E0	0x0800	/* bank 0: enable */
#define MEM_CFG1_AC1_S	12
#define MEM_CFG1_AC1	0x7000	/* bank 1: SDRAM addr check (added) */
#define MEM_CFG1_E1	0x8000	/* bank 1: enable */

/* GPIO Address Map */
#define AR2312_GPIO	(AR2312_APBBASE  + 0x2000)
#define AR2312_GPIO_DO	(AR2312_GPIO + 0x00)	/* output register */
#define AR2312_GPIO_DI	(AR2312_GPIO + 0x04)	/* intput register */
#define AR2312_GPIO_CR	(AR2312_GPIO + 0x08)	/* control register */

/* GPIO Control Register bit field definitions */
#define AR2312_GPIO_CR_M(x)    (1 << (x))	/* mask for i/o */
#define AR2312_GPIO_CR_O(x)    (0 << (x))	/* mask for output */
#define AR2312_GPIO_CR_I(x)    (1 << (x))	/* mask for input */
#define AR2312_GPIO_CR_INT(x)  (1 << ((x)+8))	/* mask for interrupt */
#define AR2312_GPIO_CR_UART(x) (1 << ((x)+16))	/* uart multiplex */
#define AR2312_NUM_GPIO		8

#endif
