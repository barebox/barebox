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
#include <mach/revision.h>

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
