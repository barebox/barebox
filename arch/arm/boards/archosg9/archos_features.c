/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <boot.h>
#include <asm/setup.h>
#include "archos_features.h"
#include "feature_list.h"

static inline void *atag_data(struct tag *t)
{
	return ((void *)t) + sizeof(struct tag_header);
}

static struct feature_tag *features;

static void setup_feature_core(void)
{
	features->hdr.tag = FTAG_CORE;
	features->hdr.size = feature_tag_size(feature_tag_core);

	features->u.core.magic = FEATURE_LIST_MAGIC;
	features->u.core.list_revision = FEATURE_LIST_REV;
	features->u.core.flags = 0;

	features = feature_tag_next(features);
}
static void setup_feature_product_name(void)
{
	features->hdr.tag = FTAG_PRODUCT_NAME;
	features->hdr.size = feature_tag_size(feature_tag_product_name);

	memset(features->u.product_name.name, 0,
		sizeof(features->u.product_name.name));
	sprintf(features->u.product_name.name, "A80S");
	features->u.product_name.id = 0x13A8;

	features = feature_tag_next(features);
}
static void setup_feature_product_serial_number(void)
{
	features->hdr.tag = FTAG_PRODUCT_SERIAL_NUMBER;
	features->hdr.size = feature_tag_size(feature_tag_product_serial);

	features->u.product_serial.serial[0] = 0;
	features->u.product_serial.serial[1] = 0;
	features->u.product_serial.serial[2] = 0;
	features->u.product_serial.serial[3] = 0;

	features = feature_tag_next(features);
}
static void setup_feature_product_mac_address(void)
{
	features->hdr.tag = FTAG_PRODUCT_MAC_ADDRESS;
	features->hdr.size = feature_tag_size(feature_tag_product_mac_address);

	features->u.mac_address.addr[0] = 0;
	features->u.mac_address.addr[1] = 0;
	features->u.mac_address.addr[2] = 0;
	features->u.mac_address.addr[3] = 0;
	features->u.mac_address.addr[4] = 0;
	features->u.mac_address.addr[5] = 0;
	features->u.mac_address.reserved1 = 0;
	features->u.mac_address.reserved2 = 0;

	features = feature_tag_next(features);
}
static void setup_feature_board_pcb_revision(void)
{
	features->hdr.tag = FTAG_BOARD_PCB_REVISION;
	features->hdr.size = feature_tag_size(feature_tag_board_revision);

	features->u.board_revision.revision = 5;

	features = feature_tag_next(features);
}
static void setup_feature_sdram(void)
{
	features->hdr.tag = FTAG_SDRAM;
	features->hdr.size = feature_tag_size(feature_tag_sdram);

	memset(features->u.sdram.vendor, 0, sizeof(features->u.sdram.vendor));
	memset(features->u.sdram.product, 0,
		sizeof(features->u.sdram.product));
	sprintf(features->u.sdram.vendor , "elpida");
	sprintf(features->u.sdram.product, "EDB8064B1PB"/*"EDB4064B2PB"*/);
	features->u.sdram.type     = 0;
	features->u.sdram.revision = 0;
	features->u.sdram.flags    = 0;
	features->u.sdram.clock    = 400;
	features->u.sdram.param_0  = 0;
	features->u.sdram.param_1  = 0;
	features->u.sdram.param_2  = 0;
	features->u.sdram.param_3  = 0;
	features->u.sdram.param_4  = 0;
	features->u.sdram.param_5  = 0;
	features->u.sdram.param_6  = 0;
	features->u.sdram.param_7  = 0;

	features = feature_tag_next(features);
}
static void setup_feature_pmic(void)
{
	features->hdr.tag = FTAG_PMIC;
	features->hdr.size = feature_tag_size(feature_tag_pmic);

	features->u.pmic.flags = FTAG_PMIC_TPS62361;

	features = feature_tag_next(features);
}
static void setup_feature_serial_port(void)
{
	features->hdr.tag = FTAG_SERIAL_PORT;
	features->hdr.size = feature_tag_size(feature_tag_serial_port);

	features->u.serial_port.uart_id = 1;
	features->u.serial_port.speed = 115200;

	features = feature_tag_next(features);
}
static void setup_feature_has_gpio_volume_keys(void)
{
	features->hdr.tag = FTAG_HAS_GPIO_VOLUME_KEYS;
	features->hdr.size = feature_tag_size(feature_tag_gpio_volume_keys);

	features->u.gpio_volume_keys.gpio_vol_up   = 0x2B;
	features->u.gpio_volume_keys.gpio_vol_down = 0x2C;
	features->u.gpio_volume_keys.flags = 0;

	features = feature_tag_next(features);
}
static void setup_feature_screen(void)
{
	features->hdr.tag = FTAG_SCREEN;
	features->hdr.size = feature_tag_size(feature_tag_screen);

	memset(features->u.screen.vendor, 0,
		sizeof(features->u.screen.vendor));
	sprintf(features->u.screen.vendor, "CMI");
	features->u.screen.type = 0;
	features->u.screen.revision = 0;
	features->u.screen.vcom = 0;
	features->u.screen.backlight = 0xC8;
	features->u.screen.reserved[0] = 0;
	features->u.screen.reserved[1] = 0;
	features->u.screen.reserved[2] = 0;
	features->u.screen.reserved[3] = 0;
	features->u.screen.reserved[4] = 0;

	features = feature_tag_next(features);
}
static void setup_feature_turbo(void)
{
	features->hdr.tag = FTAG_TURBO;
	features->hdr.size = feature_tag_size(feature_tag_turbo);

	features->u.turbo.flag = 1;

	features = feature_tag_next(features);
}
static void setup_feature_none(void)
{
	features->hdr.tag = FTAG_NONE;
	features->hdr.size = sizeof(struct feature_tag_header) >> 2;

	features = feature_tag_next(features);
}
static struct tag *setup_feature_list(struct tag * params)
{
	struct tag_feature_list *fl;

	fl = atag_data(params);
	features = (struct feature_tag *)fl->data;

	setup_feature_core();
	setup_feature_product_name();
	setup_feature_product_serial_number();
	setup_feature_product_mac_address();
	setup_feature_board_pcb_revision();
	setup_feature_sdram();
	setup_feature_pmic();
	setup_feature_serial_port();
	setup_feature_has_gpio_volume_keys();
	setup_feature_screen();
	setup_feature_turbo();
	setup_feature_none();

	fl->size = ((u32)features) - ((u32)(fl->data));

	params->hdr.tag = ATAG_FEATURE_LIST;
	params->hdr.size = (sizeof(struct tag_feature_list) + fl->size) >> 2;

	return tag_next(params);
}

static struct tag *setup_boot_version(struct tag *params)
{
	struct tag_boot_version *bv;

	bv = atag_data(params);

	params->hdr.tag = ATAG_BOOT_VERSION;
	params->hdr.size = tag_size(tag_boot_version);

	bv->major = 5;
	bv->minor = 5;
	bv->extra = 3;

	return tag_next(params);
}

struct tag *archos_append_atags(struct tag *params)
{
	params = setup_feature_list(params);
	params = setup_boot_version(params);
	return params;
}
