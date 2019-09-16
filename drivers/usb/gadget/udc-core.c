/**
 * udc.c - Core UDC Framework
 *
 * Copyright (C) 2010 Texas Instruments
 * Author: Felipe Balbi <balbi@ti.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define VERBOSE_DEBUG
#include <common.h>
#include <driver.h>
#include <init.h>
#include <poller.h>
#include <usb/ch9.h>
#include <usb/gadget.h>

/**
 * struct usb_udc - describes one usb device controller
 * @driver - the gadget driver pointer. For use by the class code
 * @dev - the child device to the actual controller
 * @gadget - the gadget. For use by the class code
 * @list - for use by the udc class driver
 *
 * This represents the internal data structure which is used by the UDC-class
 * to hold information about udc driver and gadget together.
 */
struct usb_udc {
	struct usb_gadget_driver	*driver;
	struct usb_gadget		*gadget;
	struct device_d			dev;
	struct list_head		list;
	struct poller_struct		poller;
};

static LIST_HEAD(udc_list);

/* ------------------------------------------------------------------------- */

#ifdef	CONFIG_KERNEL_HAS_DMA

int usb_gadget_map_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in)
{
	if (req->length == 0)
		return 0;

	if (req->num_sgs) {
		int     mapped;

		mapped = dma_map_sg(&gadget->dev, req->sg, req->num_sgs,
				is_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
		if (mapped == 0) {
			dev_err(&gadget->dev, "failed to map SGs\n");
			return -EFAULT;
		}

		req->num_mapped_sgs = mapped;
	} else {
		req->dma = dma_map_single(&gadget->dev, req->buf, req->length,
				is_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

		if (dma_mapping_error(&gadget->dev, req->dma)) {
			dev_err(&gadget->dev, "failed to map buffer\n");
			return -EFAULT;
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(usb_gadget_map_request);

void usb_gadget_unmap_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in)
{
	if (req->length == 0)
		return;

	if (req->num_mapped_sgs) {
		dma_unmap_sg(&gadget->dev, req->sg, req->num_mapped_sgs,
				is_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

		req->num_mapped_sgs = 0;
	} else {
		dma_unmap_single(&gadget->dev, req->dma, req->length,
				is_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
	}
}
EXPORT_SYMBOL_GPL(usb_gadget_unmap_request);

#endif	/* CONFIG_HAS_DMA */

/* ------------------------------------------------------------------------- */

void usb_gadget_set_state(struct usb_gadget *gadget,
		enum usb_device_state state)
{
	gadget->state = state;
}
EXPORT_SYMBOL_GPL(usb_gadget_set_state);

/**
 * usb_gadget_udc_reset - notifies the udc core that bus reset occurs
 * @gadget: The gadget which bus reset occurs
 * @driver: The gadget driver we want to notify
 *
 * If the udc driver has bus reset handler, it needs to call this when the bus
 * reset occurs, it notifies the gadget driver that the bus reset occurs as
 * well as updates gadget state.
 */
void usb_gadget_udc_reset(struct usb_gadget *gadget,
		struct usb_gadget_driver *driver)
{
	usb_gadget_set_state(gadget, USB_STATE_DEFAULT);
}
EXPORT_SYMBOL_GPL(usb_gadget_udc_reset);
/* ------------------------------------------------------------------------- */

/**
 * usb_gadget_udc_start - tells usb device controller to start up
 * @gadget: The gadget we want to get started
 * @driver: The driver we want to bind to @gadget
 *
 * This call is issued by the UDC Class driver when it's about
 * to register a gadget driver to the device controller, before
 * calling gadget driver's bind() method.
 *
 * It allows the controller to be powered off until strictly
 * necessary to have it powered on.
 *
 * Returns zero on success, else negative errno.
 */
static inline int usb_gadget_udc_start(struct usb_gadget *gadget,
		struct usb_gadget_driver *driver)
{
	return gadget->ops->udc_start(gadget, driver);
}

/**
 * usb_gadget_udc_stop - tells usb device controller we don't need it anymore
 * @gadget: The device we want to stop activity
 * @driver: The driver to unbind from @gadget
 *
 * This call is issued by the UDC Class driver after calling
 * gadget driver's unbind() method.
 *
 * The details are implementation specific, but it can go as
 * far as powering off UDC completely and disable its data
 * line pullups.
 */
static inline void usb_gadget_udc_stop(struct usb_gadget *gadget,
		struct usb_gadget_driver *driver)
{
	gadget->ops->udc_stop(gadget, driver);
}

int usb_gadget_poll(void)
{
	struct usb_udc		*udc;

	list_for_each_entry(udc, &udc_list, list) {
		if (udc->gadget->ops->udc_poll)
			udc->gadget->ops->udc_poll(udc->gadget);
	}

	return 0;
}

/**
 * usb_add_gadget_udc_release - adds a new gadget to the udc class driver list
 * @parent: the parent device to this udc. Usually the controller driver's
 * device.
 * @gadget: the gadget to be added to the list.
 * @release: a gadget release function.
 *
 * Returns zero on success, negative errno otherwise.
 */
int usb_add_gadget_udc_release(struct device_d *parent, struct usb_gadget *gadget,
		void (*release)(struct device_d *dev))
{
	struct usb_udc		*udc;
	int			ret = -ENOMEM;

	udc = kzalloc(sizeof(*udc), GFP_KERNEL);
	if (!udc)
		goto err1;

	dev_set_name(&gadget->dev, "usbgadget");
	gadget->dev.id = DEVICE_ID_SINGLE;
	gadget->dev.parent = parent;

	ret = register_device(&gadget->dev);
	if (ret)
		goto err2;

	dev_add_param_uint32(&gadget->dev, "product", NULL, NULL,
			&gadget->product_id, "0x%04x", NULL);
	dev_add_param_uint32(&gadget->dev, "vendor", NULL, NULL,
			&gadget->vendor_id, "0x%04x", NULL);
	gadget->manufacturer = xstrdup("barebox");
	dev_add_param_string(&gadget->dev, "manufacturer", NULL, NULL,
			&gadget->manufacturer, NULL);
	gadget->productname = xstrdup(barebox_get_model());
	dev_add_param_string(&gadget->dev, "productname", NULL, NULL,
			&gadget->productname, NULL);

	dev_set_name(&udc->dev, "udc");
	udc->dev.id = DEVICE_ID_DYNAMIC;

	udc->gadget = gadget;

	list_add_tail(&udc->list, &udc_list);

	register_device(&udc->dev);

	usb_gadget_set_state(gadget, USB_STATE_NOTATTACHED);

	return 0;
err2:
	kfree(udc);

err1:
	return ret;
}
EXPORT_SYMBOL_GPL(usb_add_gadget_udc_release);

/**
 * usb_add_gadget_udc - adds a new gadget to the udc class driver list
 * @parent: the parent device to this udc. Usually the controller
 * driver's device.
 * @gadget: the gadget to be added to the list
 *
 * Returns zero on success, negative errno otherwise.
 */
int usb_add_gadget_udc(struct device_d *parent, struct usb_gadget *gadget)
{
	return usb_add_gadget_udc_release(parent, gadget, NULL);
}
EXPORT_SYMBOL_GPL(usb_add_gadget_udc);

static void usb_gadget_remove_driver(struct usb_udc *udc)
{
	dev_dbg(&udc->dev, "unregistering UDC driver [%s]\n",
			udc->gadget->name);

	if (udc->gadget->ops->udc_poll)
		poller_unregister(&udc->poller);

	usb_gadget_disconnect(udc->gadget);
	udc->driver->disconnect(udc->gadget);
	udc->driver->unbind(udc->gadget);
	usb_gadget_udc_stop(udc->gadget, NULL);

	udc->driver = NULL;
	udc->dev.driver = NULL;
	udc->gadget->dev.driver = NULL;
}

/**
 * usb_del_gadget_udc - deletes @udc from udc_list
 * @gadget: the gadget to be removed.
 *
 * This, will call usb_gadget_unregister_driver() if
 * the @udc is still busy.
 */
void usb_del_gadget_udc(struct usb_gadget *gadget)
{
	struct usb_udc		*udc = NULL;

	list_for_each_entry(udc, &udc_list, list)
		if (udc->gadget == gadget)
			goto found;

	dev_err(gadget->dev.parent, "gadget not registered.\n");

	return;

found:
	dev_vdbg(gadget->dev.parent, "unregistering gadget\n");

	list_del(&udc->list);

	if (udc->driver)
		usb_gadget_remove_driver(udc);

	unregister_device(&udc->dev);
	unregister_device(&gadget->dev);
}
EXPORT_SYMBOL_GPL(usb_del_gadget_udc);

/* ------------------------------------------------------------------------- */

static void udc_poll_driver(struct poller_struct *poller)
{
	struct usb_udc *udc = container_of(poller, struct usb_udc, poller);

	udc->gadget->ops->udc_poll(udc->gadget);
}

static int udc_bind_to_driver(struct usb_udc *udc, struct usb_gadget_driver *driver)
{
	int ret;

	dev_dbg(&udc->dev, "registering UDC driver [%s]\n",
			driver->function);

	udc->driver = driver;
	udc->dev.driver = &driver->driver;
	udc->gadget->dev.driver = &driver->driver;

	if (udc->gadget->ops->udc_poll) {
		udc->poller.func = udc_poll_driver;
		ret = poller_register(&udc->poller);
		if (ret)
			return ret;
	}

	ret = driver->bind(udc->gadget, driver);
	if (ret)
		goto err1;

	ret = usb_gadget_udc_start(udc->gadget, driver);
	if (ret) {
		driver->unbind(udc->gadget);
		goto err1;
	}
	usb_gadget_connect(udc->gadget);

	return 0;
err1:
	if (udc->gadget->ops->udc_poll)
		poller_unregister(&udc->poller);

	if (ret != -EISNAM)
		dev_err(&udc->dev, "failed to start %s: %d\n",
			udc->driver->function, ret);
	udc->driver = NULL;
	udc->dev.driver = NULL;
	udc->gadget->dev.driver = NULL;
	return ret;
}

int udc_attach_driver(const char *name, struct usb_gadget_driver *driver)
{
	struct usb_udc *udc = NULL;
	int ret = -ENODEV;

	list_for_each_entry(udc, &udc_list, list) {
		ret = strcmp(name, dev_name(&udc->dev));
		if (!ret)
			break;
	}
	if (ret) {
		ret = -ENODEV;
		goto out;
	}
	if (udc->driver) {
		ret = -EBUSY;
		goto out;
	}
	ret = udc_bind_to_driver(udc, driver);
out:
	return ret;
}
EXPORT_SYMBOL_GPL(udc_attach_driver);

int usb_gadget_probe_driver(struct usb_gadget_driver *driver)
{
	struct usb_udc		*udc = NULL;
	int			ret;

	if (!driver || !driver->bind || !driver->setup)
		return -EINVAL;

	list_for_each_entry(udc, &udc_list, list) {
		/* For now we take the first one */
		if (!udc->driver)
			goto found;
	}

	pr_debug("couldn't find an available UDC\n");

	return -ENODEV;
found:
	ret = udc_bind_to_driver(udc, driver);

	return ret;
}
EXPORT_SYMBOL_GPL(usb_gadget_probe_driver);

int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct usb_udc		*udc = NULL;
	int			ret = -ENODEV;

	if (!driver || !driver->unbind)
		return -EINVAL;

	list_for_each_entry(udc, &udc_list, list)
		if (udc->driver == driver) {
			usb_gadget_remove_driver(udc);
			ret = 0;
			break;
		}

	return ret;
}
EXPORT_SYMBOL_GPL(usb_gadget_unregister_driver);
