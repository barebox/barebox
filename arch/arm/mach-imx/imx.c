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
#include <init.h>
#include <mach/revision.h>
#include <mach/generic.h>

static int __imx_silicon_revision = IMX_CHIP_REV_UNKNOWN;

int imx_silicon_revision(void)
{
	return __imx_silicon_revision;
}

void imx_set_silicon_revision(const char *soc, int revision)
{
	__imx_silicon_revision = revision;

	printf("detected %s revision %d.%d\n", soc,
			(revision >> 4) & 0xf,
			revision & 0xf);
}

static int imx_init(void)
{
	int ret;

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
	else if (cpu_is_mx51())
		ret = imx51_init();
	else if (cpu_is_mx53())
		ret = imx53_init();
	else if (cpu_is_mx6())
		ret = imx6_init();
	else
		return -EINVAL;

	if (of_get_root_node())
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
