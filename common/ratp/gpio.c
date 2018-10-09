/*
 * Copyright (c) 2018 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 * Copyright (c) 2018 Zodiac Inflight Innovations
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) "barebox-ratp: gpio: " fmt

#include <common.h>
#include <ratp_bb.h>
#include <malloc.h>
#include <environment.h>
#include <gpio.h>
#include <errno.h>

struct ratp_bb_gpio_get_value_request {
	struct ratp_bb header;
	uint32_t       gpio;
} __packed;

struct ratp_bb_gpio_get_value_response {
	struct ratp_bb header;
	uint8_t        value;
} __packed;

static int ratp_cmd_gpio_get_value(const struct ratp_bb *req, int req_len,
				   struct ratp_bb **rsp, int *rsp_len)
{
	struct ratp_bb_gpio_get_value_request *gpio_req = (struct ratp_bb_gpio_get_value_request *)req;
	struct ratp_bb_gpio_get_value_response *gpio_rsp;
	int gpio_rsp_len;

	if (req_len < sizeof(*gpio_req)) {
		pr_err("get value request ignored: size mismatch (%d < %zu)\n", req_len, sizeof(*gpio_req));
		return 2;
	}

	gpio_rsp_len = sizeof(struct ratp_bb_gpio_get_value_response);
	gpio_rsp = xzalloc(gpio_rsp_len);
	gpio_rsp->header.type = cpu_to_be16(BB_RATP_TYPE_GPIO_GET_VALUE_RETURN);
	gpio_rsp->value = !!gpio_get_value(be32_to_cpu(gpio_req->gpio));

	*rsp_len = gpio_rsp_len;
	*rsp = (struct ratp_bb *)gpio_rsp;
	return 0;
}

BAREBOX_RATP_CMD_START(GPIO_GET_VALUE)
	.request_id = BB_RATP_TYPE_GPIO_GET_VALUE,
	.response_id = BB_RATP_TYPE_GPIO_GET_VALUE_RETURN,
	.cmd = ratp_cmd_gpio_get_value
BAREBOX_RATP_CMD_END


struct ratp_bb_gpio_set_value_request {
	struct ratp_bb header;
	uint32_t       gpio;
	uint8_t        value;
} __packed;

static int ratp_cmd_gpio_set_value(const struct ratp_bb *req, int req_len,
				   struct ratp_bb **rsp, int *rsp_len)
{
	struct ratp_bb_gpio_set_value_request *gpio_req = (struct ratp_bb_gpio_set_value_request *)req;

	if (req_len < sizeof(*gpio_req)) {
		pr_err("set value request ignored: size mismatch (%d < %zu)\n", req_len, sizeof(*gpio_req));
		return 2;
	}

	gpio_set_value(be32_to_cpu(gpio_req->gpio), gpio_req->value);

	*rsp_len = sizeof(struct ratp_bb);
	*rsp = xzalloc(*rsp_len);
	(*rsp)->type = cpu_to_be16(BB_RATP_TYPE_GPIO_SET_VALUE_RETURN);
	return 0;
}

BAREBOX_RATP_CMD_START(GPIO_SET_VALUE)
	.request_id = BB_RATP_TYPE_GPIO_SET_VALUE,
	.response_id = BB_RATP_TYPE_GPIO_SET_VALUE_RETURN,
	.cmd = ratp_cmd_gpio_set_value
BAREBOX_RATP_CMD_END


struct ratp_bb_gpio_set_direction_request {
	struct ratp_bb header;
	uint32_t       gpio;
	uint8_t        direction; /* 0: input, 1: output */
	uint8_t        value;     /* applicable only if direction output */
} __packed;

struct ratp_bb_gpio_set_direction_response {
	struct ratp_bb header;
	uint32_t       errno;
} __packed;

static int ratp_cmd_gpio_set_direction(const struct ratp_bb *req, int req_len,
				       struct ratp_bb **rsp, int *rsp_len)
{
	struct ratp_bb_gpio_set_direction_request *gpio_req = (struct ratp_bb_gpio_set_direction_request *)req;
	struct ratp_bb_gpio_set_direction_response *gpio_rsp;
	int gpio_rsp_len;
	uint32_t gpio;
	int ret;

	if (req_len < sizeof(*gpio_req)) {
		pr_err("set direction request ignored: size mismatch (%d < %zu)\n", req_len, sizeof(*gpio_req));
		return 2;
	}

	gpio = be32_to_cpu(gpio_req->gpio);
	if (gpio_req->direction)
		ret = gpio_direction_output(gpio, gpio_req->value);
	else
		ret = gpio_direction_input(gpio);

	gpio_rsp_len = sizeof(struct ratp_bb_gpio_set_direction_response);
	gpio_rsp = xzalloc(gpio_rsp_len);
	gpio_rsp->header.type = cpu_to_be16(BB_RATP_TYPE_GPIO_SET_DIRECTION_RETURN);
	gpio_rsp->errno = (ret == 0 ? 0 : EIO);

	*rsp_len = gpio_rsp_len;
	*rsp = (struct ratp_bb *)gpio_rsp;
	return 0;
}

BAREBOX_RATP_CMD_START(GPIO_SET_DIRECTION)
	.request_id = BB_RATP_TYPE_GPIO_SET_DIRECTION,
	.response_id = BB_RATP_TYPE_GPIO_SET_DIRECTION_RETURN,
	.cmd = ratp_cmd_gpio_set_direction
BAREBOX_RATP_CMD_END
