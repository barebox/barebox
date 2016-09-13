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

#include <linux/types.h>

#define MC13XXX_REG_IDENTIFICATION	0x07

#define MC13783_REG_INT_STATUS0		0x00
#define MC13783_REG_INT_MASK0		0x01
#define MC13783_REG_INT_SENSE0		0x02
#define MC13783_REG_INT_STATUS1		0x03
#define MC13783_REG_INT_MASK1		0x04
#define MC13783_REG_INT_SENSE1		0x05
#define MC13783_REG_PU_MODE_S		0x06
#define MC13783_REG_SEMAPHORE		0x08
#define MC13783_REG_ARB_PER_AUDIO	0x09
#define MC13783_REG_ARB_SWITCHERS	0x0a
#define MC13783_REG_ARB_REGULATORS(x)	(0x0b + (x)) /* 0 .. 1 */
#define MC13783_REG_POWER_CONTROL(x)	(0x0d + (x)) /* 0 .. 2 */
#define MC13783_REG_REGEN_ASSIGNMENT	0x10
#define MC13783_REG_CONTROL_SPARE	0x11
#define MC13783_REG_MEMORY_A		0x12
#define MC13783_REG_MEMORY_B		0x13
#define MC13783_REG_RTC_TIME		0x14
#define MC13783_REG_RTC_ALARM		0x15
#define MC13783_REG_RTC_DAY		0x16
#define MC13783_REG_RTC_DAY_ALARM	0x17
#define MC13783_REG_SWITCHERS(x)	(0x18 + (x)) /* 0 .. 5 */
#define MC13783_REG_REG_SETTING(x)	(0x1e + (x)) /* 0 .. 1 */
#define MC13783_REG_REG_MODE(x)		(0x20 + (x)) /* 0 .. 1 */
#define MC13783_REG_POWER_MISC		0x22
#define MC13783_REG_POWER_SPARE		0x23
#define MC13783_REG_AUDIO_RX_0		0x24
#define MC13783_REG_AUDIO_RX_1		0x25
#define MC13783_REG_AUDIO_TX		0x26
#define MC13783_REG_AUDIO_SSI_NETWORK	0x27
#define MC13783_REG_AUDIO_CODEC		0x28
#define MC13783_REG_AUDIO_STEREO_DAC	0x29
#define MC13783_REG_AUDIO_SPARE		0x2a
#define MC13783_REG_ADC(x)		(0x2b + (x)) /* 0 .. 4 */
#define MC13783_REG_CHARGER		0x30
#define MC13783_REG_USB			0x31
#define MC13783_REG_CHARGE_USB_SPARE	0x32
#define MC13783_REG_LED_CONTROL(x)	(0x33 + (x)) /* 0 .. 5 */
#define MC13783_REG_SPARE		0x39
#define MC13783_REG_TRIM(x)		(0x3a + (x)) /* 0 .. 1 */
#define MC13783_REG_TEST(x)		(0x3c + (x)) /* 0 .. 3 */

#define MC13892_REG_INT_STATUS0		0x00
#define MC13892_REG_INT_MASK0		0x01
#define MC13892_REG_INT_SENSE0		0x02
#define MC13892_REG_INT_STATUS1		0x03
#define MC13892_REG_INT_MASK1		0x04
#define MC13892_REG_INT_SENSE1		0x05
#define MC13892_REG_PU_MODE_S		0x06
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

#define MC13892_REVISION_1_0		0
#define MC13892_REVISION_1_1		1
#define MC13892_REVISION_1_2		2
#define MC13892_REVISION_2_0		3
#define MC13892_REVISION_2_0a		4
#define MC13892_REVISION_2_1		5
#define MC13892_REVISION_2_4		6
#define MC13892_REVISION_3_0		7
#define MC13892_REVISION_3_1		8
#define MC13892_REVISION_3_2		9
#define MC13892_REVISION_3_2a		10
#define MC13892_REVISION_3_3		11
#define MC13892_REVISION_3_5		12

#define MC13783_SWX_VOLTAGE(x)		((x) & 0x3f)
#define MC13783_SWX_VOLTAGE_DVS(x)	(((x) & 0x3f) << 6)
#define MC13783_SWX_VOLTAGE_STANDBY(x)	(((x) & 0x3f) << 12)
#define MC13783_SWX_VOLTAGE_1_450	0x16

#define MC13783_SWX_MODE_OFF		0
#define MC13783_SWX_MODE_NO_PULSE_SKIP	1
#define MC13783_SWX_MODE_PULSE_SKIP	2
#define MC13783_SWX_MODE_LOW_POWER_PFM	3

#define MC13783_SW1A_MODE(x)		(((x) & 0x3) << 0)
#define MC13783_SW1A_MODE_STANDBY(x)	(((x) & 0x3) << 2)
#define MC13783_SW1B_MODE(x)		(((x) & 0x3) << 10)
#define MC13783_SW1B_MODE_STANDBY(x)	(((x) & 0x3) << 12)
#define MC13783_SW1A_SOFTSTART		(1 << 9)
#define MC13783_SW1B_SOFTSTART		(1 << 17)
#define MC13783_SW_PLL_FACTOR(x)	(((x) - 28) << 19)

/* MC34708 Definitions */
#define SWx_VOLT_MASK_MC34708	0x3F
#define SWx_1_250V_MC34708	0x30
#define SWx_1_300V_MC34708	0x34
#define TIMER_MASK_MC34708	0x300
#define TIMER_4S_MC34708	0x100
#define VUSBSEL_MC34708		(1 << 2)
#define VUSBEN_MC34708		(1 << 3)
#define SWBST_CTRL		31
#define SWBST_AUTO		0x8

struct mc13xxx;

#ifdef CONFIG_MFD_MC13XXX
extern struct mc13xxx *mc13xxx_get(void);
extern int mc13xxx_revision(struct mc13xxx *mc13xxx);
extern int mc13xxx_reg_read(struct mc13xxx *mc13xxx, u8 reg, u32 *val);
extern int mc13xxx_reg_write(struct mc13xxx *mc13xxx, u8 reg, u32 val);
extern int mc13xxx_set_bits(struct mc13xxx *mc13xxx, u8 reg, u32 mask, u32 val);
int mc13xxx_register_init_callback(void(*callback)(struct mc13xxx *mc13xxx));
#else
static inline struct mc13xxx *mc13xxx_get(void)
{
	return NULL;
}

static inline int mc13xxx_revision(struct mc13xxx *mc13xxx)
{
	return -ENODEV;
}

static inline int mc13xxx_reg_read(struct mc13xxx *mc13xxx, u8 reg, u32 *val)
{
	return -ENODEV;
}

static inline int mc13xxx_reg_write(struct mc13xxx *mc13xxx, u8 reg, u32 val)
{
	return -ENODEV;
}

static inline int mc13xxx_set_bits(struct mc13xxx *mc13xxx, u8 reg, u32 mask, u32 val)
{
	return -ENODEV;
}

static inline int mc13xxx_register_init_callback(void(*callback)(struct mc13xxx *mc13xxx))
{
	return -ENODEV;
}
#endif

#endif /* __MFD_MC13XXX_H */
