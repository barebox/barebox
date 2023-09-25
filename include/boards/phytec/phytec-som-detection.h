/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#ifndef _BOARDS_PHYTEC_PHYTEC_SOM_DETECTION_H
#define _BOARDS_PHYTEC_PHYTEC_SOM_DETECTION_H

#include <common.h>
#include <pbl/i2c.h>

#define PHYTEC_MAX_OPTIONS	17
#define PHYTEC_IMX8MQ_SOM	66
#define PHYTEC_IMX8MM_SOM	69
#define PHYTEC_IMX8MP_SOM	70

#define PHYTEC_EEPROM_INVAL	0xff

enum {
	PHYTEC_API_REV0 = 0,
	PHYTEC_API_REV1,
	PHYTEC_API_REV2,
};

static const char * const phytec_som_type_str[] = {
	"PCM",
	"PCL",
	"KSM",
	"KSP",
};

struct phytec_api0_data {
	u8 pcb_rev;		/* PCB revision of SoM */
	u8 som_type;		/* SoM type */
	u8 ksp_no;		/* KSP no */
	char opt[16];		/* SoM options */
	u8 mac[6];		/* MAC address (optional) */
	u8 __pad[5];		/* padding */
	u8 cksum;		/* checksum */
} __attribute__ ((__packed__));

struct phytec_api2_data {
	u8 pcb_rev;		/* PCB revision of SoM */
	u8 pcb_sub_opt_rev;	/* PCB subrevision and opt revision */
	u8 som_type;		/* SoM type */
	u8 som_no;		/* SoM number */
	u8 ksp_no;		/* KSP information */
	char opt[PHYTEC_MAX_OPTIONS]; /* SoM options */
	char bom_rev[2];	/* BOM revision */
	u8 mac[6];		/* MAC address (optional) */
	u8 crc8;		/* checksum */
} __attribute__ ((__packed__));

struct phytec_eeprom_data {
	u8 api_rev;
	union {
		struct phytec_api0_data data_api0;
		struct phytec_api2_data data_api2;
	} data;
} __attribute__ ((__packed__));

const char *phytec_get_opt(const struct phytec_eeprom_data *data);
int phytec_eeprom_data_setup(struct pbl_i2c *i2c, struct phytec_eeprom_data *data,
			     int addr, int addr_fallback, u8 cpu_type);

void phytec_print_som_info(const struct phytec_eeprom_data *data);

#endif /* _BOARDS_PHYTEC_PHYTEC_SOM_DETECTION_H */
