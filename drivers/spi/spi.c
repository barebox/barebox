// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2008 Sascha Hauer, Pengutronix
 *
 * Derived from Linux SPI Framework
 *
 * Copyright (C) 2005 David Brownell
 */

#include <common.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/spi/spi-mem.h>
#include <spi/spi.h>
#include <xfuncs.h>
#include <malloc.h>
#include <slice.h>
#include <errno.h>
#include <init.h>
#include <of.h>

/* SPI devices should normally not be created by SPI device drivers; that
 * would make them board-specific.  Similarly with SPI master drivers.
 * Device registration normally goes into like arch/.../mach.../board-YYY.c
 * with other readonly (flashable) information about mainboard devices.
 */

struct boardinfo {
	struct list_head	list;
	unsigned		n_board_info;
	struct spi_board_info	board_info[0];
};

static LIST_HEAD(board_list);
static LIST_HEAD(spi_controller_list);

/**
 * spi_set_cs_timing - configure CS setup, hold, and inactive delays
 * @spi: the device that requires specific CS timing configuration
 *
 * Return: zero on success, else a negative error code.
 */
static int spi_set_cs_timing(struct spi_device *spi)
{
	int status = 0;

	if (spi->controller->set_cs_timing && !spi->cs_gpiod)
		status = spi->controller->set_cs_timing(spi);

	return status;
}

/**
 * spi_new_device - instantiate one new SPI device
 * @master: Controller to which device is connected
 * @chip: Describes the SPI device
 * Context: can sleep
 *
 * On typical mainboards, this is purely internal; and it's not needed
 * after board init creates the hard-wired devices.  Some development
 * platforms may not be able to use spi_register_board_info though, and
 * this is exported so that for example a USB or parport based adapter
 * driver could add devices (which it would learn about out-of-band).
 *
 * Returns the new device, or NULL.
 */
struct spi_device *spi_new_device(struct spi_controller *ctrl,
				  struct spi_board_info *chip)
{
	struct spi_device	*proxy;
	struct spi_mem		*mem;
	int			status;

	/* Chipselects are numbered 0..max; validate. */
	if (chip->chip_select >= ctrl->num_chipselect) {
		debug("cs%d > max %d\n",
			chip->chip_select,
			ctrl->num_chipselect);
		return NULL;
	}

	proxy = xzalloc(sizeof *proxy);
	proxy->master = ctrl;
	proxy->chip_select = chip->chip_select;
	if (ctrl->cs_gpiods)
		proxy->cs_gpiod = ctrl->cs_gpiods[chip->chip_select];
	proxy->max_speed_hz = chip->max_speed_hz;
	proxy->mode = chip->mode;
	proxy->cs_setup = chip->cs_setup;
	proxy->cs_hold = chip->cs_hold;
	proxy->cs_inactive = chip->cs_inactive;
	proxy->bits_per_word = chip->bits_per_word ? chip->bits_per_word : 8;
	proxy->dev.platform_data = chip->platform_data;
	proxy->dev.bus = &spi_bus;
	dev_set_name(&proxy->dev, chip->name);
	/* allocate a free id for this chip */
	proxy->dev.id = DEVICE_ID_DYNAMIC;
	proxy->dev.type_data = proxy;
	proxy->dev.of_node = chip->device_node;
	proxy->dev.parent = ctrl->dev;
	proxy->master = proxy->controller = ctrl;

	mem = xzalloc(sizeof *mem);
	mem->spi = proxy;

	if (ctrl->mem_ops && ctrl->mem_ops->get_name)
		mem->name = ctrl->mem_ops->get_name(mem);
	else
		mem->name = dev_name(&proxy->dev);
	proxy->mem = mem;

	/* drivers may modify this initial i/o setup */
	status = ctrl->setup(proxy);
	if (status < 0) {
		printf("can't setup %s, status %d\n",
				proxy->dev.name, status);
		goto fail;
	}

	status = spi_set_cs_timing(proxy);
	if (status)
		goto fail;

	status = register_device(&proxy->dev);
	if (status)
		goto fail;

	chip->device_node->dev = &proxy->dev;

	return proxy;
fail:
	free(proxy);
	return NULL;
}
EXPORT_SYMBOL(spi_new_device);

static void of_spi_parse_dt_cs_delay(struct device_node *nc,
				     struct spi_delay *delay, const char *prop)
{
	u32 value;

	if (!of_property_read_u32(nc, prop, &value)) {
		if (value > U16_MAX) {
			delay->value = DIV_ROUND_UP(value, 1000);
			delay->unit = SPI_DELAY_UNIT_USECS;
		} else {
			delay->value = value;
			delay->unit = SPI_DELAY_UNIT_NSECS;
		}
	}
}

static void spi_of_register_slaves(struct spi_controller *ctrl)
{
	struct device_node *n;
	struct property *reg;
	struct device_node *node = ctrl->dev->of_node;

	if (!IS_ENABLED(CONFIG_OFDEVICE))
		return;

	if (!node)
		return;

	for_each_available_child_of_node(node, n) {
		struct spi_board_info chip = {};

		if (n->dev) {
			dev_dbg(ctrl->dev, "skipping already registered %s\n",
				dev_name(n->dev));
			continue;
		}

		chip.name = xstrdup(n->name);
		chip.bus_num = ctrl->bus_num;
		/* Mode (clock phase/polarity/etc.) */
		if (of_property_read_bool(n, "spi-cpha"))
			chip.mode |= SPI_CPHA;
		if (of_property_read_bool(n, "spi-cpol"))
			chip.mode |= SPI_CPOL;
		if (of_property_read_bool(n, "spi-cs-high"))
			chip.mode |= SPI_CS_HIGH;
		if (of_property_read_bool(n, "spi-3wire"))
			chip.mode |= SPI_3WIRE;
		of_property_read_u32(n, "spi-max-frequency",
				&chip.max_speed_hz);

		/* Device CS delays */
		of_spi_parse_dt_cs_delay(n, &chip.cs_setup, "spi-cs-setup-delay-ns");
		of_spi_parse_dt_cs_delay(n, &chip.cs_hold, "spi-cs-hold-delay-ns");
		of_spi_parse_dt_cs_delay(n, &chip.cs_inactive, "spi-cs-inactive-delay-ns");

		reg = of_find_property(n, "reg", NULL);
		if (!reg)
			continue;
		chip.chip_select = of_read_number(reg->value, 1);
		chip.device_node = n;
		spi_new_device(ctrl, &chip);
	}
}

static void spi_controller_rescan(struct device *dev)
{
	struct spi_controller *ctrl;

	list_for_each_entry(ctrl, &spi_controller_list, list) {
		if (ctrl->dev != dev)
			continue;
		spi_of_register_slaves(ctrl);
	}
}

/**
 * spi_register_board_info - register SPI devices for a given board
 * @info: array of chip descriptors
 * @n: how many descriptors are provided
 * Context: can sleep
 *
 * Board-specific early init code calls this (probably during arch_initcall)
 * with segments of the SPI device table.  Any device nodes are created later,
 * after the relevant parent SPI controller (bus_num) is defined.  We keep
 * this table of devices forever, so that reloading a controller driver will
 * not make Linux forget about these hard-wired devices.
 *
 * Other code can also call this, e.g. a particular add-on board might provide
 * SPI devices through its expansion connector, so code initializing that board
 * would naturally declare its SPI devices.
 *
 * The board info passed can safely be __initdata ... but be careful of
 * any embedded pointers (platform_data, etc), they're copied as-is.
 */
int
spi_register_board_info(struct spi_board_info const *info, int n)
{
	struct boardinfo	*bi;

	bi = xmalloc(sizeof(*bi) + n * sizeof *info);

	bi->n_board_info = n;
	memcpy(bi->board_info, info, n * sizeof *info);

	list_add_tail(&bi->list, &board_list);

	return 0;
}

static void scan_boardinfo(struct spi_controller *ctrl)
{
	struct boardinfo	*bi;

	list_for_each_entry(bi, &board_list, list) {
		struct spi_board_info	*chip = bi->board_info;
		unsigned		n;

		for (n = bi->n_board_info; n > 0; n--, chip++) {
			debug("%s %d %d\n", __FUNCTION__, chip->bus_num, ctrl->bus_num);
			if (chip->bus_num != ctrl->bus_num)
				continue;
			/* NOTE: this relies on spi_new_device to
			 * issue diagnostics when given bogus inputs
			 */
			(void) spi_new_device(ctrl, chip);
		}
	}
}

/**
 * spi_get_gpio_descs() - grab chip select GPIOs for the master
 * @ctlr: The SPI master to grab GPIO descriptors for
 */
static int spi_get_gpio_descs(struct spi_controller *ctlr)
{
	int nb, i;
	struct gpio_desc **cs;
	struct device *dev = ctlr->dev;

	nb = gpiod_count(dev, "cs");
	if (nb < 0) {
		/* No GPIOs at all is fine, else return the error */
		if (nb == -ENOENT)
			return 0;
		return nb;
	}

	ctlr->num_chipselect = max_t(int, nb, ctlr->num_chipselect);

	cs = devm_kcalloc(dev, ctlr->num_chipselect, sizeof(*cs),
			  GFP_KERNEL);
	if (!cs)
		return -ENOMEM;
	ctlr->cs_gpiods = cs;

	for (i = 0; i < nb; i++) {
		/*
		 * Most chipselects are active low, the inverted
		 * semantics are handled by special quirks in gpiolib,
		 * so initializing them GPIOD_OUT_LOW here means
		 * "unasserted", in most cases this will drive the physical
		 * line high.
		 */
		cs[i] = gpiod_get_index_optional(dev, "cs", i, GPIOD_OUT_LOW);
		if (IS_ERR(cs[i]))
			return PTR_ERR(cs[i]);

		if (cs[i]) {
			/*
			 * If we find a CS GPIO, name it after the device and
			 * chip select line.
			 */
			char *gpioname;

			gpioname = xasprintf("%s CS%d", dev_name(dev), i);
			gpiod_set_consumer_name(cs[i], gpioname);
			free(gpioname);
		}
	}

	return 0;
}

static void _spi_transfer_delay_ns(u32 ns)
{
	if (!ns)
		return;
	if (ns <= NSEC_PER_USEC) {
		ndelay(ns);
	} else {
		u32 us = DIV_ROUND_UP(ns, NSEC_PER_USEC);

		udelay(us);
	}
}

int spi_delay_to_ns(struct spi_delay *_delay, struct spi_transfer *xfer)
{
	u32 delay = _delay->value;
	u32 unit = _delay->unit;
	u32 hz;

	if (!delay)
		return 0;

	switch (unit) {
	case SPI_DELAY_UNIT_USECS:
		delay *= NSEC_PER_USEC;
		break;
	case SPI_DELAY_UNIT_NSECS:
		/* Nothing to do here */
		break;
	case SPI_DELAY_UNIT_SCK:
		/* Clock cycles need to be obtained from spi_transfer */
		if (!xfer)
			return -EINVAL;
		/*
		 * If there is unknown effective speed, approximate it
		 * by underestimating with half of the requested Hz.
		 */
		hz = xfer->effective_speed_hz ?: xfer->speed_hz / 2;
		if (!hz)
			return -EINVAL;

		/* Convert delay to nanoseconds */
		delay *= DIV_ROUND_UP(NSEC_PER_SEC, hz);
		break;
	default:
		return -EINVAL;
	}

	return delay;
}
EXPORT_SYMBOL_GPL(spi_delay_to_ns);

int spi_delay_exec(struct spi_delay *_delay, struct spi_transfer *xfer)
{
	int delay;

	if (!_delay)
		return -EINVAL;

	delay = spi_delay_to_ns(_delay, xfer);
	if (delay < 0)
		return delay;

	_spi_transfer_delay_ns(delay);

	return 0;
}
EXPORT_SYMBOL_GPL(spi_delay_exec);

static void spi_set_cs(struct spi_device *spi, bool enable)
{
        bool activate = enable;

        if (spi->cs_gpiod && !activate)
                spi_delay_exec(&spi->cs_hold, NULL);

        if (spi->mode & SPI_CS_HIGH)
                enable = !enable;

        if (spi->cs_gpiod) {
                if (!(spi->mode & SPI_NO_CS)) {
			/* Polarity handled by GPIO library */
			gpiod_set_value(spi->cs_gpiod, activate);
                }
                /* Some SPI masters need both GPIO CS & slave_select */
                if ((spi->controller->flags & SPI_CONTROLLER_GPIO_SS) &&
                    spi->controller->set_cs)
                        spi->controller->set_cs(spi, !enable);
        } else if (spi->controller->set_cs) {
                spi->controller->set_cs(spi, !enable);
        }

        if (spi->cs_gpiod || !spi->controller->set_cs_timing) {
                if (activate)
                        spi_delay_exec(&spi->cs_setup, NULL);
                else
                        spi_delay_exec(&spi->cs_inactive, NULL);
        }
}

/*
 * spi_transfer_one_message - Default implementation of transfer()
 *
 * This is a standard implementation of transfer() for drivers which implement a
 * transfer_one() operation. It provides standard handling of delays and chip
 * select management.
 *
 */
static int spi_transfer_one_message(struct spi_device *spi, struct spi_message *msg)
{
	struct spi_controller *ctlr = spi->controller;
	struct spi_transfer *xfer;
	bool keep_cs = false;
	int ret = 0;

	if (ctlr->prepare_message) {
		ret = ctlr->prepare_message(ctlr, msg);
		if (ret) {
			dev_err(ctlr->dev, "failed to prepare message: %d\n",
				ret);
			msg->status = ret;
			return ret;
		}
	}

	spi_set_cs(msg->spi, true);

	list_for_each_entry(xfer, &msg->transfers, transfer_list) {
		if ((xfer->tx_buf || xfer->rx_buf) && xfer->len) {
			ret = ctlr->transfer_one(ctlr, msg->spi, xfer);
			if (ret < 0) {
				dev_err(&msg->spi->dev,
					"SPI transfer failed: %d\n", ret);
				goto out;
			}
		} else {
			if (xfer->len)
				dev_err(&msg->spi->dev,
					"Bufferless transfer has length %u\n",
					xfer->len);
		}

		if (msg->status != -EINPROGRESS)
			goto out;

		/* TODO: Convert to new spi_delay API */
		if (xfer->delay_usecs)
			udelay(xfer->delay_usecs);

		if (xfer->cs_change) {
			if (list_is_last(&xfer->transfer_list,
					 &msg->transfers)) {
				keep_cs = true;
			} else {
				spi_set_cs(msg->spi, false);
				/* TODO: Convert to new spi_delay API */
				udelay(10);
				spi_set_cs(msg->spi, true);
			}
		}

		msg->actual_length += xfer->len;
	}

out:
	if (ret != 0 || !keep_cs)
		spi_set_cs(msg->spi, false);

	if (msg->status == -EINPROGRESS)
		msg->status = ret;

	if (msg->status && ctlr->handle_err)
		ctlr->handle_err(ctlr, msg);

	return ret;
}

static int spi_controller_check_ops(struct spi_controller *ctlr)
{
	/*
	 * The controller may implement only the high-level SPI-memory like
	 * operations if it does not support regular SPI transfers, and this is
	 * valid use case.
	 * If ->mem_ops is NULL, we request that at least one of the
	 * ->transfer_xxx() method be implemented.
	 */
	if (ctlr->mem_ops) {
		if (!ctlr->mem_ops->exec_op)
			return -EINVAL;
	} else if (!ctlr->transfer && !ctlr->transfer_one) {
		return -EINVAL;
	}

	return 0;
}


/**
 * spi_register_ctrl - register SPI ctrl controller
 * @ctrl: initialized ctrl, originally from spi_alloc_ctrl()
 * Context: can sleep
 *
 * SPI controllers connect to their drivers using some non-SPI bus,
 * such as the platform bus.  The final stage of probe() in that code
 * includes calling spi_register_ctrl() to hook up to this SPI bus glue.
 *
 * SPI controllers use board specific (often SOC specific) bus numbers,
 * and board-specific addressing for SPI devices combines those numbers
 * with chip select numbers.  Since SPI does not directly support dynamic
 * device identification, boards need configuration tables telling which
 * chip is at which address.
 *
 * This must be called from context that can sleep.  It returns zero on
 * success, else a negative error code (dropping the ctrl's refcount).
 * After a successful return, the caller is responsible for calling
 * spi_unregister_ctrl().
 */
int spi_register_controller(struct spi_controller *ctrl)
{
	static int dyn_bus_id = (1 << 15) - 1;
	int			status = -ENODEV;

	debug("%s: %s:%d\n", __func__, ctrl->dev->name, ctrl->dev->id);

	/*
	 * Make sure all necessary hooks are implemented before registering
	 * the SPI controller.
	 */
	status = spi_controller_check_ops(ctrl);
	if (status)
		return status;

	if (ctrl->transfer_one)
		ctrl->transfer = spi_transfer_one_message;

	slice_init(&ctrl->slice, dev_name(ctrl->dev));

	/* even if it's just one always-selected device, there must
	 * be at least one chipselect
	 */
	if (ctrl->num_chipselect == 0)
		return -EINVAL;

	if ((ctrl->bus_num < 0) && ctrl->dev->of_node)
		ctrl->bus_num = of_alias_get_id(ctrl->dev->of_node, "spi");

	/* convention:  dynamically assigned bus IDs count down from the max */
	if (ctrl->bus_num < 0)
		ctrl->bus_num = dyn_bus_id--;

	if (ctrl->use_gpio_descriptors) {
		status = spi_get_gpio_descs(ctrl);
		if (status)
			return status;
	}

	list_add_tail(&ctrl->list, &spi_controller_list);

	spi_of_register_slaves(ctrl);

	/* populate children from any spi device tables */
	scan_boardinfo(ctrl);
	status = 0;

	if (!ctrl->dev->rescan)
		ctrl->dev->rescan = spi_controller_rescan;

	return status;
}
EXPORT_SYMBOL(spi_register_controller);

struct spi_controller *spi_get_controller(int bus)
{
	struct spi_controller* m;

	list_for_each_entry(m, &spi_controller_list, list) {
		if (m->bus_num == bus)
			return m;
	}

	return NULL;
}

static int __spi_validate(struct spi_device *spi, struct spi_message *message)
{
	struct spi_controller *ctlr = spi->controller;
	struct spi_transfer *xfer;
	int w_size;

	if (list_empty(&message->transfers))
		return -EINVAL;

	message->spi = spi;

	list_for_each_entry(xfer, &message->transfers, transfer_list) {
		xfer->effective_speed_hz = 0;
		if (!xfer->bits_per_word)
			xfer->bits_per_word = spi->bits_per_word;

		if (!xfer->speed_hz)
			xfer->speed_hz = spi->max_speed_hz;

		if (ctlr->max_speed_hz && xfer->speed_hz > ctlr->max_speed_hz)
			xfer->speed_hz = ctlr->max_speed_hz;

		/*
		 * SPI transfer length should be multiple of SPI word size
		 * where SPI word size should be power-of-two multiple
		 */
		if (xfer->bits_per_word <= 8)
			w_size = 1;
		else if (xfer->bits_per_word <= 16)
			w_size = 2;
		else
			w_size = 4;

		/* No partial transfers accepted */
		if (xfer->len % w_size)
			return -EINVAL;
	}

	message->actual_length = 0;
	message->status = -EINPROGRESS;

	return 0;
}

int spi_sync(struct spi_device *spi, struct spi_message *message)
{
	int status;
	int ret;

	if (!spi->controller->transfer)
		return -ENOTSUPP;

	status = __spi_validate(spi, message);
	if (status != 0)
		return status;

	slice_acquire(&spi->controller->slice);

	ret = spi->controller->transfer(spi, message);

	slice_release(&spi->controller->slice);

	return ret;
}

/**
 * spi_write_then_read - SPI synchronous write followed by read
 * @spi: device with which data will be exchanged
 * @txbuf: data to be written
 * @n_tx: size of txbuf, in bytes
 * @rxbuf: buffer into which data will be read
 * @n_rx: size of rxbuf, in bytes
 * Context: can sleep
 *
 * This performs a half duplex MicroWire style transaction with the
 * device, sending txbuf and then reading rxbuf.  The return value
 * is zero for success, else a negative errno status code.
 * This call may only be used from a context that may sleep.
 */
int spi_write_then_read(struct spi_device *spi,
		const void *txbuf, unsigned n_tx,
		void *rxbuf, unsigned n_rx)
{
	int			status;
	struct spi_message	message;
	struct spi_transfer	x[2];

	spi_message_init(&message);
	memset(x, 0, sizeof x);
	if (n_tx) {
		x[0].len = n_tx;
		spi_message_add_tail(&x[0], &message);
	}
	if (n_rx) {
		x[1].len = n_rx;
		spi_message_add_tail(&x[1], &message);
	}

	x[0].tx_buf = txbuf;
	x[1].rx_buf = rxbuf;

	/* do the i/o */
	status = spi_sync(spi, &message);
	return status;
}
EXPORT_SYMBOL(spi_write_then_read);

struct bus_type spi_bus = {
	.name = "spi",
	.match = device_match_of_modalias,
};

static int spi_bus_init(void)
{
	return bus_register(&spi_bus);
}
pure_initcall(spi_bus_init);
