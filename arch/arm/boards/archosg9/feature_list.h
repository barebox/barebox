#ifndef _FEATURE_LIST_H
#define _FEATURE_LIST_H

/*
 This file comes from:
 http://gitorious.org/archos/archos-gpl-gen9-kernel-ics/blobs/raw/master/
 arch/arm/include/asm/feature_list.h
*/

#define FEATURE_LIST_MAGIC		0xFEA01234

#define FEATURE_LIST_REV		0x00000001

struct feature_tag_header {
	u32 size;
	u32 tag;
};

struct feature_tag_generic {
	u32 vendor;
	u32 product;
	u32 type;
	u32 revision;
	u32 flags;
};

#define FTAG_NONE			0x00000000

#define FTAG_CORE			0x00000001
struct feature_tag_core {
	u32 magic;
	u32 list_revision;
	u32 flags;
};

/* product specific */
#define FTAG_PRODUCT_NAME		0x00000002
struct feature_tag_product_name {
	char name[64];
	u32 id;
};
#define FTAG_PRODUCT_SERIAL_NUMBER	0x00000003
struct feature_tag_product_serial {
	u32 serial[4];
};

#define FTAG_PRODUCT_MAC_ADDRESS	0x00000004
struct feature_tag_product_mac_address {
	u8 addr[6];
	u8 reserved1;
	u8 reserved2;
};

#define FTAG_PRODUCT_OEM		0x00000005
struct feature_tag_product_oem {
	char name[16];
	u32 id;
};

#define FTAG_PRODUCT_ZONE		0x00000006
struct feature_tag_product_zone {
	char name[16];
	u32 id;
};

/* board pcb specific */
#define FTAG_BOARD_PCB_REVISION		0x00000010
struct feature_tag_board_revision {
	u32 revision;
};

/* clock and ram setup */
#define FTAG_CLOCK			0x00000011
struct feature_tag_clock {
	u32 clock;
};

#define FTAG_SDRAM			0x00000012
struct feature_tag_sdram {
	char vendor[16];
	char product[32];
	u32 type;
	u32 revision;
	u32 flags;
	u32 clock;
	/* custom params */
	u32 param_0;
	u32 param_1;
	u32 param_2;
	u32 param_3;
	u32 param_4;
	u32 param_5;
	u32 param_6;
	u32 param_7;
};

/* PMIC */
#define FTAG_PMIC			0x00000013
#define FTAG_PMIC_TPS62361		0x00000001
struct feature_tag_pmic {
	u32 flags;
};

/* serial port */
#define FTAG_SERIAL_PORT		0x00000020
struct feature_tag_serial_port {
	u32 uart_id;
	u32 speed;
};

/* turbo bit */
#define FTAG_TURBO			0x00000014
struct feature_tag_turbo {
	u32 flag;
};

/*** features ****/
#define FTAG_HAS_GPIO_VOLUME_KEYS	0x00010001
struct feature_tag_gpio_volume_keys {
	u32 gpio_vol_up;
	u32 gpio_vol_down;
	u32 flags;
};

#define FTAG_HAS_ELECTRICAL_SHORTCUT	0x00010002
#define FTAG_HAS_DCIN			0x00010003
struct feature_tag_dcin {
	u32 autodetect;
};

/* external screen support */
#define FTAG_HAS_EXT_SCREEN		0x00010004

#define EXT_SCREEN_TYPE_TVOUT	0x00000001
#define EXT_SCREEN_TYPE_HDMI	0x00000002
#define EXT_SCREEN_TYPE_VGA	0x00000004
struct feature_tag_ext_screen {
	u32 type;
	u32 revision;
};

/* wireless lan */
#define FTAG_HAS_WIFI			0x00010005

#define WIFI_TYPE_TIWLAN	0x00000001
struct feature_tag_wifi {
	u32 vendor;
	u32 product;
	u32 type;
	u32 revision;
	u32 flags;
};

/* bluetooth */
#define FTAG_HAS_BLUETOOTH		0x00010006

#define BLUETOOTH_TYPE_TIWLAN	0x00000001
struct feature_tag_bluetooth {
	u32 vendor;
	u32 product;
	u32 type;
	u32 revision;
	u32 flags;
};

/* accelerometer */
#define FTAG_HAS_ACCELEROMETER		0x00010007
struct feature_tag_accelerometer {
	u32 vendor;
	u32 product;
	u32 type;
	u32 revision;
	u32 flags;
};

/* gyroscope */
#define FTAG_HAS_GYROSCOPE		0x00010008

/* compass */
#define FTAG_HAS_COMPASS		0x00010009

/* gps */
#define FTAG_HAS_GPS			0x0001000a
#define GPS_FLAG_DISABLED		0x00000001
struct feature_tag_gps {
	u32 vendor;
	u32 product;
	u32 revision;
	u32 flags;
};

/* camera */
#define FTAG_HAS_CAMERA			0x0001000b

/* harddisk controller */
#define FTAG_HAS_HARDDISK_CONTROLLER	0x0001000c
#define HDCONTROLLER_TYPE_SATA	0x00000001
struct feature_tag_harddisk_controller {
	u32 vendor;
	u32 product;
	u32 type;
	u32 revision;
	u32 flags;
};

/* harddisk */
#define FTAG_HAS_HARDDISK		0x0001000d

#define HARDDISK_TYPE_SATA	0x00000001
#define HARDDISK_TYPE_PATA	0x00000002
struct feature_tag_harddisk {
	u32 vendor;
	u32 product;
	u32 type;
	u32 revision;
	u32 flags;
};

/* touchscreen */
#define FTAG_HAS_TOUCHSCREEN		0x0001000e

#define TOUCHSCREEN_TYPE_CAPACITIVE 0x00000001
#define TOUCHSCREEN_TYPE_RESISTIVE  0x00000002

#define TOUCHSCREEN_FLAG_MULTITOUCH 0x00000001
struct feature_tag_touchscreen {
	u32 vendor;
	u32 product;
	u32 type;
	u32 revision;
	u32 flags;
};

/* microphone */
#define FTAG_HAS_MICROPHONE		0x0001000f

/* external SDMMC slot */
#define FTAG_HAS_EXT_MMCSD_SLOT		0x00010010
#define MMCSD_FLAG_CARDDETECT    0x00000001
#define MMCSD_FLAG_CARDPREDETECT 0x00000002

struct feature_tag_mmcsd {
	u32 width;
	u32 voltagemask;
	u32 revision;
	u32 flags;
};

/* ambient light sensor */
#define FTAG_HAS_AMBIENT_LIGHT_SENSOR	0x00010011

/* proximity sensor */
#define FTAG_HAS_PROXIMITY_SENSOR	0x00010012

/* gps */
#define FTAG_HAS_GSM			0x00010013

/* dect */
#define FTAG_HAS_DECT			0x00010014

/* hsdpa data modem */
#define FTAG_HAS_HSDPA			0x00010015

/* near field communication */
#define FTAG_HAS_NFC			0x00010016

#define FTAG_GPIO_KEYS			0x00010017
struct feature_tag_gpio_keys {
#define GPIO_KEYS_LONG_PRESS		0x00010000
	u32 vol_up;
	u32 vol_down;
	u32 ok;
	u32 reserved[5];
};

#define FTAG_SCREEN			0x00010018
struct feature_tag_screen {
	char vendor[16];
	u32 type;
	u32 revision;
	u32 vcom;
	u32 backlight;
	u32 reserved[5];
};

#define FTAG_WIFI_PA			0x00010019
struct feature_tag_wifi_pa {
	char vendor[16];
	u32 type;
};

/* loudspeaker */
#define FTAG_HAS_SPEAKER		0x0001001a

#define SPEAKER_FLAG_STEREO	 0x00000001
#define SPEAKER_FLAG_OWN_VOLCTRL 0x00000002
struct feature_tag_speaker {
	u32 flags;
};

#define FTAG_BATTERY			0x0001001b
struct feature_tag_battery {
	u32 type;
};
#define BATTERY_TYPE_HIGHRS	 0x00000000
#define BATTERY_TYPE_LOWRS	 0x00000001


#define feature_tag_next(t) \
	((struct feature_tag *)((u32 *)(t) + (t)->hdr.size))
#define feature_tag_size(type) \
	((sizeof(struct feature_tag_header) + sizeof(struct type)) >> 2)
#define for_each_feature_tag(t, base) \
	for (t = base; t->hdr.size; t = feature_tag_next(t))


struct feature_tag {
	struct feature_tag_header hdr;
	union {
		struct feature_tag_core			core;
		struct feature_tag_generic		generic;
		struct feature_tag_product_name		product_name;
		struct feature_tag_product_serial	product_serial;
		struct feature_tag_product_oem		product_oem;
		struct feature_tag_product_zone		product_zone;
		struct feature_tag_product_mac_address	mac_address;
		struct feature_tag_board_revision	board_revision;
		struct feature_tag_clock		clock;
		struct feature_tag_sdram		sdram;
		struct feature_tag_pmic			pmic;
		struct feature_tag_turbo		turbo;
		struct feature_tag_serial_port		serial_port;
		struct feature_tag_gpio_volume_keys	gpio_volume_keys;
		struct feature_tag_dcin			dcin;
		struct feature_tag_ext_screen		ext_screen;
		struct feature_tag_wifi			wifi;
		struct feature_tag_bluetooth		bluetooth;
		struct feature_tag_accelerometer	accelerometer;
		struct feature_tag_harddisk_controller	harddisk_controller;
		struct feature_tag_harddisk		harddisk;
		struct feature_tag_touchscreen		touchscreen;
		struct feature_tag_gps			gps;
		struct feature_tag_speaker		speaker;
		struct feature_tag_mmcsd		mmcsd;
		struct feature_tag_gpio_keys		gpio_keys;
		struct feature_tag_screen		screen;
		struct feature_tag_wifi_pa		wifi_pa;
		struct feature_tag_battery		battery;
	} u;
};

#endif /* _FEATURE_LIST_H */
