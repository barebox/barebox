/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <errno.h>
#include <init.h>
#include <clock.h>
#include <poller.h>
#include <kfifo.h>
#include <i2c/i2c.h>
#include <malloc.h>
#include <readkey.h>
#include <input/qt1070.h>
#include <gpio.h>

#define QT1070_CHIP_ID		0x2e

#define QT1070_READ_CHIP_ID	0x00
#define QT1070_FW_VERSION	0x01
#define QT1070_DET_STATUS	0x02
#define QT1070_KEY_STATUS	0x03

/* Calibrate */
#define QT1070_CALIBRATE_CMD	0x38
#define QT1070_CAL_TIME		200

/* Reset */
#define QT1070_RESET		0x39
#define QT1070_RESET_TIME	255

static int default_code[QT1070_NB_BUTTONS] = {
	BB_KEY_ENTER, BB_KEY_HOME, BB_KEY_UP, BB_KEY_DOWN,
	BB_KEY_RIGHT, BB_KEY_LEFT, BB_KEY_CLEAR_SCREEN };

struct qt1070_data {
	int code[QT1070_NB_BUTTONS];

	int irq_pin;

	u64 start;

	/* optional */
	int fifo_size;

	struct i2c_client *client;
	u8 button_state[QT1070_NB_BUTTONS];
	u8 previous_state;

	struct kfifo *recv_fifo;
	struct poller_struct poller;
	struct console_device cdev;
};

static inline struct qt1070_data *
poller_to_qt_data(struct poller_struct *poller)
{
	return container_of(poller, struct qt1070_data, poller);
}

static inline struct qt1070_data *
cdev_to_qt_data(struct console_device *cdev)
{
	return container_of(cdev, struct qt1070_data, cdev);
}

static bool qt1070_read(struct qt1070_data *data, u8 reg, u8 *val)
{
	int ret;

	ret = i2c_read_reg(data->client, reg, val, 1);

	if (ret < 0)
		return ret;
	if (ret != 1)
		return -EIO;
	return 0;
}

static bool qt1070_write(struct qt1070_data *data, u8 reg, const u8 val)
{
	int ret;

	ret = i2c_write_reg(data->client, reg, &val, 1);
	if (ret < 0)
		return ret;
	if (ret != 1)
		return -EIO;
	return 0;
}

static void qt1070_poller(struct poller_struct *poller)
{
	struct qt1070_data *data = poller_to_qt_data(poller);
	int i;
	u8 buf, bt;
	u8 mask = 0x1;

	if (gpio_is_valid(data->irq_pin)) {
		/* active low */
		if (gpio_get_value(data->irq_pin))
			return;
	}

	qt1070_read(data, QT1070_DET_STATUS, &buf);

	if (qt1070_read(data, QT1070_KEY_STATUS, &buf))
		return;

	if (!buf & !data->previous_state)
		return;

	for (i = 0; i < QT1070_NB_BUTTONS; i++) {
		bt = buf & mask;

		if (!bt && data->button_state[i]) {
			dev_dbg(data->cdev.dev, "release key(%d) as %d\n", i, data->code[i]);
			kfifo_put(data->recv_fifo, (u_char*)&data->code[i], sizeof(int));
		} else if (bt) {
			dev_dbg(data->cdev.dev, "pressed key(%d) as %d\n", i, data->code[i]);
		}

		data->button_state[i] = bt;
		mask <<= 1;
	}

	data->previous_state = buf;
}

static void qt1070_reset_poller(struct poller_struct *poller)
{
	struct qt1070_data *data = poller_to_qt_data(poller);
	u8 buf;

	if (!is_timeout_non_interruptible(data->start, QT1070_RESET_TIME * MSECOND))
		return;

	/* clear the status */
	qt1070_read(data, QT1070_DET_STATUS, &buf);

	poller->func = qt1070_poller;
}

static void qt1070_calibrate_poller(struct poller_struct *poller)
{
	struct qt1070_data *data = poller_to_qt_data(poller);
	int ret;

	if (!is_timeout_non_interruptible(data->start, QT1070_CAL_TIME * MSECOND))
		return;

	/* Soft reset */
	ret = qt1070_write(data, QT1070_RESET, 1);
	if (ret) {
		dev_err(data->cdev.dev, "can not reset the chip (%d)\n", ret);
		poller_unregister(poller);
		return;
	}
	data->start = get_time_ns();

	poller->func = qt1070_reset_poller;
}

static int qt1070_tstc(struct console_device *cdev)
{
	struct qt1070_data *data = cdev_to_qt_data(cdev);

	return (kfifo_len(data->recv_fifo) == 0) ? 0 : 1;
}

static int qt1070_getc(struct console_device *cdev)
{
	int code = 0;
	struct qt1070_data *data = cdev_to_qt_data(cdev);

	kfifo_get(data->recv_fifo, (u_char*)&code, sizeof(int));
	return code;
}

static int qt1070_pdata_init(struct device_d *dev, struct qt1070_data *data)
{
	struct qt1070_platform_data *pdata = dev->platform_data;
	int ret;

	if (!pdata)
		return 0;

	if (pdata->nb_code)
		memcpy(data->code, pdata->code, sizeof(int) * pdata->nb_code);

	if (!gpio_is_valid(pdata->irq_pin))
		return 0;

	ret = gpio_request(pdata->irq_pin, "qt1070");
	if (ret)
		return ret;

	ret = gpio_direction_input(pdata->irq_pin);
	if (ret)
		goto err;

	data->irq_pin = pdata->irq_pin;

	return 0;
err:
	gpio_free(pdata->irq_pin);
	return ret;
}

static int qt1070_probe(struct device_d *dev)
{
	struct console_device *cdev;
	struct qt1070_data *data;
	u8 fw_version, chip_id;
	int ret;

	data = xzalloc(sizeof(*data));
	data->client = to_i2c_client(dev);
	data->irq_pin = -EINVAL;

	ret = qt1070_read(data, QT1070_READ_CHIP_ID, &chip_id);
	if (ret) {
		dev_err(dev, "can not read chip id (%d)\n", ret);
		goto err;
	}

	if (chip_id != QT1070_CHIP_ID) {
		dev_err(dev, "unsupported id 0x%x\n", chip_id);
		ret = -ENXIO;
		goto err;
	}

	ret = qt1070_read(data, QT1070_FW_VERSION, &fw_version);
	if (ret) {
		dev_err(dev, "can not read firmware version (%d)\n", ret);
		goto err;
	}

	dev_add_param_uint32_fixed(dev, "fw_version", fw_version, "0x%x");
	dev_add_param_uint32_fixed(dev, "chip_ip", chip_id, "0x%x");

	memcpy(data->code, default_code, sizeof(int) * ARRAY_SIZE(default_code));

	ret = qt1070_pdata_init(dev, data);
	if (ret) {
		dev_err(dev, "can not get pdata (%d)\n", ret);
		goto err;
	}

	/* Calibrate device */
	ret = qt1070_write(data, QT1070_CALIBRATE_CMD, 1);
	if (ret) {
		dev_err(dev, "can not calibrate the chip (%d)\n", ret);
		goto err;
	}
	data->start = get_time_ns();


	data->fifo_size = 50;
	data->recv_fifo = kfifo_alloc(data->fifo_size);

	data->poller.func = qt1070_calibrate_poller;

	cdev = &data->cdev;
	cdev->dev = dev;
	cdev->tstc = qt1070_tstc;
	cdev->getc = qt1070_getc;

	console_register(&data->cdev);

	ret = poller_register(&data->poller);
	if (ret)
		goto err;

	return 0;
err:
	free(data);
	return ret;
}

static struct driver_d qt1070_driver = {
	.name	= "qt1070",
	.probe	= qt1070_probe,
};

static int qt1070_init(void)
{
	i2c_driver_register(&qt1070_driver);
	return 0;
}
device_initcall(qt1070_init);
