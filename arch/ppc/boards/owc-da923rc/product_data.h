/*
 * Copyright 2013 GE Intelligent Platforms, Inc.
 *
 * The product data structure and function prototypes.
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
 */

struct board_info {
	uint32_t bid;
	uint32_t rev;
};

#define MAX_MAC		8
enum product_data_version {
	PDVERSION_V1 = 1,
	PDVERSION_V1bis = 0x10,
	PDVERSION_V2 = 2,
	PDVERSION_MAX = PDVERSION_V2,
};

struct __attribute__ ((__packed__)) product_data_header {
	unsigned short tag;
	unsigned char version;
	unsigned short len;
};

struct __attribute__ ((__packed__)) mac {
	unsigned char count;
	unsigned char mac[MAX_MAC][6];
};

struct __attribute__ ((__packed__)) product_data_v1 {
	struct product_data_header pdh;
	struct mac mac;
	int crc32;
};

struct __attribute__ ((__packed__)) product_data_v2 {
	struct product_data_header pdh;
	struct mac mac;
	char sn[8];
	int crc32;
};

struct __attribute__ ((__packed__)) ge_product_data {
	union {
		struct product_data_v1 v1;
		struct product_data_v2 v2;
	};
};

extern int ge_get_product_data(struct ge_product_data *productp);
extern void da923rc_boardinfo_get(struct board_info *bi);
