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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <io.h>
#include <init.h>
#include <mci.h>
#include <asm/armlinux.h>
#include <mach/imx51-regs.h>
#include <mach/iomux-mx51.h>
#include <mach/devices-imx51.h>
#include <generated/mach-types.h>

#include "ccxmx51.h"

static iomux_v3_cfg_t ccxmx51js_pads[] = {
	/* SD1 */
	MX51_PAD_SD1_CLK__SD1_CLK,
	MX51_PAD_SD1_CMD__SD1_CMD,
	MX51_PAD_SD1_DATA0__SD1_DATA0,
	MX51_PAD_SD1_DATA1__SD1_DATA1,
	MX51_PAD_SD1_DATA2__SD1_DATA2,
	MX51_PAD_SD1_DATA3__SD1_DATA3,
	/* SD3 */
	MX51_PAD_NANDF_CS7__SD3_CLK,
	MX51_PAD_NANDF_RDY_INT__SD3_CMD,
	MX51_PAD_NANDF_D8__SD3_DATA0,
	MX51_PAD_NANDF_D9__SD3_DATA1,
	MX51_PAD_NANDF_D10__SD3_DATA2,
	MX51_PAD_NANDF_D11__SD3_DATA3,
	MX51_PAD_NANDF_D12__SD3_DAT4,
	MX51_PAD_NANDF_D13__SD3_DAT5,
	MX51_PAD_NANDF_D14__SD3_DAT6,
	MX51_PAD_NANDF_D15__SD3_DAT7,
	/* USB HOST1 */
	MX51_PAD_USBH1_CLK__USBH1_CLK,
	MX51_PAD_USBH1_DIR__USBH1_DIR,
	MX51_PAD_USBH1_NXT__USBH1_NXT,
	MX51_PAD_USBH1_STP__USBH1_STP,
	MX51_PAD_USBH1_DATA0__USBH1_DATA0,
	MX51_PAD_USBH1_DATA1__USBH1_DATA1,
	MX51_PAD_USBH1_DATA2__USBH1_DATA2,
	MX51_PAD_USBH1_DATA3__USBH1_DATA3,
	MX51_PAD_USBH1_DATA4__USBH1_DATA4,
	MX51_PAD_USBH1_DATA5__USBH1_DATA5,
	MX51_PAD_USBH1_DATA6__USBH1_DATA6,
	MX51_PAD_USBH1_DATA7__USBH1_DATA7,
	MX51_PAD_DISPB2_SER_RS__GPIO3_8,	/* Reset */
};

static struct esdhc_platform_data sdhc1_pdata = {
	.cd_type	= ESDHC_CD_NONE,
	.wp_type	= ESDHC_WP_NONE,
	.caps		= MMC_MODE_4BIT,
};

static struct esdhc_platform_data sdhc3_pdata = {
	.cd_type	= ESDHC_CD_NONE,
	.wp_type	= ESDHC_WP_NONE,
	.caps		= MMC_MODE_4BIT | MMC_MODE_8BIT,
};

static int ccxmx51js_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(ccxmx51js_pads, ARRAY_SIZE(ccxmx51js_pads));

	if (IS_ENABLED(CONFIG_MCI_IMX_ESDHC)) {
		imx51_add_mmc0(&sdhc1_pdata);
		imx51_add_mmc2(&sdhc3_pdata);
	}

	armlinux_set_architecture(ccxmx51_id->wless ? MACH_TYPE_CCWMX51JS : MACH_TYPE_CCMX51JS);

	return 0;
}

late_initcall(ccxmx51js_init);
