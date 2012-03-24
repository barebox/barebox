#include <common.h>
#include <usb/ulpi.h>

int isp1504_set_vbus_power(void __iomem *view, int on)
{
	if (ulpi_init(view))
		return -1;

	return ulpi_set_vbus(view, on);
}
