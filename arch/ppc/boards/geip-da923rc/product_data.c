/*
 * Copyright 2013 GE Intelligent Platforms Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Retrieve and check the product data.
 */

#include <common.h>
#include <i2c/i2c.h>
#include <mach/immap_85xx.h>
#include <mach/fsl_i2c.h>
#include "product_data.h"

static int ge_pd_header_check(unsigned short header)
{
	if (header != 0xa5a5)
		return -1;
	else
		return 0;
}

static int ge_is_data_valid(struct ge_product_data *v)
{
	int crc, ret = 0;
	const unsigned char *p = (const unsigned char *)v;

	if (ge_pd_header_check(v->v1.pdh.tag))
		return -1;

	switch (v->v1.pdh.version) {
	case PDVERSION_V1:
	case PDVERSION_V1bis:
		crc = crc32(0, p, sizeof(struct product_data_v1) - 4);
		if (crc != v->v1.crc32)
			ret = -1;
		break;
	case PDVERSION_V2:
		crc = crc32(0, p, sizeof(struct product_data_v2) - 4);
		if (crc != v->v2.crc32)
			ret = -1;
		break;
	default:
		ret = -1;
	}

	return ret;
}

int ge_get_product_data(struct ge_product_data *productp)
{
	struct i2c_adapter *adapter;
	struct i2c_client client;
	unsigned int width = 0;
	int ret;

	adapter = i2c_get_adapter(0);
	client.addr = 0x51;
	client.adapter = adapter;
	ret = i2c_read_reg(&client, 0, (uint8_t *) productp,
			   sizeof(unsigned short));

	/* If there is no valid header, it may be a 16-bit eeprom. */
	if (ge_pd_header_check(productp->v1.pdh.tag))
		width = I2C_ADDR_16_BIT;

	ret = i2c_read_reg(&client, width, (uint8_t *) productp,
			   sizeof(struct ge_product_data));

	if (ret == sizeof(struct ge_product_data))
		ret = ge_is_data_valid(productp);

	return ret;
}
