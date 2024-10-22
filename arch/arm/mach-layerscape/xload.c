// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <bootsource.h>
#include <mach/layerscape/layerscape.h>
#include <mach/layerscape/xload.h>

int ls1046a_xload_start_image(void)
{
	enum bootsource src;

	src = ls1046a_bootsource_get();

	switch (src) {
	case BOOTSOURCE_SPI_NOR:
		return ls1046a_qspi_start_image();
#if defined(CONFIG_MCI_IMX_ESDHC_PBL)
	case BOOTSOURCE_MMC:
		return ls1046a_esdhc_start_image();
#endif
	default:
		pr_err("Unknown bootsource\n");
		return -EINVAL;
	}
}

int ls1021a_xload_start_image(void)
{
	enum bootsource src;

	src = ls1021a_bootsource_get();

	switch (src) {
	case BOOTSOURCE_SPI_NOR:
		return ls1021a_qspi_start_image();
	default:
		pr_err("Unknown bootsource\n");
		return -EINVAL;
	}
}
