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

#include <common.h>
#include <of.h>
#include <init.h>
#include <io.h>
#include <mach/revision.h>
#include <mach/generic.h>
#include <mach/reset-reason.h>

static int __imx_silicon_revision = IMX_CHIP_REV_UNKNOWN;

int imx_silicon_revision(void)
{
	return __imx_silicon_revision;
}

void imx_set_silicon_revision(const char *soc, int revision)
{
	__imx_silicon_revision = revision;

	if (revision == IMX_CHIP_REV_UNKNOWN)
		pr_info("detected %s revision unknown\n", soc);
	else
		pr_info("detected %s revision %d.%d\n", soc,
			(revision >> 4) & 0xf,
			revision & 0xf);
}

unsigned int __imx_cpu_type;

static int imx_soc_from_dt(void)
{
	if (of_machine_is_compatible("fsl,imx1"))
		return IMX_CPU_IMX1;
	if (of_machine_is_compatible("fsl,imx21"))
		return IMX_CPU_IMX21;
	if (of_machine_is_compatible("fsl,imx25"))
		return IMX_CPU_IMX25;
	if (of_machine_is_compatible("fsl,imx27"))
		return IMX_CPU_IMX27;
	if (of_machine_is_compatible("fsl,imx31"))
		return IMX_CPU_IMX31;
	if (of_machine_is_compatible("fsl,imx35"))
		return IMX_CPU_IMX35;
	if (of_machine_is_compatible("fsl,imx50"))
		return IMX_CPU_IMX50;
	if (of_machine_is_compatible("fsl,imx51"))
		return IMX_CPU_IMX51;
	if (of_machine_is_compatible("fsl,imx53"))
		return IMX_CPU_IMX53;
	if (of_machine_is_compatible("fsl,imx6q"))
		return IMX_CPU_IMX6;
	if (of_machine_is_compatible("fsl,imx6dl"))
		return IMX_CPU_IMX6;
	if (of_machine_is_compatible("fsl,imx6sx"))
		return IMX_CPU_IMX6;
	if (of_machine_is_compatible("fsl,imx6sl"))
		return IMX_CPU_IMX6;
	if (of_machine_is_compatible("fsl,imx6qp"))
		return IMX_CPU_IMX6;
	if (of_machine_is_compatible("fsl,imx6ul"))
		return IMX_CPU_IMX6;
	if (of_machine_is_compatible("fsl,imx6ull"))
		return IMX_CPU_IMX6;
	if (of_machine_is_compatible("fsl,imx7s"))
		return IMX_CPU_IMX7;
	if (of_machine_is_compatible("fsl,imx7d"))
		return IMX_CPU_IMX7;
	if (of_machine_is_compatible("fsl,imx8mq"))
		return IMX_CPU_IMX8MQ;
	if (of_machine_is_compatible("fsl,vf610"))
		return IMX_CPU_VF610;

	return 0;
}

static int imx_init(void)
{
	int ret;
	struct device_node *root;

	root = of_get_root_node();
	if (root) {
		__imx_cpu_type = imx_soc_from_dt();
		if (!__imx_cpu_type)
			hang();
	}

	if (cpu_is_mx1())
		ret = imx1_init();
	else if (cpu_is_mx21())
		ret = imx21_init();
	else if (cpu_is_mx25())
		ret = imx25_init();
	else if (cpu_is_mx27())
		ret = imx27_init();
	else if (cpu_is_mx31())
		ret = imx31_init();
	else if (cpu_is_mx35())
		ret = imx35_init();
	else if (cpu_is_mx50())
		ret = imx50_init();
	else if (cpu_is_mx51())
		ret = imx51_init();
	else if (cpu_is_mx53())
		ret = imx53_init();
	else if (cpu_is_mx6())
		ret = imx6_init();
	else if (cpu_is_mx7())
		ret = imx7_init();
	else if (cpu_is_mx8mq())
		ret = imx8mq_init();
	else if (cpu_is_vf610())
		ret = vf610_init();
	else
		return -EINVAL;

	if (root)
		return ret;

	if (cpu_is_mx1())
		ret = imx1_devices_init();
	else if (cpu_is_mx21())
		ret = imx21_devices_init();
	else if (cpu_is_mx25())
		ret = imx25_devices_init();
	else if (cpu_is_mx27())
		ret = imx27_devices_init();
	else if (cpu_is_mx31())
		ret = imx31_devices_init();
	else if (cpu_is_mx35())
		ret = imx35_devices_init();
	else if (cpu_is_mx50())
		ret = imx50_devices_init();
	else if (cpu_is_mx51())
		ret = imx51_devices_init();
	else if (cpu_is_mx53())
		ret = imx53_devices_init();
	else if (cpu_is_mx6())
		ret = imx6_devices_init();
	else
		return -EINVAL;

	return ret;
}
postcore_initcall(imx_init);

const struct imx_reset_reason imx_reset_reasons[] = {
	{ IMX_SRC_SRSR_IPP_RESET,     RESET_POR,  0 },
	{ IMX_SRC_SRSR_WDOG1_RESET,   RESET_WDG,  0 },
	{ IMX_SRC_SRSR_JTAG_RESET,    RESET_JTAG, 0 },
	{ IMX_SRC_SRSR_JTAG_SW_RESET, RESET_JTAG, 0 },
	{ IMX_SRC_SRSR_WARM_BOOT,     RESET_RST,  0 },
	{ /* sentinel */ }
};

void imx_set_reset_reason(void __iomem *srsr,
			  const struct imx_reset_reason *reasons)
{
	enum reset_src_type type = RESET_UKWN;
	const u32 reg = readl(srsr);
	int i, instance = 0;

	/*
	 * SRSR register captures ALL reset event that occured since
	 * POR, so we need to clear it to make sure we only caputre
	 * the latest one.
	 */
	writel(reg, srsr);

	for (i = 0; reasons[i].mask; i++) {
		if (reg & reasons[i].mask) {
			type     = reasons[i].type;
			instance = reasons[i].instance;
			break;
		}
	}

	/*
	 * Report this with above default priority in order to make
	 * sure we'll always override info from watchdog driver.
	 */
	reset_source_set_priority(type,
				  RESET_SOURCE_DEFAULT_PRIORITY + 1);
	reset_source_set_instance(type, instance);

	pr_info("i.MX reset reason %s (SRSR: 0x%08x)\n",
		reset_source_name(), reg);
}
