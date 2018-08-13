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
#include <asm/syscounter.h>
#include <asm/system.h>
#include <mach/generic.h>
#include <mach/revision.h>
#include <mach/imx8mq.h>
#include <mach/reset-reason.h>

#include <linux/arm-smccc.h>

#define FSL_SIP_BUILDINFO			0xC2000003
#define FSL_SIP_BUILDINFO_GET_COMMITHASH	0x00

static int imx8mq_init_syscnt_frequency(void)
{
	if (current_el() == 3) {
		void __iomem *syscnt = IOMEM(MX8MQ_SYSCNT_CTRL_BASE_ADDR);
		/*
		 * Update with accurate clock frequency
		 */
		set_cntfrq(syscnt_get_cntfrq(syscnt));
		syscnt_enable(syscnt);
	}

	return 0;
}
/*
 * This call needs to happen before timer driver gets probed and
 * requests its update frequency via cntfrq_el0
 */
core_initcall(imx8mq_init_syscnt_frequency);

int imx8mq_init(void)
{
	void __iomem *anatop = IOMEM(MX8MQ_ANATOP_BASE_ADDR);
	void __iomem *src = IOMEM(MX8MQ_SRC_BASE_ADDR);
	uint32_t type = FIELD_GET(DIGPROG_MAJOR,
				  readl(anatop + MX8MQ_ANATOP_DIGPROG));
	struct arm_smccc_res res;
	const char *cputypestr;

	imx8_boot_save_loc();

	switch (type) {
	case IMX8M_CPUTYPE_IMX8MQ:
		cputypestr = "i.MX8MQ";
		break;
	default:
		cputypestr = "unknown i.MX8M";
		break;
	};

	imx_set_silicon_revision(cputypestr, imx8mq_cpu_revision());
	/*
	 * Reset reasons seem to be identical to that of i.MX7
	 */
	imx_set_reset_reason(src + IMX7_SRC_SRSR, imx7_reset_reasons);

	if (IS_ENABLED(CONFIG_ARM_SMCCC) &&
	    IS_ENABLED(CONFIG_FIRMWARE_IMX8MQ_ATF)) {
		arm_smccc_smc(FSL_SIP_BUILDINFO,
			      FSL_SIP_BUILDINFO_GET_COMMITHASH,
			      0, 0, 0, 0, 0, 0, &res);
		pr_info("i.MX ARM Trusted Firmware: %s\n", (char *)&res.a0);
	}

	return 0;
}
