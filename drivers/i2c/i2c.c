/*
 * Copyright (C) 2009 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 * Derived from:
 * - i2c-core.c - a device driver for the iic-bus interface
 *   Copyright (C) 1995-99 Simon G. Vogl
 * - at24.c - handle most I2C EEPROMs
 *   Copyright (C) 2005-2007 David Brownell
 *   Copyright (C) 2008 Wolfram Sang, Pengutronix
 * - spi.c - barebox SPI Framework
 *   Copyright (C) 2008 Sascha Hauer, Pengutronix
 * - Linux SPI Framework
 *   Copyright (C) 2005 David Brownell
 *
 */

#include <clock.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <xfuncs.h>
#include <init.h>
#include <of.h>
#include <gpio.h>

#include <i2c/i2c.h>

/**
 * I2C devices should normally not be created by I2C device drivers;
 * that would make them board-specific. Similarly with I2C master
 * drivers. Device registration normally goes into like
 * arch/.../mach.../board-YYY.c with other readonly (flashable)
 * information about mainboard devices.
 */
struct boardinfo {
	struct list_head	list;
	unsigned int		bus_num;
	unsigned int		n_board_info;
	struct i2c_board_info	board_info[0];
};

static LIST_HEAD(board_list);
LIST_HEAD(i2c_adapter_list);

/**
 * i2c_transfer - execute a single or combined I2C message
 * @param	adap	Handle to I2C bus
 * @param	msgs	One or more messages to execute before STOP is
 *			issued to terminate the operation; each
 *			message begins with a START.
 *
 * @param	num	Number of messages to be executed.
 *
 * Returns negative errno, else the number of messages executed.
 *
 * Note that there is no requirement that each message be sent to the
 * same slave address, although that is the most common model.
 */
int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	uint64_t start;
	int ret, try;

	/*
	 * REVISIT the fault reporting model here is weak:
	 *
	 *  - When we get an error after receiving N bytes from a slave,
	 *    there is no way to report "N".
	 *
	 *  - When we get a NAK after transmitting N bytes to a slave,
	 *    there is no way to report "N" ... or to let the master
	 *    continue executing the rest of this combined message, if
	 *    that's the appropriate response.
	 *
	 *  - When for example "num" is two and we successfully complete
	 *    the first message but get an error part way through the
	 *    second, it's unclear whether that should be reported as
	 *    one (discarding status on the second message) or errno
	 *    (discarding status on the first one).
	 */

	for (ret = 0; ret < num; ret++) {
		dev_dbg(&adap->dev, "master_xfer[%d] %c, addr=0x%02x, "
			"len=%d\n", ret, (msgs[ret].flags & I2C_M_RD)
			? 'R' : 'W', msgs[ret].addr, msgs[ret].len);
	}

	/* Retry automatically on arbitration loss */
	start = get_time_ns();
	for (ret = 0, try = 0; try <= 2; try++) {
		ret = adap->master_xfer(adap, msgs, num);
		if (ret != -EAGAIN)
			break;
		if (is_timeout(start, SECOND >> 1))
			break;
	}

	return ret;
}
EXPORT_SYMBOL(i2c_transfer);

/**
 * i2c_master_send - issue a single I2C message in master transmit mode
 *
 * @param	client	Handle to slave device
 * @param	buf	Data that will be written to the slave
 * @param	count	How many bytes to write
 *
 * Returns negative errno, or else the number of bytes written.
 */
int i2c_master_send(struct i2c_client *client, const char *buf, int count)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	int ret;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = count;
	msg.buf = (char *)buf;

	ret = i2c_transfer(adap, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg transmitted), return
	 * #bytes transmitted, else error code.
	 */
	return (ret == 1) ? count : ret;
}
EXPORT_SYMBOL(i2c_master_send);

/**
 * i2c_master_recv - issue a single I2C message in master receive mode
 *
 * @param	client	Handle to slave device
 * @param	buf	Where to store data read from slave
 * @param	count	How many bytes to read
 *
 * Returns negative errno, or else the number of bytes read.
 */
int i2c_master_recv(struct i2c_client *client, char *buf, int count)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	int ret;

	msg.addr = client->addr;
	msg.flags = I2C_M_RD;
	msg.len = count;
	msg.buf = buf;

	ret = i2c_transfer(adap, &msg, 1);

	/*
	 * If everything went ok (i.e. 1 msg transmitted), return
	 * #bytes transmitted, else error code.
	 */
	return (ret == 1) ? count : ret;
}
EXPORT_SYMBOL(i2c_master_recv);

int i2c_read_reg(struct i2c_client *client, u32 addr, u8 *buf, u16 count)
{
	u8 msgbuf[2];
	struct i2c_msg msg[] = {
		{
			.addr	= client->addr,
			.buf	= msgbuf,
		},
		{
			.addr	= client->addr,
			.flags	= I2C_M_RD,
			.buf	= buf,
			.len	= count,
		},
	};
	int status, i;

	i = 0;
	if (addr & I2C_ADDR_16_BIT)
		msgbuf[i++] = addr >> 8;
	msgbuf[i++] = addr;
	msg->len = i;

	status = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	dev_dbg(&client->adapter->dev, "%s: %u@%u --> %d\n", __func__,
		count, addr, status);

	if (status == ARRAY_SIZE(msg))
		return count;
	else if (status >= 0)
		return -EIO;
	else
		return status;
}
EXPORT_SYMBOL(i2c_read_reg);

int i2c_write_reg(struct i2c_client *client, u32 addr, const u8 *buf, u16 count)
{
	u8 msgbuf[256];				/* FIXME */
	struct i2c_msg msg[] = {
		{
			.addr	= client->addr,
			.buf	= msgbuf,
			.len	= count,
		}
	};
	int status, i;

	i = 0;
	if (addr & I2C_ADDR_16_BIT)
		msgbuf[i++] = addr >> 8;
	msgbuf[i++] = addr;
	msg->len += i;

	memcpy(msg->buf + i, buf, count);

	status = i2c_transfer(client->adapter, msg, ARRAY_SIZE(msg));
	dev_dbg(&client->adapter->dev, "%s: %u@%u --> %d\n", __func__,
		count, addr, status);

	if (status == ARRAY_SIZE(msg))
		return count;
	else if (status >= 0)
		return -EIO;
	else
		return status;
}
EXPORT_SYMBOL(i2c_write_reg);

/* i2c bus recovery routines */
int i2c_get_scl_gpio_value(struct i2c_adapter *adap)
{
	gpio_direction_input(adap->bus_recovery_info->scl_gpio);
	return gpio_get_value(adap->bus_recovery_info->scl_gpio);
}

void i2c_set_scl_gpio_value(struct i2c_adapter *adap, int val)
{
	if (val)
		gpio_direction_input(adap->bus_recovery_info->scl_gpio);
	else
		gpio_direction_output(adap->bus_recovery_info->scl_gpio, 0);
}

int i2c_get_sda_gpio_value(struct i2c_adapter *adap)
{
	return gpio_get_value(adap->bus_recovery_info->sda_gpio);
}

static int i2c_get_gpios_for_recovery(struct i2c_adapter *adap)
{
	struct i2c_bus_recovery_info *bri = adap->bus_recovery_info;
	struct device_d *dev = &adap->dev;
	int ret = 0;

	ret = gpio_request_one(bri->scl_gpio, GPIOF_IN, "i2c-scl");
	if (ret) {
		dev_warn(dev, "Can't get SCL gpio: %d\n", bri->scl_gpio);
		return ret;
	}

	if (bri->get_sda) {
		if (gpio_request_one(bri->sda_gpio, GPIOF_IN, "i2c-sda")) {
			/* work without SDA polling */
			dev_warn(dev, "Can't get SDA gpio: %d. Not using SDA polling\n",
					bri->sda_gpio);
			bri->get_sda = NULL;
		}
	}

	return ret;
}

static void i2c_put_gpios_for_recovery(struct i2c_adapter *adap)
{
	struct i2c_bus_recovery_info *bri = adap->bus_recovery_info;

	if (bri->get_sda)
		gpio_free(bri->sda_gpio);

	gpio_free(bri->scl_gpio);
}

/*
 * We are generating clock pulses. ndelay() determines durating of clk pulses.
 * We will generate clock with rate 100 KHz and so duration of both clock levels
 * is: delay in ns = (10^6 / 100) / 2
 */
#define RECOVERY_NDELAY		5000
#define RECOVERY_CLK_CNT	9

static int i2c_generic_recovery(struct i2c_adapter *adap)
{
	struct i2c_bus_recovery_info *bri = adap->bus_recovery_info;
	int i = 0, val = 1, ret = 0;

	if (bri->prepare_recovery)
		bri->prepare_recovery(adap);

	bri->set_scl(adap, val);
	ndelay(RECOVERY_NDELAY);

	/*
	 * By this time SCL is high, as we need to give 9 falling-rising edges
	 */
	while (i++ < RECOVERY_CLK_CNT * 2) {
		if (val) {
			/* Break if SDA is high */
			if (bri->get_sda && bri->get_sda(adap))
					break;
			/* SCL shouldn't be low here */
			if (!bri->get_scl(adap)) {
				dev_err(&adap->dev,
					"SCL is stuck low, exit recovery\n");
				ret = -EBUSY;
				break;
			}
		}

		val = !val;
		bri->set_scl(adap, val);
		ndelay(RECOVERY_NDELAY);
	}

	if (bri->unprepare_recovery)
		bri->unprepare_recovery(adap);

	return ret;
}

int i2c_generic_scl_recovery(struct i2c_adapter *adap)
{
	return i2c_generic_recovery(adap);
}

int i2c_generic_gpio_recovery(struct i2c_adapter *adap)
{
	int ret;

	ret = i2c_get_gpios_for_recovery(adap);
	if (ret)
		return ret;

	ret = i2c_generic_recovery(adap);
	i2c_put_gpios_for_recovery(adap);

	return ret;
}

int i2c_recover_bus(struct i2c_adapter *adap)
{
	if (!adap->bus_recovery_info)
		return -EOPNOTSUPP;

	dev_dbg(&adap->dev, "Trying i2c bus recovery\n");
	return adap->bus_recovery_info->recover_bus(adap);
}

static void i2c_info(struct device_d *dev)
{
	const struct i2c_client *client = to_i2c_client(dev);

	printf("  Address: 0x%02x\n", client->addr);
	return;
}

/**
 * i2c_new_device - instantiate one new I2C device
 *
 * @param	adapter	Controller to which device is connected
 * @param	chip	Describes the I2C device
 *
 * On typical mainboards, this is purely internal; and it's not needed
 * after board init creates the hard-wired devices. Some development
 * platforms may not be able to use i2c_register_board_info though,
 * and this is exported so that for example a USB or parport based
 * adapter driver could add devices (which it would learn about
 * out-of-band).
 *
 * Returns the new device, or NULL.
 */
static struct i2c_client *i2c_new_device(struct i2c_adapter *adapter,
					 struct i2c_board_info *chip)
{
	struct i2c_client *client;
	int status;

	client = xzalloc(sizeof *client);
	strcpy(client->dev.name, chip->type);
	client->dev.type_data = client;
	client->dev.platform_data = chip->platform_data;
	client->dev.id = DEVICE_ID_DYNAMIC;
	client->dev.bus = &i2c_bus;
	client->dev.device_node = chip->of_node;
	client->adapter = adapter;
	client->addr = chip->addr;

	client->dev.parent = &adapter->dev;

	status = register_device(&client->dev);
	if (status) {
		free(client);
		return NULL;
	}
	client->dev.info = i2c_info;

	return client;
}

static void of_i2c_register_devices(struct i2c_adapter *adap)
{
	struct device_node *n;

	/* Only register child devices if the adapter has a node pointer set */
	if (!IS_ENABLED(CONFIG_OFDEVICE) || !adap->dev.device_node)
		return;

	for_each_available_child_of_node(adap->dev.device_node, n) {
		struct i2c_board_info info = {};
		struct i2c_client *result;
		const __be32 *addr;
		int len;

		of_modalias_node(n, info.type, I2C_NAME_SIZE);

		info.of_node = n;

		addr = of_get_property(n, "reg", &len);
		if (!addr || (len < sizeof(int))) {
			dev_err(&adap->dev, "of_i2c: invalid reg on %s\n",
				n->full_name);
			continue;
		}

		info.addr = be32_to_cpup(addr);
		if (info.addr > (1 << 10) - 1) {
			dev_err(&adap->dev, "of_i2c: invalid addr=%x on %s\n",
				info.addr, n->full_name);
			continue;
		}

		result = i2c_new_device(adap, &info);
		if (!result)
			dev_err(&adap->dev, "of_i2c: Failure registering %s\n",
			        n->full_name);
	}
}

/**
 * i2c_new_dummy - return a new i2c device bound to a dummy driver
 * @adapter: the adapter managing the device
 * @address: seven bit address to be used
 * Context: can sleep
 *
 * This returns an I2C client bound to the "dummy" driver, intended for use
 * with devices that consume multiple addresses.  Examples of such chips
 * include various EEPROMS (like 24c04 and 24c08 models).
 *
 * These dummy devices have two main uses.  First, most I2C and SMBus calls
 * except i2c_transfer() need a client handle; the dummy will be that handle.
 * And second, this prevents the specified address from being bound to a
 * different driver.
 *
 * This returns the new i2c client, which should be saved for later use with
 * i2c_unregister_device(); or NULL to indicate an error.
 */
struct i2c_client *i2c_new_dummy(struct i2c_adapter *adapter, u16 address)
{
	struct i2c_board_info info = {
		I2C_BOARD_INFO("dummy", address),
	};

	return i2c_new_device(adapter, &info);
}
EXPORT_SYMBOL_GPL(i2c_new_dummy);

/**
 * i2c_register_board_info - register I2C devices for a given board
 *
 * @param	info	array of chip descriptors
 * @param	n	how many descriptors are provided
 *
 * Board-specific early init code calls this (probably during
 * arch_initcall) with segments of the I2C device table.
 *
 * Other code can also call this, e.g. a particular add-on board might
 * provide I2C devices through its expansion connector, so code
 * initializing that board would naturally declare its I2C devices.
 *
 */
int i2c_register_board_info(int bus_num, struct i2c_board_info const *info, unsigned n)
{
	struct boardinfo *bi;

	bi = xmalloc(sizeof(*bi) + n * sizeof(*info));

	bi->n_board_info = n;
	bi->bus_num = bus_num;
	memcpy(bi->board_info, info, n * sizeof(*info));

	list_add_tail(&bi->list, &board_list);

	return 0;
}

static void scan_boardinfo(struct i2c_adapter *adapter)
{
	struct boardinfo	*bi;

	list_for_each_entry(bi, &board_list, list) {
		struct i2c_board_info *chip = bi->board_info;
		unsigned n;

		if (bi->bus_num != adapter->nr)
			continue;

		for (n = bi->n_board_info; n > 0; n--, chip++) {
			debug("%s: bus_num: %d, chip->addr 0x%02x\n", __func__, bi->bus_num, chip->addr);
			/*
			 * NOTE: this relies on i2c_new_device to
			 * issue diagnostics when given bogus inputs
			 */
			(void) i2c_new_device(adapter, chip);
		}
	}
}

/**
 *
 * i2c_get_adapter - get an i2c adapter from its busnum
 *
 * @param	busnum	the desired bus number
 *
 */
struct i2c_adapter *i2c_get_adapter(int busnum)
{
	struct i2c_adapter *adap;

	for_each_i2c_adapter(adap)
		if (adap->nr == busnum)
			return adap;
	return NULL;
}

struct i2c_adapter *of_find_i2c_adapter_by_node(struct device_node *node)
{
	struct i2c_adapter *adap;

	for_each_i2c_adapter(adap)
		if (adap->dev.device_node == node)
			return adap;

	return NULL;
}

/**
 * i2c_add_numbered_adapter - declare i2c adapter, use static bus number
 * @adapter: the adapter to register (with adap->nr initialized)
 *
 * This routine is used to declare an I2C adapter when its bus number
 * matters.  For example, use it for I2C adapters from system-on-chip CPUs,
 * or otherwise built in to the system's mainboard, and where i2c_board_info
 * is used to properly configure I2C devices.
 *
 * When this returns zero, the specified adapter became available for
 * clients using the bus number provided in adap->nr.  Also, the table
 * of I2C devices pre-declared using i2c_register_board_info() is scanned,
 * and the appropriate driver model device nodes are created.  Otherwise, a
 * negative errno value is returned.
 */
int i2c_add_numbered_adapter(struct i2c_adapter *adapter)
{
	int ret;

	if (adapter->nr < 0) {
		int nr;

		for (nr = 0;; nr++)
			if (!i2c_get_adapter(nr))
				break;
		adapter->nr = nr;
	} else {
		if (i2c_get_adapter(adapter->nr))
			return -EBUSY;
	}

	adapter->dev.id = adapter->nr;
	strcpy(adapter->dev.name, "i2c");

	ret = register_device(&adapter->dev);
	if (ret)
		return ret;

	list_add_tail(&adapter->list, &i2c_adapter_list);

	/* populate children from any i2c device tables */
	scan_boardinfo(adapter);

	of_i2c_register_devices(adapter);

	return 0;
}
EXPORT_SYMBOL(i2c_add_numbered_adapter);

static int i2c_probe(struct device_d *dev)
{
	return dev->driver->probe(dev);
}

static void i2c_remove(struct device_d *dev)
{
	if (dev->driver->remove)
		dev->driver->remove(dev);
}

struct bus_type i2c_bus = {
	.name = "i2c",
	.match = device_match_of_modalias,
	.probe = i2c_probe,
	.remove = i2c_remove,
};

static int i2c_bus_init(void)
{
	return bus_register(&i2c_bus);
}
pure_initcall(i2c_bus_init);
