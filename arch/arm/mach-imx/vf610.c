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
#include <mach/generic.h>
#include <mach/revision.h>
#include <mach/vf610.h>
#include <mach/reset-reason.h>

static const struct imx_reset_reason vf610_reset_reasons[] = {
	{ VF610_SRC_SRSR_POR_RST,       RESET_POR,   0 },
	{ VF610_SRC_SRSR_WDOG_A5,       RESET_WDG,   0 },
	{ VF610_SRC_SRSR_WDOG_M4,       RESET_WDG,   1 },
	{ VF610_SRC_SRSR_JTAG_RST,      RESET_JTAG,  0 },
	{ VF610_SRC_SRSR_RESETB,        RESET_EXT,   0 },
	{ VF610_SRC_SRSR_SW_RST,        RESET_RST,   0 },
	{ /* sentinel */ }
};

int vf610_init(void)
{
	const char *cputypestr;
	void __iomem *src = IOMEM(VF610_SRC_BASE_ADDR);

	vf610_boot_save_loc();

	switch (vf610_cpu_type()) {
	case VF610_CPUTYPE_VF610:
		cputypestr = "VF610";
		break;
	case VF610_CPUTYPE_VF600:
		cputypestr = "VF600";
		break;
	case VF610_CPUTYPE_VF510:
		cputypestr = "VF510";
		break;
	case VF610_CPUTYPE_VF500:
		cputypestr = "VF500";
		break;
	default:
		cputypestr = "unknown VFxxx";
		break;
	}

	imx_set_silicon_revision(cputypestr, vf610_cpu_revision());
	imx_set_reset_reason(src + IMX_SRC_SRSR, vf610_reset_reasons);
	return 0;
}
