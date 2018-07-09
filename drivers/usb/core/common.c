#include <common.h>
#include <usb/ch9.h>

static const char *const speed_names[] = {
	[USB_SPEED_UNKNOWN] = "UNKNOWN",
	[USB_SPEED_LOW] = "low-speed",
	[USB_SPEED_FULL] = "full-speed",
	[USB_SPEED_HIGH] = "high-speed",
	[USB_SPEED_WIRELESS] = "wireless",
	[USB_SPEED_SUPER] = "super-speed",
};

const char *usb_speed_string(enum usb_device_speed speed)
{
	if (speed < 0 || speed >= ARRAY_SIZE(speed_names))
		speed = USB_SPEED_UNKNOWN;
	return speed_names[speed];
}
EXPORT_SYMBOL_GPL(usb_speed_string);

enum usb_device_speed usb_speed_by_string(const char *string)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(speed_names); i++)
		if (!strcmp(string, speed_names[i]))
			return i;

	return USB_SPEED_UNKNOWN;
}
EXPORT_SYMBOL_GPL(usb_speed_by_string);
