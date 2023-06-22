// SPDX-License-Identifier: GPL-2.0
/*
 * DRM driver for MIPI DBI compatible display panels
 *
 * Copyright 2022 Noralf Trønnes
 */

#include <clock.h>
#include <common.h>
#include <fb.h>
#include <firmware.h>
#include <linux/gpio/consumer.h>
#include <linux/printk.h>
#include <of.h>
#include <regulator.h>
#include <spi/spi.h>

#include <video/backlight.h>
#include <video/mipi_dbi.h>
#include <video/mipi_display.h>

static const u8 panel_mipi_dbi_magic[15] = { 'M', 'I', 'P', 'I', ' ', 'D', 'B', 'I',
					     0, 0, 0, 0, 0, 0, 0 };

/*
 * The display controller configuration is stored in a firmware file.
 * The Device Tree 'compatible' property value with a '.bin' suffix is passed
 * to request_firmware() to fetch this file.
 */
struct panel_mipi_dbi_config {
	/* Magic string: panel_mipi_dbi_magic */
	u8 magic[15];

	/* Config file format version */
	u8 file_format_version;

	/*
	 * MIPI commands to execute when the display pipeline is enabled.
	 * This is used to configure the display controller.
	 *
	 * The commands are stored in a byte array with the format:
	 *     command, num_parameters, [ parameter, ...], command, ...
	 *
	 * Some commands require a pause before the next command can be received.
	 * Inserting a delay in the command sequence is done by using the NOP command with one
	 * parameter: delay in miliseconds (the No Operation command is part of the MIPI Display
	 * Command Set where it has no parameters).
	 *
	 * Example:
	 *     command 0x11
	 *     sleep 120ms
	 *     command 0xb1 parameters 0x01, 0x2c, 0x2d
	 *     command 0x29
	 *
	 * Byte sequence:
	 *     0x11 0x00
	 *     0x00 0x01 0x78
	 *     0xb1 0x03 0x01 0x2c 0x2d
	 *     0x29 0x00
	 */
	u8 commands[];
};

struct panel_mipi_dbi_commands {
	const u8 *buf;
	size_t len;
};

static struct panel_mipi_dbi_commands *
panel_mipi_dbi_check_commands(struct device *dev, const struct firmware *fw)
{
	const struct panel_mipi_dbi_config *config = (struct panel_mipi_dbi_config *)fw->data;
	struct panel_mipi_dbi_commands *commands;
	size_t size = fw->size, commands_len;
	unsigned int i = 0;

	if (size < sizeof(*config) + 2) { /* At least 1 command */
		dev_err(dev, "config: file size=%zu is too small\n", size);
		return ERR_PTR(-EINVAL);
	}

	if (memcmp(config->magic, panel_mipi_dbi_magic, sizeof(config->magic))) {
		dev_err(dev, "config: Bad magic: %15ph\n", config->magic);
		return ERR_PTR(-EINVAL);
	}

	if (config->file_format_version != 1) {
		dev_err(dev, "config: version=%u is not supported\n", config->file_format_version);
		return ERR_PTR(-EINVAL);
	}

	dev_dbg(dev, "size=%zu version=%u\n", size, config->file_format_version);

	commands_len = size - sizeof(*config);

	while ((i + 1) < commands_len) {
		u8 command = config->commands[i++];
		u8 num_parameters = config->commands[i++];
		const u8 *parameters = &config->commands[i];

		i += num_parameters;
		if (i > commands_len) {
			dev_err(dev, "config: command=0x%02x num_parameters=%u overflows\n",
				command, num_parameters);
			return ERR_PTR(-EINVAL);
		}

		if (command == 0x00 && num_parameters == 1)
			dev_dbg(dev, "sleep %ums\n", parameters[0]);
		else
			dev_dbg(dev, "command %02x %*ph\n",
				command, num_parameters, parameters);
	}

	if (i != commands_len) {
		dev_err(dev, "config: malformed command array\n");
		return ERR_PTR(-EINVAL);
	}

	commands = kzalloc(sizeof(*commands), GFP_KERNEL);
	if (!commands)
		return ERR_PTR(-ENOMEM);

	commands->len = commands_len;
	commands->buf = kmemdup(config->commands, commands->len, GFP_KERNEL);
	if (!commands->buf)
		return ERR_PTR(-ENOMEM);

	return commands;
}

static struct panel_mipi_dbi_commands *panel_mipi_dbi_commands_from_fw(struct device *dev)
{
	struct panel_mipi_dbi_commands *commands;
	const struct firmware *fw;
	const char *compatible;
	char fw_name[40];
	int ret;

	ret = of_property_read_string_index(dev->of_node, "compatible", 0, &compatible);
	if (ret)
		return ERR_PTR(ret);

	snprintf(fw_name, sizeof(fw_name), "%s.bin", compatible);
	ret = request_firmware(&fw, fw_name, dev);
	if (ret) {
		dev_err(dev, "No config file found for compatible '%s' (error=%d)\n",
			compatible, ret);

		return ERR_PTR(ret);
	}

	commands = panel_mipi_dbi_check_commands(dev, fw);
	release_firmware(fw);

	return commands;
}

static void panel_mipi_dbi_commands_execute(struct mipi_dbi *dbi,
					    struct panel_mipi_dbi_commands *commands)
{
	unsigned int i = 0;

	if (!commands)
		return;

	while (i < commands->len) {
		u8 command = commands->buf[i++];
		u8 num_parameters = commands->buf[i++];
		const u8 *parameters = &commands->buf[i];

		if (command == 0x00 && num_parameters == 1)
			mdelay(parameters[0]);
		else if (num_parameters)
			mipi_dbi_command_stackbuf(dbi, command, parameters, num_parameters);
		else
			mipi_dbi_command(dbi, command);

		i += num_parameters;
	}
}

static void panel_mipi_dbi_enable(struct fb_info *info)
{
	struct mipi_dbi_dev *dbidev = container_of(info, struct mipi_dbi_dev, info);
	struct mipi_dbi *dbi = &dbidev->dbi;
	int ret;

	if (!info->mode) {
		dev_err(dbidev->dev, "No valid mode found\n");
		return;
	}

	if (dbidev->backlight_node && !dbidev->backlight) {
		dbidev->backlight = of_backlight_find(dbidev->backlight_node);
		if (!dbidev->backlight)
			dev_err(dbidev->dev, "No backlight found\n");
	}

	if (!dbidev->driver_private) {
		dbidev->driver_private = panel_mipi_dbi_commands_from_fw(dbidev->dev);
		if (IS_ERR(dbidev->driver_private)) {
			dbidev->driver_private = NULL;
			return;
		}
	}

	ret = mipi_dbi_poweron_conditional_reset(dbidev);
	if (ret < 0)
		return;
	if (!ret)
		panel_mipi_dbi_commands_execute(dbi, dbidev->driver_private);

	mipi_dbi_enable_flush(dbidev, info);
}


static struct fb_ops panel_mipi_dbi_ops = {
	.fb_enable = panel_mipi_dbi_enable,
	.fb_disable = mipi_dbi_fb_disable,
	.fb_damage = mipi_dbi_fb_damage,
	.fb_flush = mipi_dbi_fb_flush,
};


static int panel_mipi_dbi_get_mode(struct mipi_dbi_dev *dbidev, struct fb_videomode *mode)
{
	struct device *dev = dbidev->dev;
	int ret;

	ret = of_get_display_timing(dev->of_node, "panel-timing", mode);
	if (ret) {
		dev_err(dev, "%pOF: failed to get panel-timing (error=%d)\n", dev->of_node, ret);
		return ret;
	}

	/*
	 * Make sure width and height are set and that only back porch and
	 * pixelclock are set in the other timing values. Also check that
	 * width and height don't exceed the 16-bit value specified by MIPI DCS.
	 */
	if (!mode->xres || !mode->yres || mode->display_flags ||
	    mode->right_margin || mode->hsync_len || (mode->left_margin + mode->xres) > 0xffff ||
	    mode->lower_margin || mode->vsync_len || (mode->upper_margin + mode->yres) > 0xffff) {
		dev_err(dev, "%pOF: panel-timing out of bounds\n", dev->of_node);
		return -EINVAL;
	}

	/* The driver doesn't use the pixel clock but it is mandatory so fake one if not set */
	if (!mode->pixclock) {
		mode->pixclock =
			(mode->left_margin + mode->xres + mode->right_margin + mode->hsync_len) *
			(mode->upper_margin + mode->yres + mode->lower_margin + mode->vsync_len) *
			60 / 1000;
	}

	return 0;
}

static int panel_mipi_dbi_spi_probe(struct device *dev)
{
	struct mipi_dbi_dev *dbidev;
	struct spi_device *spi = to_spi_device(dev);
	struct mipi_dbi *dbi;
	struct fb_info *info;
	struct gpio_desc *dc;
	int ret;

	dbidev = kzalloc(sizeof(*dbidev), GFP_KERNEL);
	if (!dbidev)
		return -ENOMEM;

	dbidev->dev = dev;
	dbi = &dbidev->dbi;
	info = &dbidev->info;

	ret = panel_mipi_dbi_get_mode(dbidev, &dbidev->mode);
	if (ret)
		return ret;

	dbidev->regulator = regulator_get(dev, "power");
	if (IS_ERR(dbidev->regulator))
		return dev_err_probe(dev, PTR_ERR(dbidev->regulator),
				     "Failed to get regulator 'power'\n");

	dbidev->io_regulator = regulator_get(dev, "io");
	if (IS_ERR(dbidev->io_regulator))
		return dev_err_probe(dev, PTR_ERR(dbidev->io_regulator),
				     "Failed to get regulator 'io'\n");

	dbidev->backlight_node = of_parse_phandle(dev->of_node, "backlight", 0);

	dbi->reset = gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(dbi->reset))
		return dev_errp_probe(dev, dbi->reset,
				     "Failed to get GPIO 'reset'\n");

	dc = gpiod_get_optional(dev, "dc", GPIOD_OUT_LOW);
	if (IS_ERR(dc))
		return dev_errp_probe(dev, dc, "Failed to get GPIO 'dc'\n");

	ret = mipi_dbi_spi_init(spi, dbi, dc);
	if (ret)
		return ret;

	ret = mipi_dbi_dev_init(dbidev, &panel_mipi_dbi_ops, &dbidev->mode);
	if (ret)
		return ret;

	ret = register_framebuffer(info);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to register framebuffer\n");

	return 0;
}

static const struct of_device_id panel_mipi_dbi_spi_of_match[] = {
	{ .compatible = "panel-mipi-dbi-spi" },
	{},
};
MODULE_DEVICE_TABLE(of, panel_mipi_dbi_spi_of_match);

static struct driver panel_mipi_dbi_spi_driver = {
	.name = "panel-mipi-dbi-spi",
	.probe = panel_mipi_dbi_spi_probe,
	.of_compatible = DRV_OF_COMPAT(panel_mipi_dbi_spi_of_match),
};
device_spi_driver(panel_mipi_dbi_spi_driver);

MODULE_DESCRIPTION("MIPI DBI compatible display panel driver");
MODULE_AUTHOR("Noralf Trønnes");
MODULE_LICENSE("GPL");
