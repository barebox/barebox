/*
 * Copyright (C) 2013 Gateworks Corporation
 * Copyright (C) 2014 Lucas Stach, Pengutronix
 * Author: Tim Harvey <tharvey@gateworks.com>
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

/* i2c slave addresses */
#define	GSC_SC_ADDR			0x20
#define	GSC_RTC_ADDR			0x68
#define	GSC_HWMON_ADDR			0x29
#define	GSC_EEPROM_ADDR			0x51

/* System Controller registers */
#define	GSC_SC_CTRL0			0x00
#define	GSC_SC_CTRL1			0x01
#define		GSC_SC_CTRL1_WDDIS	(1 << 7)
#define	GSC_SC_STATUS			0x0a
#define	GSC_SC_FWVER			0x0e

/* System Controller Interrupt bits */
#define	GSC_SC_IRQ_PB			0 /* Pushbutton switch */
#define	GSC_SC_IRQ_SECURE		1 /* Secure Key erase complete */
#define	GSC_SC_IRQ_EEPROM_WP		2 /* EEPROM write violation */
#define	GSC_SC_IRQ_GPIO			4 /* GPIO change */
#define	GSC_SC_IRQ_TAMPER		5 /* Tamper detect */
#define	GSC_SC_IRQ_WATCHDOG		6 /* Watchdog trip */
#define	GSC_SC_IRQ_PBLONG		7 /* Pushbutton long hold */

/* Hardware Monitor registers */
#define	GSC_HWMON_TEMP			0x00
#define	GSC_HWMON_VIN			0x02
#define	GSC_HWMON_VDD_3P3		0x05
#define	GSC_HWMON_VBATT			0x08
#define	GSC_HWMON_VDD_5P0		0x0b
#define	GSC_HWMON_VDD_CORE		0x0e
#define	GSC_HWMON_VDD_HIGH		0x14
#define	GSC_HWMON_VDD_DDR		0x17
#define	GSC_HWMON_VDD_SOC		0x11
#define	GSC_HWMON_VDD_1P8		0x1d
#define	GSC_HWMON_VDD_2P5		0x23
#define	GSC_HWMON_VDD_1P0		0x20

/*
 * I2C transactions to the GSC are done via these functions which
 * perform retries in the case of a busy GSC NAK'ing the transaction
 */
int gsc_i2c_read(struct i2c_client *client, u32 addr, u8 *buf, u16 count);
int gsc_i2c_write(struct i2c_client *client, u32 addr, const u8 *buf, u16 count);

char gsc_get_rev(struct i2c_client *client);
