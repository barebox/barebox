/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <init.h>
#include <common.h>
#include <io.h>
#include <linux/sizes.h>
#include <mfd/imx6q-iomuxc-gpr.h>
#include <mach/imx6.h>
#include <mach/generic.h>
#include <mach/revision.h>
#include <mach/imx6-anadig.h>
#include <mach/imx6-regs.h>
#include <mach/generic.h>
#include <asm/mmu.h>
#include <asm/cache-l2x0.h>

#define SI_REV 0x260

void imx6_init_lowlevel(void)
{
	void __iomem *aips1 = (void *)MX6_AIPS1_ON_BASE_ADDR;
	void __iomem *aips2 = (void *)MX6_AIPS2_ON_BASE_ADDR;
	void __iomem *iomux = (void *)MX6_IOMUXC_BASE_ADDR;
	bool is_imx6q = __imx6_cpu_type() == IMX6_CPUTYPE_IMX6Q;
	bool is_imx6d = __imx6_cpu_type() == IMX6_CPUTYPE_IMX6D;
	uint32_t val;

	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, aips1);
	writel(0x77777777, aips1 + 0x4);
	writel(0, aips1 + 0x40);
	writel(0, aips1 + 0x44);
	writel(0, aips1 + 0x48);
	writel(0, aips1 + 0x4c);
	writel(0, aips1 + 0x50);

	writel(0x77777777, aips2);
	writel(0x77777777, aips2 + 0x4);
	writel(0, aips2 + 0x40);
	writel(0, aips2 + 0x44);
	writel(0, aips2 + 0x48);
	writel(0, aips2 + 0x4c);
	writel(0, aips2 + 0x50);

	/* enable all clocks */
	writel(0xffffffff, 0x020c4068);
	writel(0xffffffff, 0x020c406c);
	writel(0xffffffff, 0x020c4070);
	writel(0xffffffff, 0x020c4074);
	writel(0xffffffff, 0x020c4078);
	writel(0xffffffff, 0x020c407c);
	writel(0xffffffff, 0x020c4080);

	/*
	 * Due to a hardware bug (related to errata ERR006282) on i.MX6DQ we
	 * need to gate/ungate all PFDs to make sure PFD is working right,
	 * otherwise PFDs may not output clock after reset.
	 */
	if (is_imx6q || is_imx6d) {
		writel(BM_ANADIG_PFD_480_PFD3_CLKGATE |
		       BM_ANADIG_PFD_480_PFD2_CLKGATE |
		       BM_ANADIG_PFD_480_PFD1_CLKGATE |
		       BM_ANADIG_PFD_480_PFD0_CLKGATE,
		       MX6_ANATOP_BASE_ADDR + HW_ANADIG_PFD_480_SET);
		writel(BM_ANADIG_PFD_528_PFD3_CLKGATE |
		       BM_ANADIG_PFD_528_PFD2_CLKGATE |
		       BM_ANADIG_PFD_528_PFD1_CLKGATE |
		       BM_ANADIG_PFD_528_PFD0_CLKGATE,
		       MX6_ANATOP_BASE_ADDR + HW_ANADIG_PFD_528_SET);

		writel(BM_ANADIG_PFD_480_PFD3_CLKGATE |
		       BM_ANADIG_PFD_480_PFD2_CLKGATE |
		       BM_ANADIG_PFD_480_PFD1_CLKGATE |
		       BM_ANADIG_PFD_480_PFD0_CLKGATE,
		       MX6_ANATOP_BASE_ADDR + HW_ANADIG_PFD_480_CLR);
		writel(BM_ANADIG_PFD_528_PFD3_CLKGATE |
		       BM_ANADIG_PFD_528_PFD2_CLKGATE |
		       BM_ANADIG_PFD_528_PFD1_CLKGATE |
		       BM_ANADIG_PFD_528_PFD0_CLKGATE,
		       MX6_ANATOP_BASE_ADDR + HW_ANADIG_PFD_528_CLR);
	}

	val = readl(iomux + IOMUXC_GPR4);
	val |= IMX6Q_GPR4_VPU_WR_CACHE_SEL | IMX6Q_GPR4_VPU_RD_CACHE_SEL |
		IMX6Q_GPR4_VPU_P_WR_CACHE_VAL | IMX6Q_GPR4_VPU_P_RD_CACHE_VAL_MASK |
		IMX6Q_GPR4_IPU_WR_CACHE_CTL | IMX6Q_GPR4_IPU_RD_CACHE_CTL;
	writel(val, iomux + IOMUXC_GPR4);

	/* Increase IPU read QoS priority */
	val = readl(iomux + IOMUXC_GPR6);
	val &= ~(IMX6Q_GPR6_IPU1_ID00_RD_QOS_MASK | IMX6Q_GPR6_IPU1_ID01_RD_QOS_MASK);
	val |= (0xf << 16) | (0x7 << 20);
	writel(val, iomux + IOMUXC_GPR6);

	val = readl(iomux + IOMUXC_GPR7);
	val &= ~(IMX6Q_GPR7_IPU2_ID00_RD_QOS_MASK | IMX6Q_GPR7_IPU2_ID01_RD_QOS_MASK);
	val |= (0xf << 16) | (0x7 << 20);
	writel(val, iomux + IOMUXC_GPR7);
}

int imx6_init(void)
{
	const char *cputypestr;
	u32 rev;
	u32 mx6_silicon_revision;

	imx6_init_lowlevel();

	imx6_boot_save_loc((void *)MX6_SRC_BASE_ADDR);

	rev = readl(MX6_ANATOP_BASE_ADDR + SI_REV);

	switch (rev & 0xfff) {
	case 0x00:
		mx6_silicon_revision = IMX_CHIP_REV_1_0;
		break;

	case 0x01:
		mx6_silicon_revision = IMX_CHIP_REV_1_1;
		break;

	case 0x02:
		mx6_silicon_revision = IMX_CHIP_REV_1_2;
		break;

	case 0x03:
		mx6_silicon_revision = IMX_CHIP_REV_1_3;
		break;

	case 0x04:
		mx6_silicon_revision = IMX_CHIP_REV_1_4;
		break;

	case 0x05:
		mx6_silicon_revision = IMX_CHIP_REV_1_5;
		break;

	case 0x100:
		mx6_silicon_revision = IMX_CHIP_REV_2_0;
		break;

	default:
		mx6_silicon_revision = IMX_CHIP_REV_UNKNOWN;
	}

	switch (imx6_cpu_type()) {
	case IMX6_CPUTYPE_IMX6Q:
		if (mx6_silicon_revision >= IMX_CHIP_REV_2_0)
			cputypestr = "i.MX6 Quad Plus";
		else
			cputypestr = "i.MX6 Quad";
		break;
	case IMX6_CPUTYPE_IMX6D:
		if (mx6_silicon_revision >= IMX_CHIP_REV_2_0)
			cputypestr = "i.MX6 Dual Plus";
		else
			cputypestr = "i.MX6 Dual";
		break;
	case IMX6_CPUTYPE_IMX6DL:
		cputypestr = "i.MX6 DualLite";
		break;
	case IMX6_CPUTYPE_IMX6S:
		cputypestr = "i.MX6 Solo";
		break;
	case IMX6_CPUTYPE_IMX6SX:
		cputypestr = "i.MX6 SoloX";
		break;
	default:
		cputypestr = "unknown i.MX6";
		break;
	}

	imx_set_silicon_revision(cputypestr, mx6_silicon_revision);

	return 0;
}

int imx6_devices_init(void)
{
	add_generic_device("imx-iomuxv3", 0, NULL, MX6_IOMUXC_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx6-ccm", 0, NULL, MX6_CCM_BASE_ADDR, 0x4000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpt", 0, NULL, MX6_GPT_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 0, NULL, MX6_GPIO1_BASE_ADDR, 0x4000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 1, NULL, MX6_GPIO2_BASE_ADDR, 0x4000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 2, NULL, MX6_GPIO3_BASE_ADDR, 0x4000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 3, NULL, MX6_GPIO4_BASE_ADDR, 0x4000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 4, NULL, MX6_GPIO5_BASE_ADDR, 0x4000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 5, NULL, MX6_GPIO6_BASE_ADDR, 0x4000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 6, NULL, MX6_GPIO7_BASE_ADDR, 0x4000, IORESOURCE_MEM, NULL);
	add_generic_device("imx21-wdt", 0, NULL, MX6_WDOG1_BASE_ADDR, 0x4000, IORESOURCE_MEM, NULL);
	add_generic_device("imx6-usb-misc", 0, NULL, MX6_USBOH3_USB_BASE_ADDR + 0x800, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}

static int imx6_mmu_init(void)
{
	void __iomem *l2x0_base = IOMEM(0x00a02000);
	u32 val, cache_part, cache_rtl;

	if (!cpu_is_mx6())
		return 0;

	val = readl(l2x0_base + L2X0_CACHE_ID);
	cache_part = val & L2X0_CACHE_ID_PART_MASK;
	cache_rtl  = val & L2X0_CACHE_ID_RTL_MASK;

	/* configure the PREFETCH register */
	val = readl(l2x0_base + L2X0_PREFETCH_CTRL);
	val |=  L2X0_DOUBLE_LINEFILL_EN |
		L2X0_INSTRUCTION_PREFETCH_EN |
		L2X0_DATA_PREFETCH_EN;
	/*
	 * set prefetch offset to 15
	 */
	val |= 15;
	/*
	 * The L2 cache controller(PL310) version on the i.MX6D/Q is r3p1-50rel0
	 * The L2 cache controller(PL310) version on the i.MX6DL/SOLO/SL is r3p2
	 * But according to ARM PL310 errata: 752271
	 * ID: 752271: Double linefill feature can cause data corruption
	 * Fault Status: Present in: r3p0, r3p1, r3p1-50rel0. Fixed in r3p2
	 * Workaround: The only workaround to this erratum is to disable the
	 * double linefill feature. This is the default behavior.
	 */
	if (cache_part == L2X0_CACHE_ID_PART_L310 &&
	    cache_rtl < L2X0_CACHE_ID_RTL_R3P2)
		val &= ~L2X0_DOUBLE_LINEFILL_EN;

	writel(val, l2x0_base + L2X0_PREFETCH_CTRL);

	/*
	 * Set shared attribute override bit in AUX_CTRL register, this is done
	 * here as it must be done regardless of the usage of the L2 cache in
	 * barebox itself. The kernel will not touch this bit, but it must be
	 * set to make the system compliant to the ARMv7 ARM RevC clarifications
	 * regarding conflicting memory aliases.
	 */
	val = readl(l2x0_base + L2X0_AUX_CTRL);
	val |= (1 << 22);
	writel(val, l2x0_base + L2X0_AUX_CTRL);

	l2x0_init(l2x0_base, 0x0, ~0UL);

	return 0;
}
postmmu_initcall(imx6_mmu_init);

#define SCU_CONFIG	0x04

static int imx6_fixup_cpus(struct device_node *root, void *context)
{
	struct device_node *cpus_node, *np, *tmp;
	unsigned long scu_phys_base;
	unsigned int max_core_index;

	cpus_node = of_find_node_by_name(root, "cpus");
	if (!cpus_node)
		return 0;

	/* get actual number of available CPU cores from SCU */
	asm("mrc p15, 4, %0, c15, c0, 0" : "=r" (scu_phys_base));
	max_core_index = (readl(IOMEM(scu_phys_base) + SCU_CONFIG) & 0x03);

	for_each_child_of_node_safe(cpus_node, tmp, np) {
		u32 cpu_index;

		if (of_property_read_u32(np, "reg", &cpu_index))
			continue;

		if (cpu_index > max_core_index)
			of_delete_node(np);
	}

	return 0;
}

static int imx6_fixup_cpus_register(void)
{
	if (!of_machine_is_compatible("fsl,imx6q") &&
	    !of_machine_is_compatible("fsl,imx6dl"))
		return 0;

	return of_register_fixup(imx6_fixup_cpus, NULL);
}
device_initcall(imx6_fixup_cpus_register);
