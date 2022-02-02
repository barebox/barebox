// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * MIPI Display Bus Interface (DBI) LCD controller support
 *
 * Copyright 2016 Noralf Trønnes
 */

#define pr_fmt(fmt) "mipi-dbi: " fmt

#include <common.h>
#include <linux/kernel.h>
#include <linux/sizes.h>
#include <gpiod.h>
#include <regulator.h>
#include <spi/spi.h>
#include <video/mipi_dbi.h>

#include <video/vpl.h>
#include <video/mipi_display.h>
#include <video/fourcc.h>

#define MIPI_DBI_MAX_SPI_READ_SPEED 2000000 /* 2MHz */

#define DCS_POWER_MODE_DISPLAY			BIT(2)
#define DCS_POWER_MODE_DISPLAY_NORMAL_MODE	BIT(3)
#define DCS_POWER_MODE_SLEEP_MODE		BIT(4)
#define DCS_POWER_MODE_PARTIAL_MODE		BIT(5)
#define DCS_POWER_MODE_IDLE_MODE		BIT(6)
#define DCS_POWER_MODE_RESERVED_MASK		(BIT(0) | BIT(1) | BIT(7))

LIST_HEAD(mipi_dbi_list);
EXPORT_SYMBOL(mipi_dbi_list);

/**
 * DOC: overview
 *
 * This library provides helpers for MIPI Display Bus Interface (DBI)
 * compatible display controllers.
 *
 * Many controllers for tiny lcd displays are MIPI compliant and can use this
 * library. If a controller uses registers 0x2A and 0x2B to set the area to
 * update and uses register 0x2C to write to frame memory, it is most likely
 * MIPI compliant.
 *
 * Only MIPI Type 1 displays are supported since a full frame memory is needed.
 *
 * There are 3 MIPI DBI implementation types:
 *
 * A. Motorola 6800 type parallel bus
 *
 * B. Intel 8080 type parallel bus
 *
 * C. SPI type with 3 options:
 *
 *    1. 9-bit with the Data/Command signal as the ninth bit
 *    2. Same as above except it's sent as 16 bits
 *    3. 8-bit with the Data/Command signal as a separate D/CX pin
 *
 * Currently barebox mipi_dbi only supports Type C option 3 with
 * mipi_dbi_spi_init().
 */

#define MIPI_DBI_DEBUG_COMMAND(cmd, data, len) \
({ \
	if (!len) \
		pr_debug("cmd=%02x\n", cmd); \
	else if (len <= 32) \
		pr_debug("cmd=%02x, par=%*ph\n", cmd, (int)len, data);\
	else \
		pr_debug("cmd=%02x, len=%zu\n", cmd, len); \
})

static const u8 mipi_dbi_dcs_read_commands[] = {
	MIPI_DCS_GET_DISPLAY_ID,
	MIPI_DCS_GET_RED_CHANNEL,
	MIPI_DCS_GET_GREEN_CHANNEL,
	MIPI_DCS_GET_BLUE_CHANNEL,
	MIPI_DCS_GET_DISPLAY_STATUS,
	MIPI_DCS_GET_POWER_MODE,
	MIPI_DCS_GET_ADDRESS_MODE,
	MIPI_DCS_GET_PIXEL_FORMAT,
	MIPI_DCS_GET_DISPLAY_MODE,
	MIPI_DCS_GET_SIGNAL_MODE,
	MIPI_DCS_GET_DIAGNOSTIC_RESULT,
	MIPI_DCS_READ_MEMORY_START,
	MIPI_DCS_READ_MEMORY_CONTINUE,
	MIPI_DCS_GET_SCANLINE,
	MIPI_DCS_GET_DISPLAY_BRIGHTNESS,
	MIPI_DCS_GET_CONTROL_DISPLAY,
	MIPI_DCS_GET_POWER_SAVE,
	MIPI_DCS_GET_CABC_MIN_BRIGHTNESS,
	MIPI_DCS_READ_DDB_START,
	MIPI_DCS_READ_DDB_CONTINUE,
	0, /* sentinel */
};

bool mipi_dbi_command_is_read(struct mipi_dbi *dbi, u8 cmd)
{
	unsigned int i;

	if (!dbi->read_commands)
		return false;

	for (i = 0; i < 0xff; i++) {
		if (!dbi->read_commands[i])
			return false;
		if (cmd == dbi->read_commands[i])
			return true;
	}

	return false;
}

int mipi_dbi_command_read_len(int cmd)
{
	switch (cmd) {
	case MIPI_DCS_READ_MEMORY_START:
	case MIPI_DCS_READ_MEMORY_CONTINUE:
		return 2;
	case MIPI_DCS_GET_DISPLAY_ID:
		return 3;
	case MIPI_DCS_GET_DISPLAY_STATUS:
		return 4;
	default:
		return 1;
	}
}

/**
 * mipi_dbi_command_read - MIPI DCS read command
 * @dbi: MIPI DBI structure
 * @cmd: Command
 * @val: Value read
 *
 * Send MIPI DCS read command to the controller.
 *
 * Returns:
 * Zero on success, negative error code on failure.
 */
int mipi_dbi_command_read(struct mipi_dbi *dbi, u8 cmd, u8 *val)
{
	if (!dbi->read_commands)
		return -EACCES;

	if (!mipi_dbi_command_is_read(dbi, cmd))
		return -EINVAL;

	return mipi_dbi_command_buf(dbi, cmd, val, 1);
}
EXPORT_SYMBOL(mipi_dbi_command_read);

/**
 * mipi_dbi_command_buf - MIPI DCS command with parameter(s) in an array
 * @dbi: MIPI DBI structure
 * @cmd: Command
 * @data: Parameter buffer
 * @len: Buffer length
 *
 * Returns:
 * Zero on success, negative error code on failure.
 */
int mipi_dbi_command_buf(struct mipi_dbi *dbi, u8 cmd, u8 *data, size_t len)
{
	u8 *cmdbuf;
	int ret;

	/* SPI requires dma-safe buffers */
	cmdbuf = kmemdup(&cmd, 1, GFP_KERNEL);
	if (!cmdbuf)
		return -ENOMEM;

	ret = dbi->command(dbi, cmdbuf, data, len);

	kfree(cmdbuf);

	return ret;
}
EXPORT_SYMBOL(mipi_dbi_command_buf);

/* This should only be used by mipi_dbi_command() */
int mipi_dbi_command_stackbuf(struct mipi_dbi *dbi, u8 cmd, const u8 *data,
			      size_t len)
{
	u8 *buf;
	int ret;

	buf = kmemdup(data, len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = mipi_dbi_command_buf(dbi, cmd, buf, len);

	kfree(buf);

	return ret;
}
EXPORT_SYMBOL(mipi_dbi_command_stackbuf);

/**
 * mipi_dbi_hw_reset - Hardware reset of controller
 * @dbi: MIPI DBI structure
 *
 * Reset controller if the &mipi_dbi->reset gpio is set.
 */
void mipi_dbi_hw_reset(struct mipi_dbi *dbi)
{
	if (!gpio_is_valid(dbi->reset))
		return;

	gpiod_set_value(dbi->reset, 0);
	udelay(20);
	gpiod_set_value(dbi->reset, 1);
	mdelay(120);
}
EXPORT_SYMBOL(mipi_dbi_hw_reset);

/**
 * mipi_dbi_display_is_on - Check if display is on
 * @dbi: MIPI DBI structure
 *
 * This function checks the Power Mode register (if readable) to see if
 * display output is turned on. This can be used to see if the bootloader
 * has already turned on the display avoiding flicker when the pipeline is
 * enabled.
 *
 * Returns:
 * true if the display can be verified to be on, false otherwise.
 */
bool mipi_dbi_display_is_on(struct mipi_dbi *dbi)
{
	u8 val;

	if (mipi_dbi_command_read(dbi, MIPI_DCS_GET_POWER_MODE, &val))
		return false;

	val &= ~DCS_POWER_MODE_RESERVED_MASK;

	/* The poweron/reset value is 08h DCS_POWER_MODE_DISPLAY_NORMAL_MODE */
	if (val != (DCS_POWER_MODE_DISPLAY |
	    DCS_POWER_MODE_DISPLAY_NORMAL_MODE | DCS_POWER_MODE_SLEEP_MODE))
		return false;

	pr_debug("Display is ON\n");

	return true;
}
EXPORT_SYMBOL(mipi_dbi_display_is_on);

#if IS_ENABLED(CONFIG_SPI)

/**
 * mipi_dbi_spi_cmd_max_speed - get the maximum SPI bus speed
 * @spi: SPI device
 * @len: The transfer buffer length.
 *
 * Many controllers have a max speed of 10MHz, but can be pushed way beyond
 * that. Increase reliability by running pixel data at max speed and the rest
 * at 10MHz, preventing transfer glitches from messing up the init settings.
 */
u32 mipi_dbi_spi_cmd_max_speed(struct spi_device *spi, size_t len)
{
	if (len > 64)
		return 0; /* use default */

	return min_t(u32, 10000000, spi->max_speed_hz);
}
EXPORT_SYMBOL(mipi_dbi_spi_cmd_max_speed);

static bool mipi_dbi_machine_little_endian(void)
{
#if defined(__LITTLE_ENDIAN)
	return true;
#else
	return false;
#endif
}

/* MIPI DBI Type C Option 3 */

static int mipi_dbi_typec3_command_read(struct mipi_dbi *dbi, u8 *cmd,
					u8 *data, size_t len)
{
	struct spi_device *spi = dbi->spi;
	u32 speed_hz = min_t(u32, MIPI_DBI_MAX_SPI_READ_SPEED,
			     spi->max_speed_hz / 2);
	struct spi_transfer tr[2] = {
		{
			.speed_hz = speed_hz,
			.tx_buf = cmd,
			.len = 1,
		}, {
			.speed_hz = speed_hz,
			.len = len,
		},
	};
	struct spi_message m;
	u8 *buf;
	int ret;

	if (!len)
		return -EINVAL;

	/*
	 * Support non-standard 24-bit and 32-bit Nokia read commands which
	 * start with a dummy clock, so we need to read an extra byte.
	 */
	if (*cmd == MIPI_DCS_GET_DISPLAY_ID ||
	    *cmd == MIPI_DCS_GET_DISPLAY_STATUS) {
		if (!(len == 3 || len == 4))
			return -EINVAL;

		tr[1].len = len + 1;
	}

	buf = kmalloc(tr[1].len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	tr[1].rx_buf = buf;
	gpiod_set_value(dbi->dc, 0);

	spi_message_init_with_transfers(&m, tr, ARRAY_SIZE(tr));
	ret = spi_sync(spi, &m);
	if (ret)
		goto err_free;

	if (tr[1].len == len) {
		memcpy(data, buf, len);
	} else {
		unsigned int i;

		for (i = 0; i < len; i++)
			data[i] = (buf[i] << 1) | (buf[i + 1] >> 7);
	}

	MIPI_DBI_DEBUG_COMMAND(*cmd, data, len);

err_free:
	kfree(buf);

	return ret;
}

static int mipi_dbi_typec3_command(struct mipi_dbi *dbi, u8 *cmd,
				   u8 *par, size_t num)
{
	struct spi_device *spi = dbi->spi;
	unsigned int bpw = 8;
	u32 speed_hz;
	int ret;

	if (mipi_dbi_command_is_read(dbi, *cmd))
		return mipi_dbi_typec3_command_read(dbi, cmd, par, num);

	MIPI_DBI_DEBUG_COMMAND(*cmd, par, num);

	gpiod_set_value(dbi->dc, 0);
	speed_hz = mipi_dbi_spi_cmd_max_speed(spi, 1);
	ret = mipi_dbi_spi_transfer(spi, speed_hz, 8, cmd, 1);
	if (ret || !num)
		return ret;

	if (*cmd == MIPI_DCS_WRITE_MEMORY_START && !dbi->swap_bytes)
		bpw = 16;

	gpiod_set_value(dbi->dc, 1);
	speed_hz = mipi_dbi_spi_cmd_max_speed(spi, num);

	return mipi_dbi_spi_transfer(spi, speed_hz, bpw, par, num);
}

/**
 * mipi_dbi_spi_init - Initialize MIPI DBI SPI interface
 * @spi: SPI device
 * @dbi: MIPI DBI structure to initialize
 * @dc: D/C gpio
 *
 * This function sets &mipi_dbi->command, enables &mipi_dbi->read_commands for the
 * usual read commands. It should be followed by a call to mipi_dbi_dev_init() or
 * a driver-specific init.
 *
 * Type C Option 3 interface is assumed, Type C Option 1 is not yet supported,
 * because barebox has no generic way yet to require a 9-bit SPI transfer
 *
 * If the SPI master driver doesn't support the necessary bits per word,
 * the following transformation is used:
 *
 * - 9-bit: reorder buffer as 9x 8-bit words, padded with no-op command.
 * - 16-bit: if big endian send as 8-bit, if little endian swap bytes
 *
 * Returns:
 * Zero on success, negative error code on failure.
 */
int mipi_dbi_spi_init(struct spi_device *spi, struct mipi_dbi *dbi,
		      int dc)
{
	struct device_d *dev = &spi->dev;

	dbi->spi = spi;
	dbi->read_commands = mipi_dbi_dcs_read_commands;

	if (!gpio_is_valid(dc)) {
		dev_dbg(dev, "MIPI DBI Type-C 1 unsupported\n");
		return -ENOSYS;
	}

	dbi->command = mipi_dbi_typec3_command;
	dbi->dc = dc;
	// TODO: can we just force 16 bit?
	if (mipi_dbi_machine_little_endian() && spi->bits_per_word != 16)
		dbi->swap_bytes = true;

	dev_dbg(dev, "SPI speed: %uMHz\n", spi->max_speed_hz / 1000000);

	list_add(&dbi->list, &mipi_dbi_list);
	return 0;
}
EXPORT_SYMBOL(mipi_dbi_spi_init);

/**
 * mipi_dbi_spi_transfer - SPI transfer helper
 * @spi: SPI device
 * @speed_hz: Override speed (optional)
 * @bpw: Bits per word
 * @buf: Buffer to transfer
 * @len: Buffer length
 *
 * This SPI transfer helper breaks up the transfer of @buf into chunks which
 * the SPI controller driver can handle.
 *
 * Returns:
 * Zero on success, negative error code on failure.
 */
int mipi_dbi_spi_transfer(struct spi_device *spi, u32 speed_hz,
			  u8 bpw, const void *buf, size_t len)
{
	size_t max_chunk = spi_max_transfer_size(spi);
	struct spi_transfer tr = {
		.bits_per_word = bpw,
		.speed_hz = speed_hz,
	};
	struct spi_message m;
	size_t chunk;
	int ret;

	spi_message_init_with_transfers(&m, &tr, 1);

	while (len) {
		chunk = min(len, max_chunk);

		tr.tx_buf = buf;
		tr.len = chunk;
		buf += chunk;
		len -= chunk;

		ret = spi_sync(spi, &m);
		if (ret)
			return ret;
	}

	return 0;
}
EXPORT_SYMBOL(mipi_dbi_spi_transfer);

#endif /* CONFIG_SPI */

MODULE_LICENSE("GPL");
