/*
 * Copyright (C) 2009 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 * Derived from:
 * - arch-mxc/pmic_external.h --  contains interface of the PMIC protocol driver
 *   Copyright 2008-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 */

#ifndef __MFD_MC13XXX_H
#define __MFD_MC13XXX_H

#define MC13892_REG_INT_STATUS0		0x00
#define MC13892_REG_INT_MASK0		0x01
#define MC13892_REG_INT_SENSE0		0x02
#define MC13892_REG_INT_STATUS1		0x03
#define MC13892_REG_INT_MASK1		0x04
#define MC13892_REG_INT_SENSE1		0x05
#define MC13892_REG_PU_MODE_S		0x06
#define MC13892_REG_IDENTIFICATION	0x07
#define MC13892_REG_UNUSED0		0x08
#define MC13892_REG_ACC0		0x09
#define MC13892_REG_ACC1		0x0a
#define MC13892_REG_UNUSED1		0x0b
#define MC13892_REG_UNUSED2		0x0c
#define MC13892_REG_POWER_CTL0		0x0d
#define MC13892_REG_POWER_CTL1		0x0e
#define MC13892_REG_POWER_CTL2		0x0f
#define MC13892_REG_REGEN_ASSIGN	0x10
#define MC13892_REG_UNUSED3		0x11
#define MC13892_REG_MEM_A		0x12
#define MC13892_REG_MEM_B		0x13
#define MC13892_REG_RTC_TIME		0x14
#define MC13892_REG_RTC_ALARM		0x15
#define MC13892_REG_RTC_DAY		0x16
#define MC13892_REG_RTC_DAY_ALARM	0x17
#define MC13892_REG_SW_0		0x18
#define MC13892_REG_SW_1		0x19
#define MC13892_REG_SW_2		0x1a
#define MC13892_REG_SW_3		0x1b
#define MC13892_REG_SW_4		0x1c
#define MC13892_REG_SW_5		0x1d
#define MC13892_REG_SETTING_0		0x1e
#define MC13892_REG_SETTING_1		0x1f
#define MC13892_REG_MODE_0		0x20
#define MC13892_REG_MODE_1		0x21
#define MC13892_REG_POWER_MISC		0x22
#define MC13892_REG_UNUSED4		0x23
#define MC13892_REG_UNUSED5		0x24
#define MC13892_REG_UNUSED6		0x25
#define MC13892_REG_UNUSED7		0x26
#define MC13892_REG_UNUSED8		0x27
#define MC13892_REG_UNUSED9		0x28
#define MC13892_REG_UNUSED10		0x29
#define MC13892_REG_UNUSED11		0x2a
#define MC13892_REG_ADC0		0x2b
#define MC13892_REG_ADC1		0x2c
#define MC13892_REG_ADC2		0x2d
#define MC13892_REG_ADC3		0x2e
#define MC13892_REG_ADC4		0x2f
#define MC13892_REG_CHARGE		0x30
#define MC13892_REG_USB0		0x31
#define MC13892_REG_USB1		0x32
#define MC13892_REG_LED_CTL0		0x33
#define MC13892_REG_LED_CTL1		0x34
#define MC13892_REG_LED_CTL2		0x35
#define MC13892_REG_LED_CTL3		0x36
#define MC13892_REG_UNUSED12		0x37
#define MC13892_REG_UNUSED13		0x38
#define MC13892_REG_TRIM0		0x39
#define MC13892_REG_TRIM1		0x3a
#define MC13892_REG_TEST0		0x3b
#define MC13892_REG_TEST1		0x3c
#define MC13892_REG_TEST2		0x3d
#define MC13892_REG_TEST3		0x3e
#define MC13892_REG_TEST4		0x3f

enum mc13892_revision {
	MC13892_REVISION_1_0,
	MC13892_REVISION_1_1,
	MC13892_REVISION_1_2,
	MC13892_REVISION_2_0,
	MC13892_REVISION_2_0a,
	MC13892_REVISION_2_1,
	MC13892_REVISION_3_0,
	MC13892_REVISION_3_1,
	MC13892_REVISION_3_2,
	MC13892_REVISION_3_2a,
	MC13892_REVISION_3_3,
	MC13892_REVISION_3_5,
};

enum mc13xxx_mode {
	MC13XXX_MODE_I2C,
	MC13XXX_MODE_SPI,
};

struct mc13892 {
	struct cdev		cdev;
	struct i2c_client	*client;
	struct spi_device	*spi;
	enum mc13xxx_mode	mode;
	enum mc13892_revision	revision;
};

extern struct mc13892 *mc13892_get(void);

extern int mc13892_reg_read(struct mc13892 *mc13892, u8 reg, u32 *val);
extern int mc13892_reg_write(struct mc13892 *mc13892, u8 reg, u32 val);
extern int mc13892_set_bits(struct mc13892 *mc13892, u8 reg, u32 mask, u32 val);

static inline enum mc13892_revision mc13892_get_revision(struct mc13892 *mc13892)
{
	return mc13892->revision;
}

#endif /* __MFD_MC13XXX_H */
