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

int vf610_init(void)
{
	const char *cputypestr;
	void __iomem *src = IOMEM(VF610_SRC_BASE_ADDR);

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
	return 0;
}
