/* SPDX-License-Identifier: GPL-2.0 */
/*
 * MIPI DSI Bus
 *
 * Copyright (C) 2012-2013, Samsung Electronics, Co., Ltd.
 * Copyright (C) 2018 STMicroelectronics - All Rights Reserved
 * Author(s): Andrzej Hajda <a.hajda@samsung.com>
 *            Yannick Fertre <yannick.fertre@st.com>
 *            Philippe Cornu <philippe.cornu@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef MIPI_DSI_H
#define MIPI_DSI_H

#include <video/mipi_display.h>
#include <linux/container_of.h>
#include <linux/array_size.h>
#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <module.h>

struct mipi_dsi_host;
struct mipi_dsi_device;
struct drm_display_mode;

/* request ACK from peripheral */
#define MIPI_DSI_MSG_REQ_ACK	BIT(0)
/* use Low Power Mode to transmit message */
#define MIPI_DSI_MSG_USE_LPM	BIT(1)

/**
 * struct mipi_dsi_msg - read/write DSI buffer
 * @channel: virtual channel id
 * @type: payload data type
 * @flags: flags controlling this message transmission
 * @tx_len: length of @tx_buf
 * @tx_buf: data to be written
 * @rx_len: length of @rx_buf
 * @rx_buf: data to be read, or NULL
 */
struct mipi_dsi_msg {
	u8 channel;
	u8 type;
	u16 flags;

	size_t tx_len;
	const void *tx_buf;

	size_t rx_len;
	void *rx_buf;
};

bool mipi_dsi_packet_format_is_short(u8 type);
bool mipi_dsi_packet_format_is_long(u8 type);

/**
 * struct mipi_dsi_packet - represents a MIPI DSI packet in protocol format
 * @size: size (in bytes) of the packet
 * @header: the four bytes that make up the header (Data ID, Word Count or
 *     Packet Data, and ECC)
 * @payload_length: number of bytes in the payload
 * @payload: a pointer to a buffer containing the payload, if any
 */
struct mipi_dsi_packet {
	size_t size;
	u8 header[4];
	size_t payload_length;
	const u8 *payload;
};

int mipi_dsi_create_packet(struct mipi_dsi_packet *packet,
			   const struct mipi_dsi_msg *msg);

/**
 * struct mipi_dsi_host_ops - DSI bus operations
 * @attach: attach DSI device to DSI host
 * @detach: detach DSI device from DSI host
 * @transfer: transmit a DSI packet
 *
 * DSI packets transmitted by .transfer() are passed in as mipi_dsi_msg
 * structures. This structure contains information about the type of packet
 * being transmitted as well as the transmit and receive buffers. When an
 * error is encountered during transmission, this function will return a
 * negative error code. On success it shall return the number of bytes
 * transmitted for write packets or the number of bytes received for read
 * packets.
 *
 * Note that typically DSI packet transmission is atomic, so the .transfer()
 * function will seldomly return anything other than the number of bytes
 * contained in the transmit buffer on success.
 */
struct mipi_dsi_host_ops {
	int (*attach)(struct mipi_dsi_host *host,
		      struct mipi_dsi_device *dsi);
	int (*detach)(struct mipi_dsi_host *host,
		      struct mipi_dsi_device *dsi);
	ssize_t (*transfer)(struct mipi_dsi_host *host,
			    const struct mipi_dsi_msg *msg);
};

/**
 * struct mipi_dsi_host - DSI host device
 * @dev: driver model device node for this DSI host
 * @ops: DSI host operations
 * @list: list management
 */
struct mipi_dsi_host {
	struct device *dev;
	const struct mipi_dsi_host_ops *ops;
	struct list_head list;
};

int mipi_dsi_host_register(struct mipi_dsi_host *host);
struct mipi_dsi_host *of_find_mipi_dsi_host_by_node(struct device_node *node);

/* DSI mode flags */

/* video mode */
#define MIPI_DSI_MODE_VIDEO		BIT(0)
/* video burst mode */
#define MIPI_DSI_MODE_VIDEO_BURST	BIT(1)
/* video pulse mode */
#define MIPI_DSI_MODE_VIDEO_SYNC_PULSE	BIT(2)
/* enable auto vertical count mode */
#define MIPI_DSI_MODE_VIDEO_AUTO_VERT	BIT(3)
/* enable hsync-end packets in vsync-pulse and v-porch area */
#define MIPI_DSI_MODE_VIDEO_HSE		BIT(4)
/* disable hfront-porch area */
#define MIPI_DSI_MODE_VIDEO_HFP		BIT(5)
/* disable hback-porch area */
#define MIPI_DSI_MODE_VIDEO_HBP		BIT(6)
/* disable hsync-active area */
#define MIPI_DSI_MODE_VIDEO_HSA		BIT(7)
/* flush display FIFO on vsync pulse */
#define MIPI_DSI_MODE_VSYNC_FLUSH	BIT(8)
/* disable EoT packets in HS mode */
#define MIPI_DSI_MODE_NO_EOT_PACKET	BIT(9)
/* device supports non-continuous clock behavior (DSI spec 5.6.1) */
#define MIPI_DSI_CLOCK_NON_CONTINUOUS	BIT(10)
/* transmit data in low power */
#define MIPI_DSI_MODE_LPM		BIT(11)

enum mipi_dsi_pixel_format {
	MIPI_DSI_FMT_RGB888,
	MIPI_DSI_FMT_RGB666,
	MIPI_DSI_FMT_RGB666_PACKED,
	MIPI_DSI_FMT_RGB565,
};

#define DSI_DEV_NAME_SIZE		20

/**
 * struct mipi_dsi_device_info - template for creating a mipi_dsi_device
 * @type: DSI peripheral chip type
 * @channel: DSI virtual channel assigned to peripheral
 * @node: pointer to OF device node or NULL
 *
 * This is populated and passed to mipi_dsi_device_new to create a new
 * DSI device
 */
struct mipi_dsi_device_info {
	char type[DSI_DEV_NAME_SIZE];
	u32 channel;
	struct device_node *node;
};

/**
 * struct mipi_dsi_device - DSI peripheral device
 * @host: DSI host for this peripheral
 * @dev: driver model device node for this peripheral
 * @attached: the DSI device has been successfully attached
 * @name: DSI peripheral chip type
 * @channel: virtual channel assigned to the peripheral
 * @format: pixel format for video mode
 * @lanes: number of active data lanes
 * @mode_flags: DSI operation mode related flags
 */
struct mipi_dsi_device {
	struct mipi_dsi_host *host;
	struct device dev;
	bool attached;

	char name[DSI_DEV_NAME_SIZE];
	unsigned int channel;
	unsigned int lanes;
	enum mipi_dsi_pixel_format format;
	unsigned long mode_flags;
};

/**
 * struct mipi_dsi_multi_context - Context to call multiple MIPI DSI funcs in a row
 */
struct mipi_dsi_multi_context {
	/**
	 * @dsi: Pointer to the MIPI DSI device
	 */
	struct mipi_dsi_device *dsi;

	/**
	 * @accum_err: Storage for the accumulated error over the multiple calls
	 *
	 * Init to 0. If a function encounters an error then the error code
	 * will be stored here. If you call a function and this points to a
	 * non-zero value then the function will be a noop. This allows calling
	 * a function many times in a row and just checking the error at the
	 * end to see if any of them failed.
	 */
	int accum_err;
};

#define to_mipi_dsi_device(__dev)	container_of_const(__dev, struct mipi_dsi_device, dev)

/**
 * mipi_dsi_pixel_format_to_bpp - obtain the number of bits per pixel for any
 *                                given pixel format defined by the MIPI DSI
 *                                specification
 * @fmt: MIPI DSI pixel format
 *
 * Returns: The number of bits per pixel of the given pixel format.
 */
static inline int mipi_dsi_pixel_format_to_bpp(enum mipi_dsi_pixel_format fmt)
{
	switch (fmt) {
	case MIPI_DSI_FMT_RGB888:
	case MIPI_DSI_FMT_RGB666:
		return 24;

	case MIPI_DSI_FMT_RGB666_PACKED:
		return 18;

	case MIPI_DSI_FMT_RGB565:
		return 16;
	}

	return -EINVAL;
}

enum mipi_dsi_compression_algo {
	MIPI_DSI_COMPRESSION_DSC = 0,
	MIPI_DSI_COMPRESSION_VENDOR = 3,
	/* other two values are reserved, DSI 1.3 */
};

struct mipi_dsi_device *
mipi_dsi_device_register_full(struct mipi_dsi_host *host,
			      const struct mipi_dsi_device_info *info);
void mipi_dsi_device_unregister(struct mipi_dsi_device *dsi);
struct mipi_dsi_device *of_find_mipi_dsi_device_by_node(struct device_node *np);

/**
 * mipi_dsi_attach - attach a DSI device to its DSI host
 * @dsi: DSI peripheral
 */
int mipi_dsi_attach(struct mipi_dsi_device *dsi);

/**
 * mipi_dsi_detach - detach a DSI device from its DSI host
 * @dsi: DSI peripheral
 */
int mipi_dsi_detach(struct mipi_dsi_device *dsi);
int mipi_dsi_shutdown_peripheral(struct mipi_dsi_device *dsi);
int mipi_dsi_turn_on_peripheral(struct mipi_dsi_device *dsi);
int mipi_dsi_set_maximum_return_packet_size(struct mipi_dsi_device *dsi,
					    u16 value);

int mipi_dsi_compression_mode(struct mipi_dsi_device *dsi, bool enable);
int mipi_dsi_compression_mode_ext(struct mipi_dsi_device *dsi, bool enable,
				  enum mipi_dsi_compression_algo algo,
				  unsigned int pps_selector);

void mipi_dsi_compression_mode_ext_multi(struct mipi_dsi_multi_context *ctx,
					 bool enable,
					 enum mipi_dsi_compression_algo algo,
					 unsigned int pps_selector);
void mipi_dsi_compression_mode_multi(struct mipi_dsi_multi_context *ctx,
				     bool enable);

ssize_t mipi_dsi_generic_write(struct mipi_dsi_device *dsi, const void *payload,
			       size_t size);
int mipi_dsi_generic_write_chatty(struct mipi_dsi_device *dsi,
				  const void *payload, size_t size);
void mipi_dsi_generic_write_multi(struct mipi_dsi_multi_context *ctx,
				  const void *payload, size_t size);
ssize_t mipi_dsi_generic_read(struct mipi_dsi_device *dsi, const void *params,
			      size_t num_params, void *data, size_t size);
#define mipi_dsi_mdelay(ctx, delay)	\
	do {				\
		if (!(ctx)->accum_err)	\
			mdelay(delay);	\
	} while (0)

#define mipi_dsi_udelay_range(ctx, min, max)	\
	do {					\
		(void)(max);			\
		if (!(ctx)->accum_err)		\
			udelay(min);		\
	} while (0)



/**
 * enum mipi_dsi_dcs_tear_mode - Tearing Effect Output Line mode
 * @MIPI_DSI_DCS_TEAR_MODE_VBLANK: the TE output line consists of V-Blanking
 *    information only
 * @MIPI_DSI_DCS_TEAR_MODE_VHBLANK : the TE output line consists of both
 *    V-Blanking and H-Blanking information
 */
enum mipi_dsi_dcs_tear_mode {
	MIPI_DSI_DCS_TEAR_MODE_VBLANK,
	MIPI_DSI_DCS_TEAR_MODE_VHBLANK,
};

#define MIPI_DSI_DCS_POWER_MODE_DISPLAY BIT(2)
#define MIPI_DSI_DCS_POWER_MODE_NORMAL  BIT(3)
#define MIPI_DSI_DCS_POWER_MODE_SLEEP   BIT(4)
#define MIPI_DSI_DCS_POWER_MODE_PARTIAL BIT(5)
#define MIPI_DSI_DCS_POWER_MODE_IDLE    BIT(6)

/**
 * mipi_dsi_dcs_write_buffer() - transmit a DCS command with payload
 * @dsi: DSI peripheral device
 * @data: buffer containing data to be transmitted
 * @len: size of transmission buffer
 *
 * This function will automatically choose the right data type depending on
 * the command payload length.
 *
 * Return: The number of bytes successfully transmitted or a negative error
 * code on failure.
 */
ssize_t mipi_dsi_dcs_write_buffer(struct mipi_dsi_device *dsi,
				  const void *data, size_t len);

int mipi_dsi_dcs_write_buffer_chatty(struct mipi_dsi_device *dsi,
				     const void *data, size_t len);
void mipi_dsi_dcs_write_buffer_multi(struct mipi_dsi_multi_context *ctx,
				     const void *data, size_t len);

/**
 * mipi_dsi_dcs_write() - send DCS write command
 * @dsi: DSI peripheral device
 * @cmd: DCS command
 * @data: buffer containing the command payload
 * @len: command payload length
 *
 * This function will automatically choose the right data type depending on
 * the command payload length.

 * code on failure.
 */
ssize_t mipi_dsi_dcs_write(struct mipi_dsi_device *dsi, u8 cmd,
			   const void *data, size_t len);

/**
 * mipi_dsi_dcs_read() - send DCS read request command
 * @dsi: DSI peripheral device
 * @cmd: DCS command
 * @data: buffer in which to receive data
 * @len: size of receive buffer
 *
 * Return: The number of bytes read or a negative error code on failure.
 */
ssize_t mipi_dsi_dcs_read(struct mipi_dsi_device *dsi, u8 cmd, void *data,
			  size_t len);

/**
 * mipi_dsi_dcs_nop() - send DCS nop packet
 * @dsi: DSI peripheral device
 *
 * Return: 0 on success or a negative error code on failure.
 */
int mipi_dsi_dcs_nop(struct mipi_dsi_device *dsi);

/**
 * mipi_dsi_dcs_soft_reset() - perform a software reset of the display module
 * @dsi: DSI peripheral device
 *
 * Return: 0 on success or a negative error code on failure.
 */
int mipi_dsi_dcs_soft_reset(struct mipi_dsi_device *dsi);

/**
 * mipi_dsi_dcs_get_power_mode() - query the display module's current power
 *    mode
 * @dsi: DSI peripheral device
 * @mode: return location for the current power mode
 *
 * Return: 0 on success or a negative error code on failure.
 */
int mipi_dsi_dcs_get_power_mode(struct mipi_dsi_device *dsi, u8 *mode);

/**
 * mipi_dsi_dcs_get_pixel_format() - gets the pixel format for the RGB image
 *    data used by the interface
 * @dsi: DSI peripheral device
 * @format: return location for the pixel format
 *
 * Return: 0 on success or a negative error code on failure.
 */
int mipi_dsi_dcs_get_pixel_format(struct mipi_dsi_device *dsi, u8 *format);

/**
 * mipi_dsi_dcs_enter_sleep_mode() - disable all unnecessary blocks inside the
 *    display module except interface communication
 * @dsi: DSI peripheral device
 *
 * Return: 0 on success or a negative error code on failure.
 */
int mipi_dsi_dcs_enter_sleep_mode(struct mipi_dsi_device *dsi);

/**
 * mipi_dsi_dcs_exit_sleep_mode() - enable all blocks inside the display
 *    module
 * @dsi: DSI peripheral device
 *
 * Return: 0 on success or a negative error code on failure.
 */
int mipi_dsi_dcs_exit_sleep_mode(struct mipi_dsi_device *dsi);

/**
 * mipi_dsi_dcs_set_display_off() - stop displaying the image data on the
 *    display device
 * @dsi: DSI peripheral device
 *
 * Return: 0 on success or a negative error code on failure.
 */
int mipi_dsi_dcs_set_display_off(struct mipi_dsi_device *dsi);

/**
 * mipi_dsi_dcs_set_display_on() - start displaying the image data on the
 *    display device
 * @dsi: DSI peripheral device
 *
 * Return: 0 on success or a negative error code on failure
 */
int mipi_dsi_dcs_set_display_on(struct mipi_dsi_device *dsi);

/**
 * mipi_dsi_dcs_set_column_address() - define the column extent of the frame
 *    memory accessed by the host processor
 * @dsi: DSI peripheral device
 * @start: first column of frame memory
 * @end: last column of frame memory
 *
 * Return: 0 on success or a negative error code on failure.
 */
int mipi_dsi_dcs_set_column_address(struct mipi_dsi_device *dsi, u16 start,
				    u16 end);
/**
 * mipi_dsi_dcs_set_page_address() - define the page extent of the frame
 *    memory accessed by the host processor
 * @dsi: DSI peripheral device
 * @start: first page of frame memory
 * @end: last page of frame memory
 *
 * Return: 0 on success or a negative error code on failure.
 */
int mipi_dsi_dcs_set_page_address(struct mipi_dsi_device *dsi, u16 start,
				  u16 end);

/**
 * mipi_dsi_dcs_set_tear_off() - turn off the display module's Tearing Effect
 *    output signal on the TE signal line
 * @dsi: DSI peripheral device
 *
 * Return: 0 on success or a negative error code on failure
 */
int mipi_dsi_dcs_set_tear_off(struct mipi_dsi_device *dsi);

/**
 * mipi_dsi_dcs_set_tear_on() - turn on the display module's Tearing Effect
 *    output signal on the TE signal line.
 * @dsi: DSI peripheral device
 * @mode: the Tearing Effect Output Line mode
 *
 * Return: 0 on success or a negative error code on failure
 */
int mipi_dsi_dcs_set_tear_on(struct mipi_dsi_device *dsi,
			     enum mipi_dsi_dcs_tear_mode mode);

/**
 * mipi_dsi_dcs_set_pixel_format() - sets the pixel format for the RGB image
 *    data used by the interface
 * @dsi: DSI peripheral device
 * @format: pixel format
 *
 * Return: 0 on success or a negative error code on failure.
 */
int mipi_dsi_dcs_set_pixel_format(struct mipi_dsi_device *dsi, u8 format);

/**
 * mipi_dsi_dcs_set_tear_scanline() - set the scanline to use as trigger for
 *    the Tearing Effect output signal of the display module
 * @dsi: DSI peripheral device
 * @scanline: scanline to use as trigger
 *
 * Return: 0 on success or a negative error code on failure
 */
int mipi_dsi_dcs_set_tear_scanline(struct mipi_dsi_device *dsi, u16 scanline);

/**
 * mipi_dsi_dcs_set_display_brightness() - sets the brightness value of the
 *    display
 * @dsi: DSI peripheral device
 * @brightness: brightness value
 *
 * Return: 0 on success or a negative error code on failure.
 */
int mipi_dsi_dcs_set_display_brightness(struct mipi_dsi_device *dsi,
					u16 brightness);

/**
 * mipi_dsi_dcs_get_display_brightness() - gets the current brightness value
 *    of the display
 * @dsi: DSI peripheral device
 * @brightness: brightness value
 *
 * Return: 0 on success or a negative error code on failure.
 */
int mipi_dsi_dcs_get_display_brightness(struct mipi_dsi_device *dsi,
					u16 *brightness);
int mipi_dsi_dcs_set_display_brightness_large(struct mipi_dsi_device *dsi,
					     u16 brightness);
int mipi_dsi_dcs_get_display_brightness_large(struct mipi_dsi_device *dsi,
					     u16 *brightness);

void mipi_dsi_dcs_nop_multi(struct mipi_dsi_multi_context *ctx);
void mipi_dsi_dcs_enter_sleep_mode_multi(struct mipi_dsi_multi_context *ctx);
void mipi_dsi_dcs_exit_sleep_mode_multi(struct mipi_dsi_multi_context *ctx);
void mipi_dsi_dcs_set_display_off_multi(struct mipi_dsi_multi_context *ctx);
void mipi_dsi_dcs_set_display_on_multi(struct mipi_dsi_multi_context *ctx);
void mipi_dsi_dcs_set_tear_on_multi(struct mipi_dsi_multi_context *ctx,
				    enum mipi_dsi_dcs_tear_mode mode);
void mipi_dsi_turn_on_peripheral_multi(struct mipi_dsi_multi_context *ctx);
void mipi_dsi_dcs_soft_reset_multi(struct mipi_dsi_multi_context *ctx);
void mipi_dsi_dcs_set_display_brightness_multi(struct mipi_dsi_multi_context *ctx,
					       u16 brightness);
void mipi_dsi_dcs_set_pixel_format_multi(struct mipi_dsi_multi_context *ctx,
					 u8 format);
void mipi_dsi_dcs_set_column_address_multi(struct mipi_dsi_multi_context *ctx,
					   u16 start, u16 end);
void mipi_dsi_dcs_set_page_address_multi(struct mipi_dsi_multi_context *ctx,
					 u16 start, u16 end);
void mipi_dsi_dcs_set_tear_scanline_multi(struct mipi_dsi_multi_context *ctx,
					  u16 scanline);
void mipi_dsi_dcs_set_tear_off_multi(struct mipi_dsi_multi_context *ctx);

/**
 * mipi_dsi_generic_write_seq_multi - transmit data using a generic write packet
 *
 * This macro will print errors for you and error handling is optimized for
 * callers that call this multiple times in a row.
 *
 * @ctx: Context for multiple DSI transactions
 * @seq: buffer containing the payload
 */
#define mipi_dsi_generic_write_seq_multi(ctx, seq...)                \
	do {                                                         \
		static const u8 d[] = { seq };                       \
		mipi_dsi_generic_write_multi(ctx, d, ARRAY_SIZE(d)); \
	} while (0)

/**
 * mipi_dsi_dcs_write_seq_multi - transmit a DCS command with payload
 *
 * This macro will print errors for you and error handling is optimized for
 * callers that call this multiple times in a row.
 *
 * @ctx: Context for multiple DSI transactions
 * @cmd: Command
 * @seq: buffer containing data to be transmitted
 */
#define mipi_dsi_dcs_write_seq_multi(ctx, cmd, seq...)                  \
	do {                                                            \
		static const u8 d[] = { cmd, seq };                     \
		mipi_dsi_dcs_write_buffer_multi(ctx, d, ARRAY_SIZE(d)); \
	} while (0)

/**
 * struct mipi_dsi_driver - DSI driver
 * @driver: device driver model driver
 * @probe: callback for device binding
 * @remove: callback for device unbinding
 */
struct mipi_dsi_driver {
	struct device_driver driver;
	int(*probe)(struct mipi_dsi_device *dsi);
	void (*remove)(struct mipi_dsi_device *dsi);
};

static inline struct mipi_dsi_driver *
to_mipi_dsi_driver(struct device_driver *driver)
{
	return container_of(driver, struct mipi_dsi_driver, driver);
}

static inline void *mipi_dsi_get_drvdata(const struct mipi_dsi_device *dsi)
{
	return dev_get_drvdata(&dsi->dev);
}

static inline void mipi_dsi_set_drvdata(struct mipi_dsi_device *dsi, void *data)
{
	dev_set_drvdata(&dsi->dev, data);
}

int mipi_dsi_driver_register(struct mipi_dsi_driver *driver);
void mipi_dsi_driver_unregister(struct mipi_dsi_driver *driver);

#define module_mipi_dsi_driver(__mipi_dsi_driver) \
	module_driver(__mipi_dsi_driver, mipi_dsi_driver_register, \
			mipi_dsi_driver_unregister)

extern struct bus_type mipi_dsi_bus_type;

#endif /* MIPI_DSI_H */
