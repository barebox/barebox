// SPDX-License-Identifier: GPL-2.0-or-later

#include <init.h>
#include <common.h>
#include <io.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <mach/revision.h>
#include <mach/vf610.h>
#include <mach/reset-reason.h>
#include <mach/ocotp.h>

static const struct imx_reset_reason vf610_reset_reasons[] = {
	{ VF610_SRC_SRSR_POR_RST,       RESET_POR,   0 },
	{ VF610_SRC_SRSR_WDOG_A5,       RESET_WDG,   0 },
	{ VF610_SRC_SRSR_WDOG_M4,       RESET_WDG,   1 },
	{ VF610_SRC_SRSR_JTAG_RST,      RESET_JTAG,  0 },
	{ VF610_SRC_SRSR_RESETB,        RESET_EXT,   0 },
	{ VF610_SRC_SRSR_SW_RST,        RESET_RST,   0 },
	{ /* sentinel */ }
};

u64 vf610_uid(void)
{
	return imx_ocotp_read_uid(IOMEM(VF610_OCOTP_BASE_ADDR));
}

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
	pr_info("%s unique ID: %llx\n", cputypestr, vf610_uid());

	return 0;
}
