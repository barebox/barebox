/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * MIPI Display Bus Interface (DBI) LCD controller support
 *
 * Copyright 2016 Noralf Trønnes
 */

#ifndef __LINUX_MIPI_DBI_H
#define __LINUX_MIPI_DBI_H

#include <linux/types.h>
#include <spi/spi.h>
#include <driver.h>
#include <fb.h>

struct regulator;
struct fb_videomode;
struct gpio_desc;

/**
 * struct mipi_dbi - MIPI DBI interface
 */
struct mipi_dbi {
	/**
	 * @command: Bus specific callback executing commands.
	 */
	int (*command)(struct mipi_dbi *dbi, u8 *cmd, u8 *param, size_t num);

	/**
	 * @read_commands: Array of read commands terminated by a zero entry.
	 *                 Reading is disabled if this is NULL.
	 */
	const u8 *read_commands;

	/**
	 * @swap_bytes: Swap bytes in buffer before transfer
	 */
	bool swap_bytes;

	/**
	 * @reset: Optional reset gpio
	 */
	struct gpio_desc *reset;

	/* Type C specific */

	/**
	 * @spi: SPI device
	 */
	struct spi_device *spi;

	/**
	 * @dc: Optional D/C gpio.
	 */
	struct gpio_desc *dc;

	struct list_head list;
};

/**
 * struct mipi_dbi_dev - MIPI DBI device
 */
struct mipi_dbi_dev {
	/**
	 * @dev: Device
	 */
	struct device *dev;

	/**
	 * @info: Framebuffer info
	 */
	struct fb_info info;

	/**
	 * @mode: Fixed display mode
	 */
	struct fb_videomode mode;

	/**
	 * @tx_buf: Buffer used for transfer (copy clip rect area)
	 */
	u8 *tx_buf;

	/**
	 * @backlight_node: backlight device node (optional)
	 */
	struct device_node *backlight_node;

	/**
	 * @backlight: backlight device (optional)
	 */
	struct backlight_device *backlight;

	/**
	 * @regulator: power regulator (Vdd) (optional)
	 */
	struct regulator *regulator;

	/**
	 * @io_regulator: I/O power regulator (Vddi) (optional)
	 */
	struct regulator *io_regulator;

	/**
	 * @dbi: MIPI DBI interface
	 */
	struct mipi_dbi dbi;

	/**
	 * @driver_private: Driver private data.
	 */
	void *driver_private;

	/**
	 * @damage: Damage rectangle.
	 */
	struct fb_rect damage;
};

static inline const char *mipi_dbi_name(struct mipi_dbi *dbi)
{
	return dev_name(&dbi->spi->dev);
}

int mipi_dbi_spi_init(struct spi_device *spi, struct mipi_dbi *dbi,
		      struct gpio_desc *dc);
int mipi_dbi_dev_init(struct mipi_dbi_dev *dbidev,
		      struct fb_ops *ops, struct fb_videomode *mode);
void mipi_dbi_fb_damage(struct fb_info *info, const struct fb_rect *rect);
void mipi_dbi_fb_flush(struct fb_info *info);
void mipi_dbi_enable_flush(struct mipi_dbi_dev *dbidev,
			   struct fb_info *info);
void mipi_dbi_fb_disable(struct fb_info *info);
void mipi_dbi_hw_reset(struct mipi_dbi *dbi);
bool mipi_dbi_display_is_on(struct mipi_dbi *dbi);
int mipi_dbi_poweron_conditional_reset(struct mipi_dbi_dev *dbidev);

u32 mipi_dbi_spi_cmd_max_speed(struct spi_device *spi, size_t len);
int mipi_dbi_spi_transfer(struct spi_device *spi, u32 speed_hz,
			  u8 bpw, const void *buf, size_t len);

int mipi_dbi_command_read(struct mipi_dbi *dbi, u8 cmd, u8 *val);
int mipi_dbi_command_buf(struct mipi_dbi *dbi, u8 cmd, u8 *data, size_t len);
int mipi_dbi_command_stackbuf(struct mipi_dbi *dbi, u8 cmd, const u8 *data,
			      size_t len);

/**
 * mipi_dbi_command - MIPI DCS command with optional parameter(s)
 * @dbi: MIPI DBI structure
 * @cmd: Command
 * @seq: Optional parameter(s)
 *
 * Send MIPI DCS command to the controller. Use mipi_dbi_command_read() for
 * get/read.
 *
 * Returns:
 * Zero on success, negative error code on failure.
 */
#define mipi_dbi_command(dbi, cmd, seq...) \
({ \
	const u8 d[] = { seq }; \
	struct device *dev = &(dbi)->spi->dev;	\
	int ret; \
	ret = mipi_dbi_command_stackbuf(dbi, cmd, d, ARRAY_SIZE(d)); \
	if (ret) \
		dev_err(dev, "error %pe when sending command %#02x\n", ERR_PTR(ret), cmd); \
	ret; \
})

bool mipi_dbi_command_is_read(struct mipi_dbi *dbi, u8 cmd);
int mipi_dbi_command_read_len(int cmd);

extern struct list_head mipi_dbi_list;

#endif /* __LINUX_MIPI_DBI_H */
