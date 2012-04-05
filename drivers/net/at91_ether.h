/*
 * Ethernet driver for the Atmel AT91RM9200 (Thunder)
 *
 *  Copyright (C) SAN People (Pty) Ltd
 *
 * Based on an earlier Atmel EMAC macrocell driver by Atmel and Lineo Inc.
 * Initial version by Rick Bronson.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef AT91_ETHERNET
#define AT91_ETHERNET

/* ........................................................................ */

#define MAX_RBUFF_SZ	0x600		/* 1518 rounded up */
#define MAX_RX_DESCR	64		/* max number of receive buffers */

/* ----- Ethernet Buffer definitions ----- */
#define RBF_ADDR	0xfffffffc
#define RBF_OWNER	(1<<0)
#define RBF_WRAP	(1<<1)
#define RBF_SIZE	0x07ff

struct rbf_t
{
	unsigned int addr;
	unsigned long size;
};

/*
 * Read from a EMAC register.
 */
static inline unsigned long at91_emac_read(unsigned int reg)
{
	return __raw_readl(AT91_VA_BASE_EMAC + reg);
}

/*
 * Write to a EMAC register.
 */
static inline void at91_emac_write(unsigned int reg, unsigned long value)
{
	__raw_writel(value, AT91_VA_BASE_EMAC + reg);
}
#endif
