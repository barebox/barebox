// SPDX-License-Identifier: GPL-2.0-or-later

#include <abort.h>
#include <init.h>
#include <common.h>
#include <io.h>
#include <linux/sizes.h>
#include <mfd/imx6q-iomuxc-gpr.h>
#include <mach/imx/clock-imx6.h>
#include <mach/imx/imx6.h>
#include <mach/imx/generic.h>
#include <mach/imx/revision.h>
#include <mach/imx/reset-reason.h>
#include <mach/imx/imx6-anadig.h>
#include <mach/imx/imx6-regs.h>
#include <mach/imx/imx6-fusemap.h>
#include <mach/imx/usb.h>
#include <asm/mmu.h>
#include <asm/cache-l2x0.h>
#include <mfd/pfuze.h>

#define CLPCR				0x54
#define BP_CLPCR_LPM(mode)		((mode) & 0x3)
#define BM_CLPCR_LPM			(0x3 << 0)
#define BM_CLPCR_SBYOS			(0x1 << 6)
#define BM_CLPCR_VSTBY			(0x1 << 8)
#define BP_CLPCR_STBY_COUNT		9
#define BM_CLPCR_COSC_PWRDOWN		(0x1 << 11)
#define BM_CLPCR_BYP_MMDC_CH1_LPM_HS	(0x1 << 21)

#define MX6_OCOTP_CFG0			0x410
#define MX6_OCOTP_CFG1			0x420

#define BM_MPR_MPROT1_MTW		(0x1 << 25)

static void imx6_configure_aips(void __iomem *aips)
{
	u32 mpr = readl(aips);

	/* Bail if CPU ist not trusted for write accesses. */
	if (!(mpr & BM_MPR_MPROT1_MTW))
		return;

	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, aips);
	writel(0x77777777, aips + 0x4);

	/*
	 * Set all OPACRx to be non-bufferable, not require
	 * supervisor privilege level for access,allow for
	 * write access and untrusted master access.
	 */
	writel(0, aips + 0x40);
	writel(0, aips + 0x44);
	writel(0, aips + 0x48);
	writel(0, aips + 0x4c);
	writel(0, aips + 0x50);
}

static void imx6_init_lowlevel(void)
{
	bool is_imx6ull = __imx6_cpu_type() == IMX6_CPUTYPE_IMX6ULL;
	bool is_imx6sx = __imx6_cpu_type() == IMX6_CPUTYPE_IMX6SX;

	/*
	 * Before reset the controller imx6_boot_save_loc() must be called to
	 * detect serial-downloader fall back boots. For further information
	 * check the comment in imx6_get_boot_source().
	 */
	if ((readl(MXC_CCM_CCGR6) & 0x3))
		imx_reset_otg_controller(IOMEM(MX6_OTG_BASE_ADDR));

	imx6_configure_aips(IOMEM(MX6_AIPS1_ON_BASE_ADDR));
	imx6_configure_aips(IOMEM(MX6_AIPS2_ON_BASE_ADDR));
	if (is_imx6ull || is_imx6sx)
		imx6_configure_aips(IOMEM(MX6_AIPS3_ON_BASE_ADDR));
}

static bool imx6_has_ipu(void)
{
	if (cpu_mx6_is_mx6qp() || cpu_mx6_is_mx6dp() ||
	    cpu_mx6_is_mx6q() || cpu_mx6_is_mx6d() ||
	    cpu_mx6_is_mx6dl() || cpu_mx6_is_mx6s())
		return true;

	return false;
}

static void imx6_setup_ipu_qos(void)
{
	void __iomem *iomux = (void *)MX6_IOMUXC_BASE_ADDR;
	void __iomem *fast2 = (void *)MX6_FAST2_BASE_ADDR;
	uint32_t val;

	if (!imx6_has_ipu())
		return;

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

	/*
	 * On i.MX6 QP/DP the NoC regulator for the IPU ports needs to be in
	 * bypass mode for the above settings to take effect.
	 */
	if (cpu_mx6_is_mx6qp() || cpu_mx6_is_mx6dp()) {
		writel(0x2, fast2 + 0xb048c);
		writel(0x2, fast2 + 0xb050c);
	}
}

static void imx6ul_enet_clk_init(void)
{
	void __iomem *gprbase = IOMEM(MX6_IOMUXC_BASE_ADDR) + 0x4000;
	uint32_t val;

	if (!cpu_mx6_is_mx6ul() && !cpu_mx6_is_mx6ull())
		return;

	val = readl(gprbase + IOMUXC_GPR1);
	val |= (0x3 << 17);
	writel(val, gprbase + IOMUXC_GPR1);
}

int imx6_cpu_type(void)
{
	static int cpu_type = -1;

	if (!cpu_is_mx6())
		return 0;

	if (cpu_type < 0)
		cpu_type = __imx6_cpu_type();

	return cpu_type;
}

int imx6_cpu_revision(void)
{
	static int soc_revision = -1;

	if (!cpu_is_mx6())
		return 0;

	if (soc_revision < 0)
		soc_revision = __imx6_cpu_revision();

	return soc_revision;
}

u64 imx6_uid(void)
{
	return imx_ocotp_read_uid(IOMEM(MX6_OCOTP_BASE_ADDR));
}

static void imx6_register_poweroff_init(struct regmap *map)
{
	poweroff_handler_register_fn(imx6_pm_stby_poweroff);
}

int imx6_init(void)
{
	const char *cputypestr;
	u32 mx6_silicon_revision;
	void __iomem *src = IOMEM(MX6_SRC_BASE_ADDR);
	u64 mx6_uid;

	imx6_boot_save_loc();

	imx6_init_lowlevel();

	mx6_silicon_revision = imx6_cpu_revision();
	mx6_uid = imx6_uid();

	switch (imx6_cpu_type()) {
	case IMX6_CPUTYPE_IMX6Q:
		cputypestr = "i.MX6 Quad";
		break;
	case IMX6_CPUTYPE_IMX6QP:
		cputypestr = "i.MX6 Quad Plus";
		break;
	case IMX6_CPUTYPE_IMX6D:
		cputypestr = "i.MX6 Dual";
		break;
	case IMX6_CPUTYPE_IMX6DP:
		cputypestr = "i.MX6 Dual Plus";
		break;
	case IMX6_CPUTYPE_IMX6DL:
		cputypestr = "i.MX6 DualLite";
		break;
	case IMX6_CPUTYPE_IMX6S:
		cputypestr = "i.MX6 Solo";
		break;
	case IMX6_CPUTYPE_IMX6SL:
		cputypestr = "i.MX6 SoloLite";
		break;
	case IMX6_CPUTYPE_IMX6SX:
		cputypestr = "i.MX6 SoloX";
		break;
	case IMX6_CPUTYPE_IMX6UL:
		cputypestr = "i.MX6 UltraLite";
		break;
	case IMX6_CPUTYPE_IMX6ULL:
		cputypestr = "i.MX6 ULL";
		break;
	default:
		cputypestr = "unknown i.MX6";
		break;
	}

	imx_set_silicon_revision(cputypestr, mx6_silicon_revision);
	imx_set_reset_reason(src + IMX_SRC_SRSR, imx_reset_reasons);
	pr_info("%s unique ID: %llx\n", cputypestr, mx6_uid);

	imx6_setup_ipu_qos();
	imx6ul_enet_clk_init();

	pfuze_register_init_callback(imx6_register_poweroff_init);

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

bool imx6_cannot_write_l2x0(void)
{
	void __iomem *l2x0_base = IOMEM(0x00a02000);
	u32 val;
	/*
	 * Mask data aborts and try to access the PL210. If OP-TEE is running we
	 * will receive a data-abort and assume barebox is running in the normal
	 * world.
	 */
	val = readl(l2x0_base + L2X0_PREFETCH_CTRL);

	data_abort_mask();
	writel(val, l2x0_base + L2X0_PREFETCH_CTRL);
	return data_abort_unmask();
}

static int imx6_mmu_init(void)
{
	void __iomem *l2x0_base = IOMEM(0x00a02000);
	u32 val, cache_part, cache_rtl;

	if (!cpu_is_mx6())
		return 0;

	if (imx6_cannot_write_l2x0())
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

	cpus_node = of_find_node_by_name_address(root, "cpus");
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
	if (!of_machine_is_compatible("fsl,imx6qp") &&
	    !of_machine_is_compatible("fsl,imx6q")  &&
	    !of_machine_is_compatible("fsl,imx6dl"))
		return 0;

	return of_register_fixup(imx6_fixup_cpus, NULL);
}
device_initcall(imx6_fixup_cpus_register);

void __noreturn imx6_pm_stby_poweroff(struct poweroff_handler *handler,
				      unsigned long flags)
{
	void *ccm_base = IOMEM(MX6_CCM_BASE_ADDR);
	void *gpc_base = IOMEM(MX6_GPC_BASE_ADDR);
	u32 val;

	/*
	 * All this is done to get the PMIC_STBY_REQ line high which will
	 * cause the PMIC to turn off the i.MX6.
	 */

	/*
	 * First mask all interrupts in the GPC. This is necessary for
	 * unknown reasons
	 */
	writel(0xffffffff, gpc_base + 0x8);
	writel(0xffffffff, gpc_base + 0xc);
	writel(0xffffffff, gpc_base + 0x10);
	writel(0xffffffff, gpc_base + 0x14);

	val = readl(ccm_base + CLPCR);

	val &= ~BM_CLPCR_LPM;
	val |= BP_CLPCR_LPM(2);
	val |= 0x3 << BP_CLPCR_STBY_COUNT;
	val |= BM_CLPCR_VSTBY;
	val |= BM_CLPCR_SBYOS;
	val |= BM_CLPCR_BYP_MMDC_CH1_LPM_HS;

	writel(val, ccm_base + CLPCR);

	asm("wfi");

	while(1);
}
