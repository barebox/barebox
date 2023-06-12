// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Ahmad Fatoum

#include <common.h>
#include <driver.h>
#include <xfuncs.h>
#include <spi/spi.h>
#include <aiodev.h>

#define ST_GYRO_WHO_AM_I    0x0F
#define ST_GYRO_CTRL_REG1   0x20

#define ST_GYRO_DEFAULT_OUT_TEMP_ADDR           0x26
#define ST_GYRO_DEFAULT_OUT_X_L_ADDR            0x28
#define ST_GYRO_DEFAULT_OUT_Y_L_ADDR            0x2a
#define ST_GYRO_DEFAULT_OUT_Z_L_ADDR            0x2c

#define ST_GYRO_OUT_L_ADDR(idx)		\
	(ST_GYRO_DEFAULT_OUT_X_L_ADDR + 2 * (idx))

#define ST_GYRO_OUT_H_ADDR(idx)		\
	(ST_GYRO_OUT_L_ADDR(idx) + 1)

#define ST_GYRO_READ		0x80
#define ST_GYRO_WRITE		0x00
#define ST_GYRO_MULTI		0x40

struct st_gyro {
	struct aiodevice aiodev;
	struct aiochannel aiochan[4];
	struct spi_device *spi;
};
#define to_st_gyro(chan) container_of(chan->aiodev, struct st_gyro, aiodev)

static int st_gyro_read(struct aiochannel *chan, int *val)
{
	struct st_gyro *gyro = to_st_gyro(chan);
	int ret;
	u8 tx;
	u8 rx_h, rx_l;

	if (chan->index == 3) {
		tx = ST_GYRO_DEFAULT_OUT_TEMP_ADDR | ST_GYRO_READ;
		ret = spi_write_then_read(gyro->spi, &tx, 1, &rx_l, 1);
		if (ret)
			return ret;

		*val = (s8)rx_l;
		return 0;
	}

	tx = ST_GYRO_OUT_H_ADDR(chan->index) | ST_GYRO_READ;
	ret = spi_write_then_read(gyro->spi, &tx, 1, &rx_h, 1);
	if (ret)
		return ret;

	tx = ST_GYRO_OUT_L_ADDR(chan->index) | ST_GYRO_READ;
	ret = spi_write_then_read(gyro->spi, &tx, 1, &rx_l, 1);
	if (ret)
		return ret;

	*val = (s16)((rx_h << 8) | rx_l);
	*val *= 250;
	*val >>= 16;

	return 0;
}

static int st_gyro_probe(struct device *dev)
{
	u8 tx[2], rx[2];
	struct st_gyro *gyro;
	int ret, i;

	gyro = xzalloc(sizeof(*gyro));
	gyro->spi = to_spi_device(dev);

	tx[0] = ST_GYRO_WHO_AM_I | ST_GYRO_READ;
	ret = spi_write_then_read(gyro->spi, tx, 1, rx, 1);
	if (ret)
		return -EIO;
	if (rx[0] != 0xD4)
		return dev_err_probe(dev, -ENODEV, "unexpected device WAI: %02x\n", rx[0]);

	/* initialize device */
	tx[0] = ST_GYRO_CTRL_REG1 | ST_GYRO_WRITE;
	tx[1] = 0xF; /* normal mode, 3 channels */
	ret = spi_write(gyro->spi, tx, 2);
	if (ret)
		return -EIO;

	gyro->aiodev.num_channels = 4;
	gyro->aiodev.hwdev = dev;
	gyro->aiodev.read = st_gyro_read;
	gyro->aiodev.name = "gyroscope";
	gyro->aiodev.channels =
		xmalloc(gyro->aiodev.num_channels *
			sizeof(gyro->aiodev.channels[0]));
	for (i = 0; i < 3; i++) {
		gyro->aiodev.channels[i] = &gyro->aiochan[i];
		gyro->aiochan[i].unit = "dps";
		gyro->aiochan[i].index = i;
	}

	gyro->aiodev.channels[3] = &gyro->aiochan[3];
	gyro->aiochan[3].unit = "C";
	gyro->aiochan[3].index = 3;

	return aiodevice_register(&gyro->aiodev);
}

static const struct of_device_id st_gyro_match[] = {
	{ .compatible = "st,l3gd20-gyro"  },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, st_gyro_match);

static struct driver st_gyro_driver = {
	.name  = "st_gyro",
	.probe = st_gyro_probe,
	.of_compatible = st_gyro_match,
};
device_spi_driver(st_gyro_driver);
