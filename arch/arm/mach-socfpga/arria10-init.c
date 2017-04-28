/*
 * Copyright (C) 2014 Altera Corporation <www.altera.com>
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <debug_ll.h>
#include <mach/arria10-regs.h>
#include <mach/arria10-clock-manager.h>
#include <mach/arria10-pinmux.h>
#include <mach/arria10-reset-manager.h>
#include <mach/arria10-system-manager.h>
#include <mach/generic.h>
#include <asm/io.h>
#include <asm/cache-l2x0.h>
#include <asm/system.h>

#define L310_AUX_CTRL_EARLY_BRESP		BIT(30)	/* R2P0+ */
#define L310_AUX_CTRL_NS_LOCKDOWN		BIT(26)
#define L310_AUX_CTRL_FULL_LINE_ZERO		BIT(0)	/* R2P0+ */

static inline void set_auxcr(unsigned int val)
{
	asm volatile("mcr p15, 0, %0, c1, c0, 1	@ set AUXCR"
		     : : "r" (val));
	isb();
}

static inline unsigned int get_auxcr(void)
{
	unsigned int val;

	asm("mrc p15, 0, %0, c1, c0, 1	@ get AUXCR" : "=r" (val));
	return val;
}

static void l2c310_disable(void __iomem *base)
{
	u32 aux;
	int ways = 8;

	aux = readl(base + L2X0_AUX_CTRL);

	/*
	 * If full-line-of-zeros is enabled, we must first disable it in the
	 * Cortex-A9 auxiliary control register before disabling the L2 cache.
	 */
	if (aux & L310_AUX_CTRL_FULL_LINE_ZERO)
		set_auxcr(get_auxcr() & ~(BIT(3) | BIT(2) | BIT(1)));

	/* flush all ways */
	writel((1 << ways) - 1, base + L2X0_INV_WAY);

	while (readl(base + L2X0_INV_WAY) & ways)
		;

	/* sync */
	writel(0, base + L2X0_CACHE_SYNC);

	/* disable */
	writel(0, base + L2X0_CTRL);
	dsb();
}

static void arria10_initialize_security_policies(void)
{
	void __iomem *l2x0_base = (void __iomem *) 0xfffff000;

	/* BootROM leaves the L2X0 in a weird state. Always disable L2X0 for now. */
	l2c310_disable(l2x0_base);

	/* Put OCRAM in non-secure */
	writel(0x003f0000, ARRIA10_NOC_FW_OCRAM_OCRAM_SCR_REGION0);
	writel(0x1, ARRIA10_NOC_FW_OCRAM_OCRAM_SCR_EN);

	/* Put DDR in non-secure */
	writel(0xffff0000, ARRIA10_NOC_FW_DDR_L3_DDR_SCR_REGION0);
	writel(0x1, ARRIA10_NOC_FW_DDR_L3_DDR_SCR_EN);

	/* Enable priviledge and non priviledge access to L4 peripherals */
	writel(0xffffffff, ARRIA10_NOC_L4_PRIV_L4_PRIV_L4_PRIV);

	/* Enable secure and non secure transaction to bridges */
	writel(0xffffffff, ARRIA10_NOC_FW_SOC2FPGA_SOC2FPGA_SCR_LWSOC2FPGA);
	writel(0xffffffff, ARRIA10_NOC_FW_SOC2FPGA_SOC2FPGA_SCR_SOC2FPGA);

	/* allow non-secure and secure transaction from/to all peripherals */
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_NAND_REG);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_NAND_DATA);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_QSPI_DATA);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_USB0_REG);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_USB1_REG);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_SPIM0);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_SPIM1);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_SPIS0);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_SPIS1);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_EMAC0);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_EMAC1);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_EMAC2);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_EMAC3);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_QSPI);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_SDMMC);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_GPIO0);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_GPIO1);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_GPIO2);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_I2C0);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_I2C1);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_I2C2);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_I2C3);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_I2C4);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_SPTIMER0);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_SPTIMER1);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_UART0);
	writel(0xffffffff, ARRIA10_NOC_FW_L4_PER_SCR_UART1);

	/* Return error instead of random data */
	writel(0x1, ARRIA10_NOC_FW_DDR_L3_DDR_SCR_GLOBAL);
}

static void arria10_mask_ecc_errors(void)
{
	writel(0x0007FFFF, ARRIA10_SYSMGR_ADDR + 0x94);
}

/*
 * First C function to initialize the critical hardware early
 */
void arria10_init(struct arria10_mainpll_cfg *mainpll,
		  struct arria10_perpll_cfg *perpll,
		  uint32_t *pinmux)
{
	int i;

	arria10_cm_use_intosc();

	arria10_initialize_security_policies();

	arria10_mask_ecc_errors();

	/*
	 * Configure the L2 controller to make SDRAM start at 0.
	 * Set address filtering start to 0x0 (Bits [31:20]),
	 * Enable address filtering (Bit[0])
	 */
	writel(0x00000001, ARRIA10_MPUL2_ADRFLTR_START);
	writel(0x00000002, ARRIA10_SYSMGR_NOC_ADDR_REMAP_VALUE);

	arria10_reset_peripherals();

	/* timer init */
	writel(0xffffffff, ARRIA10_OSC1TIMER0_ADDR);
	writel(0xffffffff, ARRIA10_OSC1TIMER0_ADDR + 0x4);
	writel(0x00000003, ARRIA10_OSC1TIMER0_ADDR + 0x8);

	/* configuring the clock based on handoff */
	arria10_cm_basic_init(mainpll, perpll);

	/* dedicated pins */
	for (i = arria10_pinmux_dedicated_io_4;
	     i <= arria10_pinmux_dedicated_io_17; i++)
		writel(pinmux[i], ARRIA10_PINMUX_DEDICATED_IO_4_ADDR +
		       (i - arria10_pinmux_dedicated_io_4) * sizeof(uint32_t));

	for (i = arria10_pincfg_dedicated_io_bank;
	     i <= arria10_pincfg_dedicated_io_17; i++)
		writel(pinmux[i], ARRIA10_PINCFG_DEDICATED_IO_BANK_ADDR +
		       (i - arria10_pincfg_dedicated_io_bank) * sizeof(uint32_t));

	/* deassert peripheral resets */
	arria10_reset_deassert_dedicated_peripherals();

	/* wait for fpga_usermode */
	while ((readl(0xffd03080) & 0x6) == 0);

	/* shared pins */
	for (i = arria10_pinmux_shared_io_q1_1;
	     i <= arria10_pinmux_shared_io_q4_12; i++)
		writel(pinmux[i], ARRIA10_PINMUX_SHARED_3V_IO_GRP_ADDR +
		       (i - arria10_pinmux_shared_io_q1_1) * sizeof(uint32_t));

	arria10_reset_deassert_shared_peripherals();

	/* usefpga: select source for signals: hps or fpga */
	for (i = arria10_pinmux_rgmii0_usefpga;
	     i < arria10_pinmux_max; i++)
		writel(pinmux[i], ARRIA10_PINMUX_FPGA_INTERFACE_ADDR +
		       (i - arria10_pinmux_rgmii0_usefpga) * sizeof(uint32_t));

	arria10_reset_deassert_fpga_peripherals();

	INIT_LL();
}
