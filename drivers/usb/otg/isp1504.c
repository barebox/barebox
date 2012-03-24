#include <common.h>
#include <usb/ulpi.h>
#include <usb/isp1504.h>

int isp1504_set_vbus_power(void __iomem *view, int on)
{
	int ret = 0;

	if (ulpi_init(view))
		return -1;

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
