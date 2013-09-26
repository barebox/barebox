/*
 * i2c.h - definitions for the barebox i2c framework
 *
 * Copyright (C) 2009 by Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 * Derived from:
 * - i2c.h - i.MX I2C driver header file
 *   Copyright (c) 2008, Darius Augulis <augulis.darius@gmail.com>
 * - i2c.h - definitions for the i2c-bus interface
 *   Copyright (C) 1995-2000 Simon G. Vogl
 *
 */

#ifndef I2C_I2C_H
#define I2C_I2C_H

#include <driver.h>
#include <linux/types.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*
 * struct i2c_platform_data - structure of platform data for MXC I2C driver
 * @param	bitrate	Bus speed measured in Hz
 *
 */
struct i2c_platform_data {
	int bitrate;
};

#define I2C_NAME_SIZE	20

#define I2C_M_RD		0x0001	/* read data, from slave to master */
#define I2C_M_DATA_ONLY		0x0002	/* transfer data bytes only */
#define I2C_M_TEN               0x0010  /* this is a ten bit chip address */
#define I2C_M_IGNORE_NAK        0x1000  /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_STOP		0x8000	/* if I2C_FUNC_PROTOCOL_MANGLING */

/**
 * struct i2c_msg - an I2C transaction segment beginning with START
 *
 * An i2c_msg is the low level representation of one segment of an I2C
 * transaction. It is visible to drivers in the @i2c_transfer()
 * procedure and to I2C adapter drivers through the
 * @i2c_adapter.@master_xfer() method.
 *
 * All I2C adapters implement the standard rules for I2C transactions.
 * Each transaction begins with a START. That is followed by the
 * slave address, and a bit encoding read versus write. Then follow
 * all the data bytes, The transfer terminates with a NAK, or when all
 * those bytes have been transferred and ACKed. If this is the last
 * message in a group, it is followed by a STOP. Otherwise it is
 * followed by the next @i2c_msg transaction segment, beginning with a
 * (repeated) START.
 *
 */
struct i2c_msg {
	__u8			*buf;	/**< The buffer into which data is read, or from which it's written. */
	__u16			addr;	/**< Slave address, seven bits */
	__u16			flags;	/**< I2C_M_RD is handled by all adapters */
	__u16			len;	/**< Number of data bytes in @buf being read from or written to the I2C slave address. */
};


/**
 * i2c_adapter is the structure used to identify a physical i2c bus
 * along with the access algorithms necessary to access it.
 *
 */
struct i2c_adapter {
	struct device_d		dev;	/* ptr to device */
	int			nr;	/* bus number */
	int (*master_xfer)(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);
	struct list_head	list;
	int			retries;
	void 			*algo_data;
};


struct i2c_client {
	struct device_d		dev;
	struct i2c_adapter	*adapter;
	unsigned short		addr;
};

#define to_i2c_client(a)	container_of(a, struct i2c_client, dev)


/**
 * struct i2c_board_info - template for device creation
 *
 * I2C doesn't actually support hardware probing, Drivers commonly
 * need more information than that, such as chip type, configuration,
 * and so on.
 *
 * i2c_board_info is used to build tables of information listing I2C
 * devices that are present. This information is used to grow the
 * driver model tree.  For mainboards this is done statically using
 * i2c_register_board_info(); bus numbers identify adapters that
 * aren't yet available. For add-on boards, i2c_new_device() does this
 * dynamically with the adapter already known.
 */
struct i2c_board_info {
	char		type[I2C_NAME_SIZE];	/**< name of device */
	unsigned short	addr;			/**< stored in i2c_client.addr */
	void		*platform_data;		/**< platform data for device */
	struct device_node *of_node;
};

/**
 * I2C_BOARD_INFO - macro used to list an i2c device and its address
 * @dev_type: identifies the device type
 * @dev_addr: the device's address on the bus.
 *
 * This macro initializes essential fields of a struct i2c_board_info,
 * declaring what has been provided on a particular board. Optional
 * fields (such as associated irq, or device-specific platform_data)
 * are provided using conventional syntax.
 */
#define I2C_BOARD_INFO(dev_type, dev_addr) \
	.type = dev_type, .addr = (dev_addr)

#ifdef CONFIG_I2C
extern int i2c_register_board_info(int busnum, struct i2c_board_info const *info, unsigned n);
#else
static inline int i2c_register_board_info(int busnum,
		struct i2c_board_info const *info, unsigned n)
{
	return 0;
}
#endif
extern int i2c_add_numbered_adapter(struct i2c_adapter *adapter);
struct i2c_adapter *i2c_get_adapter(int busnum);

/* For devices that use several addresses, use i2c_new_dummy() to make
 * client handles for the extra addresses.
 */
extern struct i2c_client *
i2c_new_dummy(struct i2c_adapter *adap, u16 address);



extern int i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num);
extern int i2c_master_send(struct i2c_client *client, const char *buf, int count);
extern int i2c_master_recv(struct i2c_client *client, char *buf, int count);


#define I2C_ADDR_16_BIT	(1 << 31)

extern int i2c_read_reg(struct i2c_client *client, u32 addr, u8 *buf, u16 count);
extern int i2c_write_reg(struct i2c_client *client, u32 addr, const u8 *buf, u16 count);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

extern struct bus_type i2c_bus;

static inline int i2c_driver_register(struct driver_d *drv)
{
	drv->bus = &i2c_bus;
	return register_driver(drv);
}

#endif /* I2C_I2C_H */
