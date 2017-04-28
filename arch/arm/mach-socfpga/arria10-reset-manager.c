/*
 * Copyright (C) 2014-2016 Altera Corporation <www.altera.com>
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <errno.h>
#include <io.h>
#include <mach/arria10-pinmux.h>
#include <mach/arria10-regs.h>
#include <mach/arria10-reset-manager.h>
#include <mach/arria10-system-manager.h>

void arria10_reset_peripherals(void)
{
	unsigned mask_ecc_ocp = ARRIA10_RSTMGR_PER0MODRST_EMAC0OCP |
		ARRIA10_RSTMGR_PER0MODRST_EMAC1OCP |
		ARRIA10_RSTMGR_PER0MODRST_EMAC2OCP |
		ARRIA10_RSTMGR_PER0MODRST_USB0OCP |
		ARRIA10_RSTMGR_PER0MODRST_USB1OCP |
		ARRIA10_RSTMGR_PER0MODRST_NANDOCP |
		ARRIA10_RSTMGR_PER0MODRST_QSPIOCP |
		ARRIA10_RSTMGR_PER0MODRST_SDMMCOCP;

	/* disable all components except ECC_OCP, L4 Timer0 and L4 WD0 */
	writel(0xffffffff, ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER1MODRST);
	setbits_le32(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER0MODRST,
		     ~mask_ecc_ocp);

	/* Finally disable the ECC_OCP */
	setbits_le32(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER0MODRST,
		     mask_ecc_ocp);
}

void arria10_reset_deassert_dedicated_peripherals(void)
{
	uint32_t mask;

	mask = ARRIA10_RSTMGR_PER0MODRST_SDMMCOCP |
	       ARRIA10_RSTMGR_PER0MODRST_QSPIOCP |
	       ARRIA10_RSTMGR_PER0MODRST_NANDOCP |
	       ARRIA10_RSTMGR_PER0MODRST_DMAOCP;

	/* enable ECC OCP first */
	clrbits_le32(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER0MODRST, mask);

	mask = ARRIA10_RSTMGR_PER0MODRST_SDMMC |
	       ARRIA10_RSTMGR_PER0MODRST_QSPI |
	       ARRIA10_RSTMGR_PER0MODRST_NAND |
	       ARRIA10_RSTMGR_PER0MODRST_DMA;

	clrbits_le32(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER0MODRST, mask);

	mask = ARRIA10_RSTMGR_PER1MODRST_L4SYSTIMER0 |
	       ARRIA10_RSTMGR_PER1MODRST_UART1 |
	       ARRIA10_RSTMGR_PER1MODRST_UART0;

	clrbits_le32(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER1MODRST, mask);

	clrbits_le32(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER0MODRST,
		     ARRIA10_RSTMGR_OCP_MASK);
	mask = ARRIA10_RSTMGR_PER0MODRST_EMAC1 |
		ARRIA10_RSTMGR_PER0MODRST_EMAC2 |
		ARRIA10_RSTMGR_PER0MODRST_EMAC0 |
		ARRIA10_RSTMGR_PER0MODRST_SPIS0 |
		ARRIA10_RSTMGR_PER0MODRST_SPIM0;
	clrbits_le32(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER0MODRST, mask);

	mask = ARRIA10_RSTMGR_PER1MODRST_I2C3 |
	       ARRIA10_RSTMGR_PER1MODRST_I2C4 |
	       ARRIA10_RSTMGR_PER1MODRST_I2C2 |
	       ARRIA10_RSTMGR_PER1MODRST_UART1 |
	       ARRIA10_RSTMGR_PER1MODRST_GPIO2;
	clrbits_le32(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER1MODRST, mask);
}

static const uint32_t per0fpgamasks[] = {
	ARRIA10_RSTMGR_PER0MODRST_EMAC0OCP | ARRIA10_RSTMGR_PER0MODRST_EMAC0,
	ARRIA10_RSTMGR_PER0MODRST_EMAC1OCP | ARRIA10_RSTMGR_PER0MODRST_EMAC1,
	ARRIA10_RSTMGR_PER0MODRST_EMAC2OCP | ARRIA10_RSTMGR_PER0MODRST_EMAC2,
	0, /* i2c0 per0mod */
	0, /* i2c1 per0mod */
	0, /* i2c0_emac */
	0, /* i2c1_emac */
	0, /* i2c2_emac */
	ARRIA10_RSTMGR_PER0MODRST_NANDOCP | ARRIA10_RSTMGR_PER0MODRST_NAND,
	ARRIA10_RSTMGR_PER0MODRST_QSPIOCP | ARRIA10_RSTMGR_PER0MODRST_QSPI,
	ARRIA10_RSTMGR_PER0MODRST_SDMMCOCP | ARRIA10_RSTMGR_PER0MODRST_SDMMC,
	ARRIA10_RSTMGR_PER0MODRST_SPIM0,
	ARRIA10_RSTMGR_PER0MODRST_SPIM1,
	ARRIA10_RSTMGR_PER0MODRST_SPIS0,
	ARRIA10_RSTMGR_PER0MODRST_SPIS1,
	0, /* uart0 per0mod */
	0, /* uart1 per0mod */
};

static const uint32_t per1fpgamasks[] = {
	0, /* emac0 per0mod */
	0, /* emac1 per0mod */
	0, /* emac2 per0mod */
	ARRIA10_RSTMGR_PER1MODRST_I2C0,
	ARRIA10_RSTMGR_PER1MODRST_I2C1,
	ARRIA10_RSTMGR_PER1MODRST_I2C2,
	ARRIA10_RSTMGR_PER1MODRST_I2C3,
	ARRIA10_RSTMGR_PER1MODRST_I2C4,
	0, /* nand per0mod */
	0, /* qspi per0mod */
	0, /* sdmmc per0mod */
	0, /* spim0 per0mod */
	0, /* spim1 per0mod */
	0, /* spis0 per0mod */
	0, /* spis1 per0mod */
	ARRIA10_RSTMGR_PER1MODRST_UART0,
	ARRIA10_RSTMGR_PER1MODRST_UART1,
};

void arria10_reset_deassert_fpga_peripherals(void)
{
	uint32_t mask0 = 0;
	uint32_t mask1 = 0;
	uint32_t fpga_pinux_addr = ARRIA10_PINMUX_FPGA_INTERFACE_ADDR;
	int i;

	for (i = 0; i < ARRAY_SIZE(per1fpgamasks); i++) {
		if (readl(fpga_pinux_addr)) {
			mask0 |= per0fpgamasks[i];
			mask1 |= per1fpgamasks[i];
		}
		fpga_pinux_addr += sizeof(uint32_t);
	}

	clrbits_le32(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER0MODRST,
		     mask0 & ARRIA10_RSTMGR_OCP_MASK);
	clrbits_le32(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER0MODRST, mask0);
	clrbits_le32(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER1MODRST, mask1);
}

void arria10_reset_deassert_shared_peripherals_q1(uint32_t *mask0,
						  uint32_t *mask1)
{
	uint32_t pinmux_addr = ARRIA10_PINMUX_SHARED_3V_IO_GRP_ADDR;
	int q1;

	for (q1 = 1; q1 <= 12; q1++, pinmux_addr += sizeof(uint32_t)) {
		switch (readl(pinmux_addr)) {
		case ARRIA10_PINMUX_SHARED_IO_Q1_GPIO:
			*mask1 |= ARRIA10_RSTMGR_PER1MODRST_GPIO0;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q1_NAND:
			*mask0 |= ARRIA10_RSTMGR_PER0MODRST_NANDOCP|
				ARRIA10_RSTMGR_PER0MODRST_NAND;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q1_UART:
			if ((q1 >= 1) && (q1 <= 4))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_UART0;
			else if ((q1 >= 5) && (q1 <= 8))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_UART1;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q1_QSPI:
			if ((q1 >= 5) && (q1 <= 6))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_QSPIOCP |
					  ARRIA10_RSTMGR_PER0MODRST_QSPI;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q1_USB:
			*mask0 |= ARRIA10_RSTMGR_PER0MODRST_USB0OCP |
			    ARRIA10_RSTMGR_PER0MODRST_USB0;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q1_SDMMC:
			if ((q1 >= 1) && (q1 <= 10))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_SDMMCOCP |
					  ARRIA10_RSTMGR_PER0MODRST_SDMMC;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q1_SPIM:
			if ((q1 == 1) || ((q1 >= 5) && (q1 <= 8)))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_SPIM0;
			else if ((q1 == 2) || ((q1 >= 9) && (q1 <= 12)))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_SPIM1;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q1_SPIS:
			if ((q1 >= 1) && (q1 <= 4))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_SPIS0;
			else if ((q1 >= 9) && (q1 <= 12))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_SPIS1;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q1_EMAC:
			if ((q1 == 7) || (q1 == 8))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_EMAC2OCP |
					  ARRIA10_RSTMGR_PER0MODRST_EMAC2;
			else if ((q1 == 9) || (q1 == 10))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_EMAC1OCP |
					  ARRIA10_RSTMGR_PER0MODRST_EMAC1;
			else if ((q1 == 11) || (1 == 12))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_EMAC0OCP |
					  ARRIA10_RSTMGR_PER0MODRST_EMAC0;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q1_I2C:
			if ((q1 == 3) || (q1 == 4))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C1;
			else if ((q1 == 5) || (q1 == 6))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C0;
			else if ((q1 == 7) || (q1 == 8))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C4;
			else if ((q1 == 9) || (q1 == 10))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C3;
			else if ((q1 == 11) || (q1 == 12))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C2;
			break;
		}
	}
}

void arria10_reset_deassert_shared_peripherals_q2(uint32_t *mask0,
						  uint32_t *mask1)
{
	uint32_t pinmux_addr = ARRIA10_PINMUX_SHARED_3V_IO_GRP_ADDR;
	int q2;

	for (q2 = 1; q2 <= 12; q2++, pinmux_addr += sizeof(uint32_t)) {
		switch (readl(pinmux_addr)) {
		case ARRIA10_PINMUX_SHARED_IO_Q2_GPIO:
			*mask1 |= ARRIA10_RSTMGR_PER1MODRST_GPIO0;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q2_NAND:
			if ((q2 != 4) && (q2 != 5))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_NANDOCP |
					  ARRIA10_RSTMGR_PER0MODRST_NAND;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q2_UART:
			if ((q2 >= 9) && (q2 <= 12))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_UART0;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q2_USB:
			*mask0 |= ARRIA10_RSTMGR_PER0MODRST_USB1OCP |
				  ARRIA10_RSTMGR_PER0MODRST_USB1;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q2_EMAC:
			*mask0 |= ARRIA10_RSTMGR_PER0MODRST_EMAC0OCP |
				  ARRIA10_RSTMGR_PER0MODRST_EMAC0;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q2_SPIM:
			if ((q2 >= 8) && (q2 <= 12))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_SPIM1;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q2_SPIS:
			if ((q2 >= 9) && (q2 <= 12))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_SPIS0;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q2_I2C:
			if ((q2 == 9) || (q2 == 10))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C1;
			else if ((q2 == 11) || (q2 == 12))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C0;
			break;
		}
	}
}

void arria10_reset_deassert_shared_peripherals_q3(uint32_t *mask0,
						  uint32_t *mask1)
{
	uint32_t pinmux_addr = ARRIA10_PINMUX_SHARED_3V_IO_GRP_ADDR;
	int q3;

	for (q3 = 1; q3 <= 12; q3++, pinmux_addr += sizeof(uint32_t)) {
		switch (readl(pinmux_addr)) {
		case ARRIA10_PINMUX_SHARED_IO_Q3_GPIO:
			*mask1 |= ARRIA10_RSTMGR_PER1MODRST_GPIO1;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q3_NAND:
			*mask0 |= ARRIA10_RSTMGR_PER0MODRST_NANDOCP |
				  ARRIA10_RSTMGR_PER0MODRST_NAND;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q3_UART:
			if ((q3 >= 1) && (q3 <= 4))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_UART0;
			else if ((q3 >= 5) && (q3 <= 8))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_UART1;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q3_EMAC1:
			*mask0 |= ARRIA10_RSTMGR_PER0MODRST_EMAC1OCP |
				  ARRIA10_RSTMGR_PER0MODRST_EMAC1;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q3_SPIM:
			if ((q3 >= 1) && (q3 <= 5))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_SPIM1;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q3_SPIS:
			if ((q3 >= 5) && (q3 <= 8))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_SPIS1;
			else if ((q3 >= 9) && (q3 <= 12))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_SPIS0;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q3_EMAC0:
			if ((q3 == 9) || (q3 == 10))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_EMAC2OCP |
					  ARRIA10_RSTMGR_PER0MODRST_EMAC2;
			else if ((q3 == 11) || (q3 == 12))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_EMAC0OCP |
					  ARRIA10_RSTMGR_PER0MODRST_EMAC0;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q3_I2C:
			if ((q3 == 7) || (q3 == 8))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C1;
			else if ((q3 == 3) || (q3 == 4))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C0;
			else if ((q3 == 9) || (q3 == 10))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C4;
			else if ((q3 == 11) || (q3 == 12))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C2;
			break;
		}
	}
}

void arria10_reset_deassert_shared_peripherals_q4(uint32_t *mask0, uint32_t *mask1)
{
	uint32_t pinmux_addr = ARRIA10_PINMUX_SHARED_3V_IO_GRP_ADDR;
	int q4;

	for (q4 = 1; q4 <= 12; q4++, pinmux_addr += sizeof(uint32_t)) {
		switch (readl(pinmux_addr)) {
		case ARRIA10_PINMUX_SHARED_IO_Q4_GPIO:
			*mask1 |= ARRIA10_RSTMGR_PER1MODRST_GPIO1;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q4_NAND:
			if (q4 != 4)
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_NANDOCP |
					  ARRIA10_RSTMGR_PER0MODRST_NAND;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q4_UART:
			if ((q4 >= 3) && (q4 <= 6))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_UART1;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q4_QSPI:
			if ((q4 == 5) || (q4 == 6))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_QSPIOCP |
					  ARRIA10_RSTMGR_PER0MODRST_QSPI;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q4_EMAC1:
			*mask0 |= ARRIA10_RSTMGR_PER0MODRST_EMAC2OCP |
				  ARRIA10_RSTMGR_PER0MODRST_EMAC2;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q4_SDMMC:
			if ((q4 >= 1) && (q4 <= 6))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_SDMMCOCP |
					  ARRIA10_RSTMGR_PER0MODRST_SDMMC;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q4_SPIM:
			if ((q4 >= 6) && (q4 <= 12))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_SPIM0;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q4_SPIS:
			if ((q4 >= 9) && (q4 <= 12))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_SPIS1;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q4_EMAC0:
			if ((q4 == 7) || (q4 == 8))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_EMAC1OCP |
					  ARRIA10_RSTMGR_PER0MODRST_EMAC1;
			else if ((q4 == 11) || (q4 == 12))
				*mask0 |= ARRIA10_RSTMGR_PER0MODRST_EMAC0OCP |
					  ARRIA10_RSTMGR_PER0MODRST_EMAC0;
			break;
		case ARRIA10_PINMUX_SHARED_IO_Q4_I2C:
			if ((q4 == 1) || (q4 == 2))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C1;
			else if ((q4 == 7) || (q4 == 8))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C3;
			else if ((q4 == 9) || (q4 == 10))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C4;
			else if ((q4 == 11) || (q4 == 12))
				*mask1 |= ARRIA10_RSTMGR_PER1MODRST_I2C2;
			break;
		}
	}
}

void arria10_reset_deassert_shared_peripherals(void)
{
	uint32_t mask0 = 0;
	uint32_t mask1 = 0;

	arria10_reset_deassert_shared_peripherals_q1(&mask0, &mask1);
	arria10_reset_deassert_shared_peripherals_q2(&mask0, &mask1);
	arria10_reset_deassert_shared_peripherals_q3(&mask0, &mask1);
	arria10_reset_deassert_shared_peripherals_q4(&mask0, &mask1);

	mask1 |= ARRIA10_RSTMGR_PER1MODRST_WATCHDOG1 |
		 ARRIA10_RSTMGR_PER1MODRST_L4SYSTIMER1 |
		 ARRIA10_RSTMGR_PER1MODRST_SPTIMER0 |
		 ARRIA10_RSTMGR_PER1MODRST_SPTIMER1;

	clrbits_le32(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER0MODRST,
		     mask0 & ARRIA10_RSTMGR_OCP_MASK);
	clrbits_le32(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER1MODRST, mask1);
	clrbits_le32(ARRIA10_RSTMGR_ADDR + ARRIA10_RSTMGR_PER0MODRST, mask0);
}
