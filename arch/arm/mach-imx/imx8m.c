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
#include <mach/ocotp.h>

#include <linux/iopoll.h>
#include <linux/arm-smccc.h>

#define FSL_SIP_BUILDINFO			0xC2000003
#define FSL_SIP_BUILDINFO_GET_COMMITHASH	0x00

u64 imx8mq_uid(void)
{
	return imx_ocotp_read_uid(IOMEM(MX8MQ_OCOTP_BASE_ADDR));
}

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
	pr_info("%s unique ID: %llx\n", cputypestr, imx8mq_uid());

	if (IS_ENABLED(CONFIG_ARM_SMCCC) &&
	    IS_ENABLED(CONFIG_FIRMWARE_IMX8MQ_ATF)) {
		arm_smccc_smc(FSL_SIP_BUILDINFO,
			      FSL_SIP_BUILDINFO_GET_COMMITHASH,
			      0, 0, 0, 0, 0, 0, &res);
		pr_info("i.MX ARM Trusted Firmware: %s\n", (char *)&res.a0);
	}

	return 0;
}

#define KEEP_ALIVE			0x18
#define VER_L				0x1c
#define VER_H				0x20
#define VER_LIB_L_ADDR			0x24
#define VER_LIB_H_ADDR			0x28
#define FW_ALIVE_TIMEOUT_US		100000

static int imx8mq_report_hdmi_firmware(void)
{
	void __iomem *hdmi = IOMEM(MX8MQ_HDMI_CTRL_BASE_ADDR);
	u16 ver_lib, ver;
	u32 reg;
	int ret;

	if (!cpu_is_mx8mq())
		return 0;

	/* check the keep alive register to make sure fw working */
	ret = readl_poll_timeout(hdmi + KEEP_ALIVE,
				 reg, reg, FW_ALIVE_TIMEOUT_US);
	if (ret < 0) {
		pr_info("HDP firmware is not running\n");
		return 0;
	}

	ver = readl(hdmi + VER_H) & 0xff;
	ver <<= 8;
	ver |= readl(hdmi + VER_L) & 0xff;

	ver_lib = readl(hdmi + VER_LIB_H_ADDR) & 0xff;
	ver_lib <<= 8;
	ver_lib |= readl(hdmi + VER_LIB_L_ADDR) & 0xff;

	pr_info("HDP firmware ver: %d ver_lib: %d\n", ver, ver_lib);

	return 0;
}
console_initcall(imx8mq_report_hdmi_firmware);
