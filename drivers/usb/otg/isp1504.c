#include <common.h>
#include <usb/ulpi.h>
#include <usb/isp1504.h>

int isp1504_set_vbus_power(void __iomem *view, int on)
{
	int vid, pid, ret = 0;

	vid = (ulpi_read(ULPI_VID_HIGH, view) << 8) |
		ulpi_read(ULPI_VID_LOW, view);
	pid = (ulpi_read(ULPI_PID_HIGH, view) << 8) |
		ulpi_read(ULPI_PID_LOW, view);

	pr_info("ULPI Vendor ID 0x%x    Product ID 0x%x\n", vid, pid);
	if (vid != 0x4cc || pid != 0x1504) {
		pr_err("No ISP1504 found\n");
		return -1;
	}

	if (on) {
		ret = ulpi_set(DRV_VBUS_EXT |       /* enable external Vbus */
				DRV_VBUS |          /* enable internal Vbus */
				USE_EXT_VBUS_IND |  /* use external indicator */
				CHRG_VBUS,          /* charge Vbus */
				ULPI_OTGCTL, view);
	} else {
		ret = ulpi_clear(DRV_VBUS_EXT | /* disable external Vbus */
				DRV_VBUS,         /* disable internal Vbus */
				ULPI_OTGCTL, view);

		ret |= ulpi_set(USE_EXT_VBUS_IND | /* use external indicator */
				DISCHRG_VBUS,          /* discharge Vbus */
				ULPI_OTGCTL, view);
	}

	return ret;
}
