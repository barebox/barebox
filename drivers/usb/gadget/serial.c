#include <common.h>
#include <errno.h>
#include <init.h>
#include <usb/ch9.h>
#include <usb/gadget.h>
#include <usb/composite.h>
#include <usb/usbserial.h>
#include <asm/byteorder.h>

#include "u_serial.h"

/* Defines */

#define GS_VERSION_STR			"v2.4"
#define GS_VERSION_NUM			0x2400

#define GS_LONG_NAME			"Gadget Serial"
#define GS_VERSION_NAME			GS_LONG_NAME " " GS_VERSION_STR

/* Thanks to NetChip Technologies for donating this product ID.
*
* DO NOT REUSE THESE IDs with a protocol-incompatible driver!!  Ever!!
* Instead:  allocate your own, using normal USB-IF procedures.
*/
#define GS_VENDOR_ID			0x0525	/* NetChip */
#define GS_PRODUCT_ID			0xa4a6	/* Linux-USB Serial Gadget */
#define GS_CDC_PRODUCT_ID		0xa4a7	/* ... as CDC-ACM */
#define GS_CDC_OBEX_PRODUCT_ID		0xa4a9	/* ... as CDC-OBEX */

/* string IDs are assigned dynamically */

#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_DESCRIPTION_IDX		2

static char manufacturer[50];

static struct usb_string strings_dev[] = {
	[STRING_MANUFACTURER_IDX].s = manufacturer,
	[STRING_PRODUCT_IDX].s = GS_VERSION_NAME,
	[STRING_DESCRIPTION_IDX].s = NULL /* updated; f(use_acm) */,
	{  } /* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static int use_acm = 1;
#ifdef HAVE_OBEX
static int use_obex = 0;
#endif
static unsigned n_ports = 1;

static int serial_bind_config(struct usb_configuration *c)
{
	unsigned i;
	int status = 0;

	for (i = 0; i < n_ports && status == 0; i++) {
		if (use_acm)
			status = acm_bind_config(c, i);
#ifdef HAVE_OBEX
		else if (use_obex)
			status = obex_bind_config(c, i);
#endif
		else
			status = gser_bind_config(c, i);
	}
	return status;
}

static struct usb_device_descriptor device_desc = {
	.bLength =		USB_DT_DEVICE_SIZE,
	.bDescriptorType =	USB_DT_DEVICE,
	.bcdUSB =		cpu_to_le16(0x0200),
	/* .bDeviceClass = f(use_acm) */
	.bDeviceSubClass =	0,
	.bDeviceProtocol =	0,
	/* .bMaxPacketSize0 = f(hardware) */
	.idVendor =		cpu_to_le16(GS_VENDOR_ID),
	/* .idProduct =	f(use_acm) */
	/* .bcdDevice = f(hardware) */
	/* .iManufacturer = DYNAMIC */
	/* .iProduct = DYNAMIC */
	.bNumConfigurations =	1,
};

static struct usb_configuration serial_config_driver = {
	/* .label = f(use_acm) */
	.bind		= serial_bind_config,
	/* .bConfigurationValue = f(use_acm) */
	/* .iConfiguration = DYNAMIC */
	.bmAttributes	= USB_CONFIG_ATT_SELFPOWER,
};

static int gs_bind(struct usb_composite_dev *cdev)
{
	int			gcnum;
	struct usb_gadget	*gadget = cdev->gadget;
	int			status;

	status = gserial_setup(cdev->gadget, n_ports);
	if (status < 0)
		return status;

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */

	/* device description: manufacturer, product */
	sprintf(manufacturer, "barebox with %s",
		gadget->name);
	status = usb_string_id(cdev);
	if (status < 0)
		goto fail;
	strings_dev[STRING_MANUFACTURER_IDX].id = status;

	device_desc.iManufacturer = status;

	status = usb_string_id(cdev);
	if (status < 0)
		goto fail;
	strings_dev[STRING_PRODUCT_IDX].id = status;

	device_desc.iProduct = status;

	/* config description */
	status = usb_string_id(cdev);
	if (status < 0)
		goto fail;
	strings_dev[STRING_DESCRIPTION_IDX].id = status;

	serial_config_driver.iConfiguration = status;

	/* set up other descriptors */
//	gcnum = usb_gadget_controller_number(gadget);
	gcnum = 0x19;
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16(GS_VERSION_NUM | gcnum);
	else {
		/* this is so simple (for now, no altsettings) that it
		 * SHOULD NOT have problems with bulk-capable hardware.
		 * so warn about unrcognized controllers -- don't panic.
		 *
		 * things like configuration and altsetting numbering
		 * can need hardware-specific attention though.
		 */
		pr_warning("gs_bind: controller '%s' not recognized\n",
			gadget->name);
		device_desc.bcdDevice =
			cpu_to_le16(GS_VERSION_NUM | 0x0099);
	}

	/* register our configuration */
	status = usb_add_config(cdev, &serial_config_driver);
	if (status < 0)
		goto fail;

	INFO(cdev, "%s\n", GS_VERSION_NAME);

	return 0;

fail:
//	gserial_cleanup();
	return status;
}

static struct usb_composite_driver gserial_driver = {
	.name		= "g_serial",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.bind		= gs_bind,
};

int usb_serial_register(struct usb_serial_pdata *pdata)
{
	/* We *could* export two configs; that'd be much cleaner...
	 * but neither of these product IDs was defined that way.
	 */

	/*
	 * PXA CPU suffer a silicon bug which prevents them from being a
	 * compound device, forbiding the ACM configurations.
	 */
#ifdef CONFIG_ARCH_PXA2XX
	use_acm = 0;
#endif
	switch (pdata->mode) {
	case 1:
#ifdef HAVE_OBEX
		use_obex = 1;
#endif
		use_acm = 0;
		break;
	case 2:
#ifdef HAVE_OBEX
		use_obex = 1;
#endif
		use_acm = 0;
		break;
	default:
#ifdef HAVE_OBEX
		use_obex = 0;
#endif
		use_acm = 1;
	}

	if (use_acm) {
		serial_config_driver.label = "CDC ACM config";
		serial_config_driver.bConfigurationValue = 2;
		device_desc.bDeviceClass = USB_CLASS_COMM;
		device_desc.idProduct =
				cpu_to_le16(GS_CDC_PRODUCT_ID);
	}
#ifdef HAVE_OBEX
	else if (use_obex) {
		serial_config_driver.label = "CDC OBEX config";
		serial_config_driver.bConfigurationValue = 3;
		device_desc.bDeviceClass = USB_CLASS_COMM;
		device_desc.idProduct =
			cpu_to_le16(GS_CDC_OBEX_PRODUCT_ID);
	}
#endif
	else {
		serial_config_driver.label = "Generic Serial config";
		serial_config_driver.bConfigurationValue = 1;
		device_desc.bDeviceClass = USB_CLASS_VENDOR_SPEC;
		device_desc.idProduct =
				cpu_to_le16(GS_PRODUCT_ID);
	}
	strings_dev[STRING_DESCRIPTION_IDX].s = serial_config_driver.label;
	if (pdata->idVendor)
		device_desc.idVendor = pdata->idVendor;
	if (pdata->idProduct)
		device_desc.idProduct = pdata->idProduct;
	strings_dev[STRING_MANUFACTURER_IDX].s = pdata->manufacturer;
	strings_dev[STRING_PRODUCT_IDX].s = pdata->productname;

	return usb_composite_register(&gserial_driver);
}

void usb_serial_unregister(void)
{
	usb_composite_unregister(&gserial_driver);
}
