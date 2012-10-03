/*
 * Copyright (C) 2010 Juergen Beisert
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
 */

#ifndef __MACH_IOMUX_S3C24x0_H
#define __MACH_IOMUX_S3C24x0_H

/* 3322222222221111111111
 * 10987654321098765432109876543210
 *                            ^^^^^_ Bit offset
 *                        ^^^^______ Group Number
 *                    ^^____________ Function
 *                   ^______________ initial GPIO out value
 *                  ^_______________ Pull up feature present
 *                 ^________________ initial pull up setting
 */


#define PIN(group,bit) (group * 32 + bit)
#define FUNC(x) (((x) & 0x3) << 11)
#define GET_FUNC(x) (((x) >> 11) & 0x3)
#define GET_GROUP(x) (((x) >> 5) & 0xf)
#define GET_BIT(x) (((x) & 0x1ff) % 32)
#define GET_GPIOVAL(x) (((x) >> 13) & 0x1)
#define GET_GPIO_NO(x) ((x & 0x1ff))
#define GPIO_OUT FUNC(1)
#define GPIO_IN FUNC(0)
#define GPIO_VAL(x) ((!!(x)) << 13)
#define PU (1 << 14)
#define PU_PRESENT(x) (!!((x) & (1 << 14)))
#define ENABLE_PU (0 << 15)
#define DISABLE_PU (1 << 15)
#define GET_PU(x) (!!((x) & DISABLE_PU))

/*
 * Group 0: GPIO 0...31
 * Used GPIO: 0...22
 * These pins can also act as GPIO outputs
 */
#define GPA0_ADDR0		(PIN(0,0) | FUNC(2))
#define GPA0_ADDR0_GPIO		(PIN(0,0) | FUNC(0))
#define GPA1_ADDR16		(PIN(0,1) | FUNC(2))
#define GPA1_ADDR16_GPIO	(PIN(0,1) | FUNC(0))
#define GPA2_ADDR17		(PIN(0,2) | FUNC(2))
#define GPA2_ADDR17_GPIO	(PIN(0,2) | FUNC(0))
#define GPA3_ADDR18		(PIN(0,3) | FUNC(2))
#define GPA3_ADDR18_GPIO	(PIN(0,3) | FUNC(0))
#define GPA4_ADDR19		(PIN(0,4) | FUNC(2))
#define GPA4_ADDR19_GPIO	(PIN(0,4) | FUNC(0))
#define GPA5_ADDR20		(PIN(0,5) | FUNC(2))
#define GPA5_ADDR20_GPIO	(PIN(0,5) | FUNC(0))
#define GPA6_ADDR21		(PIN(0,6) | FUNC(2))
#define GPA6_ADDR21_GPIO	(PIN(0,6) | FUNC(0))
#define GPA7_ADDR22		(PIN(0,7) | FUNC(2))
#define GPA7_ADDR22_GPIO	(PIN(0,7) | FUNC(0))
#define GPA8_ADDR23		(PIN(0,8) | FUNC(2))
#define GPA8_ADDR23_GPIO	(PIN(0,8) | FUNC(0))
#define GPA9_ADDR24		(PIN(0,9) | FUNC(2))
#define GPA9_ADDR24_GPIO	(PIN(0,9) | FUNC(0))
#define GPA10_ADDR25		(PIN(0,10) | FUNC(2))
#define GPA10_ADDR25_GPIO	(PIN(0,10) | FUNC(0))
#define GPA11_ADDR26		(PIN(0,11) | FUNC(2))
#define GPA11_ADDR26_GPIO	(PIN(0,11) | FUNC(0))
#define GPA12_NGCS1		(PIN(0,12) | FUNC(2))
#define GPA12_NGCS1_GPIO	(PIN(0,12) | FUNC(0))
#define GPA13_NGCS2		(PIN(0,13) | FUNC(2))
#define GPA13_NGCS2_GPIO	(PIN(0,13) | FUNC(0))
#define GPA14_NGCS3		(PIN(0,14) | FUNC(2))
#define GPA14_NGCS3_GPIO	(PIN(0,14) | FUNC(0))
#define GPA15_NGCS4		(PIN(0,15) | FUNC(2))
#define GPA15_NGCS4_GPIO	(PIN(0,15) | FUNC(0))
#define GPA16_NGCS5		(PIN(0,16) | FUNC(2))
#define GPA16_NGCS5_GPIO	(PIN(0,16) | FUNC(0))
#define GPA17_CLE		(PIN(0,17) | FUNC(2))
#define GPA17_CLE_GPIO		(PIN(0,17) | FUNC(0))
#define GPA18_ALE		(PIN(0,18) | FUNC(2))
#define GPA18_ALE_GPIO		(PIN(0,18) | FUNC(0))
#define GPA19_NFWE		(PIN(0,19) | FUNC(2))
#define GPA19_NFWE_GPIO		(PIN(0,19) | FUNC(0))
#define GPA20_NFRE		(PIN(0,20) | FUNC(2))
#define GPA20_NFRE_GPIO		(PIN(0,20) | FUNC(0))
#define GPA21_NRSTOUT		(PIN(0,21) | FUNC(2))
#define GPA21_NRSTOUT_GPIO	(PIN(0,21) | FUNC(0))
#define GPA22_NFCE		(PIN(0,22) | FUNC(2))
#define GPA22_NFCE_GPIO		(PIN(0,22) | FUNC(0))

/*
 * Group 1: GPIO 32...63
 * Used GPIO: 0...10
 * these pins can also act as GPIO inputs/outputs
 */
#define GPB0_TOUT0	(PIN(1,0) | FUNC(2) | PU)
#define GPB0_GPIO	(PIN(1,0) | FUNC(0) | PU)
#define GPB1_TOUT1	(PIN(1,1) | FUNC(2) | PU)
#define GPB1_GPIO	(PIN(1,1) | FUNC(0) | PU)
#define GPB2_TOUT2	(PIN(1,2) | FUNC(2) | PU)
#define GPB2_GPIO	(PIN(1,2) | FUNC(0) | PU)
#define GPB3_TOUT3	(PIN(1,3) | FUNC(2) | PU)
#define GPB3_GPIO	(PIN(1,3) | FUNC(0) | PU)
#define GPB4_TCLK0	(PIN(1,4) | FUNC(2) | PU)
#define GPB4_GPIO	(PIN(1,4) | FUNC(0) | PU)
#define GPB5_NXBACK	(PIN(1,5) | FUNC(2) | PU)
#define GPB5_GPIO	(PIN(1,5) | FUNC(0) | PU)
#define GPB6_NXBREQ	(PIN(1,6) | FUNC(2) | PU)
#define GPB6_GPIO	(PIN(1,6) | FUNC(0) | PU)
#define GPB7_NXDACK1	(PIN(1,7) | FUNC(2) | PU)
#define GPB7_GPIO	(PIN(1,7) | FUNC(0) | PU)
#define GPB8_NXDREQ1	(PIN(1,8) | FUNC(2) | PU)
#define GPB8_GPIO	(PIN(1,8) | FUNC(0) | PU)
#define GPB9_NXDACK0	(PIN(1,9) | FUNC(2) | PU)
#define GPB9_GPIO	(PIN(1,9) | FUNC(0) | PU)
#define GPB10_NXDREQ0	(PIN(1,10) | FUNC(2) | PU)
#define GPB10_GPIO	(PIN(1,10) | FUNC(0) | PU)

/*
 * Group 1: GPIO 64...95
 * Used GPIO: 0...15
 * These pins can also act as GPIO inputs/outputs
 */
#define GPC0_LEND	(PIN(2,0) | FUNC(2) | PU)
#define GPC0_GPIO	(PIN(2,0) | FUNC(0) | PU)
#define GPC1_VCLK	(PIN(2,1) | FUNC(2) | PU)
#define GPC1_GPIO	(PIN(2,1) | FUNC(0) | PU)
#define GPC2_VLINE	(PIN(2,2) | FUNC(2) | PU)
#define GPC2_GPIO	(PIN(2,2) | FUNC(0) | PU)
#define GPC3_VFRAME	(PIN(2,3) | FUNC(2) | PU)
#define GPC3_GPIO	(PIN(2,3) | FUNC(0) | PU)
#define GPC4_VM		(PIN(2,4) | FUNC(2) | PU)
#define GPC4_GPIO	(PIN(2,4) | FUNC(0) | PU)
#define GPC5_LPCOE	(PIN(2,5) | FUNC(2) | PU)
#define GPC5_GPIO	(PIN(2,5) | FUNC(0) | PU)
#define GPC6_LPCREV	(PIN(2,6) | FUNC(2) | PU)
#define GPC6_GPIO	(PIN(2,6) | FUNC(0) | PU)
#define GPC7_LPCREVB	(PIN(2,7) | FUNC(2) | PU)
#define GPC7_GPIO	(PIN(2,7) | FUNC(0) | PU)
#define GPC8_VD0	(PIN(2,8) | FUNC(2) | PU)
#define GPC8_GPIO	(PIN(2,8) | FUNC(0) | PU)
#define GPC9_VD1	(PIN(2,9) | FUNC(2) | PU)
#define GPC9_GPIO	(PIN(2,9) | FUNC(0) | PU)
#define GPC10_VD2	(PIN(2,10) | FUNC(2) | PU)
#define GPC10_GPIO	(PIN(2,10) | FUNC(0) | PU)
#define GPC11_VD3	(PIN(2,11) | FUNC(2) | PU)
#define GPC11_GPIO	(PIN(2,11) | FUNC(0) | PU)
#define GPC12_VD4	(PIN(2,12) | FUNC(2) | PU)
#define GPC12_GPIO	(PIN(2,12) | FUNC(0) | PU)
#define GPC13_VD5	(PIN(2,13) | FUNC(2) | PU)
#define GPC13_GPIO	(PIN(2,13) | FUNC(0) | PU)
#define GPC14_VD6	(PIN(2,14) | FUNC(2) | PU)
#define GPC14_GPIO	(PIN(2,14) | FUNC(0) | PU)
#define GPC15_VD7	(PIN(2,15) | FUNC(2) | PU)
#define GPC15_GPIO	(PIN(2,15) | FUNC(0) | PU)

/*
 * Group 1: GPIO 96...127
 * Used GPIO: 0...15
 * These pins can also act as GPIO inputs/outputs
 */
#define GPD0_VD8	(PIN(3,0) | FUNC(2) | PU)
#define GPD0_GPIO	(PIN(3,0) | FUNC(0) | PU)
#define GPD1_VD9	(PIN(3,1) | FUNC(2) | PU)
#define GPD1_GPIO	(PIN(3,1) | FUNC(0) | PU)
#define GPD2_VD10	(PIN(3,2) | FUNC(2) | PU)
#define GPD2_GPIO	(PIN(3,2) | FUNC(0) | PU)
#define GPD3_VD11	(PIN(3,3) | FUNC(2) | PU)
#define GPD3_GPIO	(PIN(3,3) | FUNC(0) | PU)
#define GPD4_VD12	(PIN(3,4) | FUNC(2) | PU)
#define GPD4_GPIO	(PIN(3,4) | FUNC(0) | PU)
#define GPD5_VD13	(PIN(3,5) | FUNC(2) | PU)
#define GPD5_GPIO	(PIN(3,5) | FUNC(0) | PU)
#define GPD6_VD14	(PIN(3,6) | FUNC(2) | PU)
#define GPD6_GPIO	(PIN(3,6) | FUNC(0) | PU)
#define GPD7_VD15	(PIN(3,7) | FUNC(2) | PU)
#define GPD7_GPIO	(PIN(3,7) | FUNC(0) | PU)
#define GPD8_VD16	(PIN(3,8) | FUNC(2) | PU)
#define GPD8_GPIO	(PIN(3,8) | FUNC(0) | PU)
#ifdef CONFIG_CPU_S3C2440
# define GPD8_SPIMISO1	(PIN(3,8) | FUNC(3) | PU)
#endif
#define GPD9_VD17	(PIN(3,9) | FUNC(2) | PU)
#define GPD9_GPIO	(PIN(3,9) | FUNC(0) | PU)
#ifdef CONFIG_CPU_S3C2440
# define GPD9_SPIMOSI1	(PIN(3,9) | FUNC(3) | PU)
#endif
#define GPD10_VD18	(PIN(3,10) | FUNC(2) | PU)
#define GPD10_GPIO	(PIN(3,10) | FUNC(0) | PU)
#ifdef CONFIG_CPU_S3C2440
# define GPD10_SPICLK	(PIN(3,10) | FUNC(3) | PU)
#endif
#define GPD11_VD19	(PIN(3,11) | FUNC(2) | PU)
#define GPD11_GPIO	(PIN(3,11) | FUNC(0) | PU)
#define GPD12_VD20	(PIN(3,12) | FUNC(2) | PU)
#define GPD12_GPIO	(PIN(3,12) | FUNC(0) | PU)
#define GPD13_VD21	(PIN(3,13) | FUNC(2) | PU)
#define GPD13_GPIO	(PIN(3,13) | FUNC(0) | PU)
#define GPD14_VD22	(PIN(3,14) | FUNC(2) | PU)
#define GPD14_GPIO	(PIN(3,14) | FUNC(0) | PU)
#define GPD14_NSS1	(PIN(3,14) | FUNC(3) | PU)
#define GPD15_VD23	(PIN(3,15) | FUNC(2) | PU)
#define GPD15_GPIO	(PIN(3,15) | FUNC(0) | PU)
#define GPD15_NSS0	(PIN(3,15) | FUNC(3) | PU)

/*
 * Group 1: GPIO 128...159
 * Used GPIO: 0...15
 * These pins can also act as GPIO inputs/outputs
 */
#define GPE0_I2SLRCK	(PIN(4,0) | FUNC(2) | PU)
#ifdef CONFIG_CPU_S3C2440
# define GPE0_AC_SYNC	(PIN(4,0) | FUNC(3) | PU)
#endif
#define GPE0_GPIO	(PIN(4,0) | FUNC(0) | PU)
#define GPE1_I2SSCLK	(PIN(4,1) | FUNC(2) | PU)
#ifdef CONFIG_CPU_S3C2440
# define GPE1_AC_BIT_CLK (PIN(4,1) | FUNC(3) | PU)
#endif
#define GPE1_GPIO	(PIN(4,1) | FUNC(0) | PU)
#define GPE2_CDCLK	(PIN(4,2) | FUNC(2) | PU)
#ifdef CONFIG_CPU_S3C2440
# define GPE2_AC_NRESET	(PIN(4,2) | FUNC(3) | PU)
#endif
#define GPE2_GPIO	(PIN(4,2) | FUNC(0) | PU)
#define GPE3_I2SDI	(PIN(4,3) | FUNC(2) | PU)
#ifdef CONFIG_CPU_S3C2440
# define GPE3_AC_SDATA_IN (PIN(4,3) | FUNC(3) | PU)
#endif
#ifdef CONFIG_CPU_S3C2410
# define GPE_NSS0	(PIN(4,3) | FUNC(3) | PU)
#endif
#define GPE3_GPIO	(PIN(4,3) | FUNC(0) | PU)
#define GPE4_I2SDO	(PIN(4,4) | FUNC(2) | PU)
#ifdef CONFIG_CPU_S3C2440
# define GPE4_AC_SDATA_OUT (PIN(4,4) | FUNC(3) | PU)
#endif
#ifdef CONFIG_CPU_S3C2440
# define GPE4_I2SSDI	(PIN(4,4) | FUNC(3) | PU)
#endif
#define GPE4_GPIO	(PIN(4,4) | FUNC(0) | PU)
#define GPE5_SDCLK	(PIN(4,5) | FUNC(2) | PU)
#define GPE5_GPIO	(PIN(4,5) | FUNC(0) | PU)
#define GPE6_SDCMD	(PIN(4,6) | FUNC(2) | PU)
#define GPE6_GPIO	(PIN(4,6) | FUNC(0) | PU)
#define GPE7_SDDAT0	(PIN(4,7) | FUNC(2) | PU)
#define GPE7_GPIO	(PIN(4,7) | FUNC(0) | PU)
#define GPE8_SDDAT1	(PIN(4,8) | FUNC(2) | PU)
#define GPE8_GPIO	(PIN(4,8) | FUNC(0) | PU)
#define GPE9_SDDAT2	(PIN(4,9) | FUNC(2) | PU)
#define GPE9_GPIO	(PIN(4,9) | FUNC(0) | PU)
#define GPE10_SDDAT3	(PIN(4,10) | FUNC(2) | PU)
#define GPE10_GPIO	(PIN(4,10) | FUNC(0) | PU)
#define GPE11_SPIMISO0	(PIN(4,11) | FUNC(2) | PU)
#define GPE11_GPIO	(PIN(4,11) | FUNC(0) | PU)
#define GPE12_SPIMOSI0	(PIN(4,12) | FUNC(2) | PU)
#define GPE12_GPIO	(PIN(4,12) | FUNC(0) | PU)
#define GPE13_SPICLK0	(PIN(4,13) | FUNC(2) | PU)
#define GPE13_GPIO	(PIN(4,13) | FUNC(0) | PU)
#define GPE14_IICSCL	(PIN(4,14) | FUNC(2))	/* no pullup option */
#define GPE14_GPIO	(PIN(4,14) | FUNC(0))	/* no pullup option */
#define GPE15_IICSDA	(PIN(4,15) | FUNC(2))	/* no pullup option */
#define GPE15_GPIO	(PIN(4,15) | FUNC(0))	/* no pullup option */

/*
 * Group 1: GPIO 160...191
 * Used GPIO: 0...7
 * These pins can also act as GPIO inputs/outputs
 */
#define GPF0_EINT0	(PIN(5,0) | FUNC(2) | PU)
#define GPF0_GPIO	(PIN(5,0) | FUNC(0) | PU)
#define GPF1_EINT1	(PIN(5,1) | FUNC(2) | PU)
#define GPF1_GPIO	(PIN(5,1) | FUNC(0) | PU)
#define GPF2_EINT2	(PIN(5,2) | FUNC(2) | PU)
#define GPF2_GPIO	(PIN(5,2) | FUNC(0) | PU)
#define GPF3_EINT3	(PIN(5,3) | FUNC(2) | PU)
#define GPF3_GPIO	(PIN(5,3) | FUNC(0) | PU)
#define GPF4_EINT4	(PIN(5,4) | FUNC(2) | PU)
#define GPF4_GPIO	(PIN(5,4) | FUNC(0) | PU)
#define GPF5_EINT5	(PIN(5,5) | FUNC(2) | PU)
#define GPF5_GPIO	(PIN(5,5) | FUNC(0) | PU)
#define GPF6_EINT6	(PIN(5,6) | FUNC(2) | PU)
#define GPF6_GPIO	(PIN(5,6) | FUNC(0) | PU)
#define GPF7_EINT7	(PIN(5,7) | FUNC(2) | PU)
#define GPF7_GPIO	(PIN(5,7) | FUNC(0) | PU)

/*
 * Group 1: GPIO 192..223
 * Used GPIO: 0...15
 * These pins can also act as GPIO inputs/outputs
 */
#define GPG0_EINT8	(PIN(6,0) | FUNC(2) | PU)
#define GPG0_GPIO	(PIN(6,0) | FUNC(0) | PU)
#define GPG1_EINT9	(PIN(6,1) | FUNC(2) | PU)
#define GPG1_GPIO	(PIN(6,1) | FUNC(0) | PU)
#define GPG2_EINT10	(PIN(6,2) | FUNC(2) | PU)
#define GPG2_NSS0	(PIN(6,2) | FUNC(3) | PU)
#define GPG2_GPIO	(PIN(6,2) | FUNC(0) | PU)
#define GPG3_EINT11	(PIN(6,3) | FUNC(2) | PU)
#define GPG3_NSS1	(PIN(6,3) | FUNC(3) | PU)
#define GPG3_GPIO	(PIN(6,3) | FUNC(0) | PU)
#define GPG4_EINT12	(PIN(6,4) | FUNC(2) | PU)
#define GPG4_LCD_PWREN	(PIN(6,4) | FUNC(3) | PU)
#define GPG4_GPIO	(PIN(6,4) | FUNC(0) | PU)
#define GPG5_EINT13	(PIN(6,5) | FUNC(2) | PU)
#define GPG5_SPIMISO1	(PIN(6,5) | FUNC(3) | PU)
#define GPG5_GPIO	(PIN(6,5) | FUNC(0) | PU)
#define GPG6_EINT14	(PIN(6,6) | FUNC(2) | PU)
#define GPG6_SPIMOSI1	(PIN(6,6) | FUNC(3) | PU)
#define GPG6_GPIO	(PIN(6,6) | FUNC(0) | PU)
#define GPG7_EINT15	(PIN(6,7) | FUNC(2) | PU)
#define GPG7_SPICLK1	(PIN(6,7) | FUNC(3) | PU)
#define GPG7_GPIO	(PIN(6,7) | FUNC(0) | PU)
#define GPG8_EINT16	(PIN(6,8) | FUNC(2) | PU)
#define GPG8_GPIO	(PIN(6,8) | FUNC(0) | PU)
#define GPG9_EINT17	(PIN(6,9) | FUNC(2) | PU)
#ifdef CONFIG_CPU_S3C2440
# define GPG9_NRTS1	(PIN(6,9) | FUNC(3) | PU)
#endif
#define GPG9_GPIO	(PIN(6,9) | FUNC(0) | PU)
#define GPG10_EINT18	(PIN(6,10) | FUNC(2) | PU)
#ifdef CONFIG_CPU_S3C2440
# define GPG10_NCTS1	(PIN(6,10) | FUNC(3) | PU)
#endif
#define GPG10_GPIO	(PIN(6,10) | FUNC(0) | PU)
#define GPG11_EINT19	(PIN(6,11) | FUNC(2) | PU)
#define GPG11_TCLK	(PIN(6,11) | FUNC(3) | PU)
#define GPG11_GPIO	(PIN(6,11) | FUNC(0) | PU)
#define GPG12_EINT20	(PIN(6,12) | FUNC(2) | PU)
#ifdef CONFIG_CPU_S3C2410
# define GPG12_XMON	(PIN(6,12) | FUNC(3) | PU)
#endif
#define GPG12_GPIO	(PIN(6,12) | FUNC(0) | PU)
#define GPG13_EINT21	(PIN(6,13) | FUNC(2) | PU)
#ifdef CONFIG_CPU_S3C2410
# define GPG13_NXPON	(PIN(6,13) | FUNC(3) | PU)
#endif
#define GPG13_GPIO	(PIN(6,13) | FUNC(0) | PU)	/* must be input in NAND boot mode */
#define GPG14_EINT22	(PIN(6,14) | FUNC(2) | PU)
#ifdef CONFIG_CPU_S3C2410
# define GPG14_YMON	(PIN(6,14) | FUNC(3) | PU)
#endif
#define GPG14_GPIO	(PIN(6,14) | FUNC(0) | PU)	/* must be input in NAND boot mode */
#define GPG15_EINT23	(PIN(6,15) | FUNC(2) | PU)
#ifdef CONFIG_CPU_S3C2410
# define GPG15_YPON	(PIN(6,15) | FUNC(3) | PU)
#endif
#define GPG15_GPIO	(PIN(6,15) | FUNC(0) | PU)	/* must be input in NAND boot mode */

/*
 * Group 1: GPIO 224..255
 * Used GPIO: 0...15
 * These pins can also act as GPIO inputs/outputs
 */
#define GPH0_NCTS0	(PIN(7,0) | FUNC(2) | PU)
#define GPH0_GPIO	(PIN(7,0) | FUNC(0) | PU)
#define GPH1_NRTS0	(PIN(7,1) | FUNC(2) | PU)
#define GPH1_GPIO	(PIN(7,1) | FUNC(0) | PU)
#define GPH2_TXD0	(PIN(7,2) | FUNC(2) | PU)
#define GPH2_GPIO	(PIN(7,2) | FUNC(0) | PU)
#define GPH3_RXD0	(PIN(7,3) | FUNC(2) | PU)
#define GPH3_GPIO	(PIN(7,3) | FUNC(0) | PU)
#define GPH4_TXD1	(PIN(7,4) | FUNC(2) | PU)
#define GPH4_GPIO	(PIN(7,4) | FUNC(0) | PU)
#define GPH5_RXD1	(PIN(7,5) | FUNC(2) | PU)
#define GPH5_GPIO	(PIN(7,5) | FUNC(0) | PU)
#define GPH6_TXD2	(PIN(7,6) | FUNC(2) | PU)
#define GPH6_NRTS1	(PIN(7,6) | FUNC(3) | PU)
#define GPH6_GPIO	(PIN(7,6) | FUNC(0) | PU)
#define GPH7_RXD2	(PIN(7,7) | FUNC(2) | PU)
#define GPH7_NCTS1	(PIN(7,7) | FUNC(3) | PU)
#define GPH7_GPIO	(PIN(7,7) | FUNC(0) | PU)
#define GPH8_UEXTCLK	(PIN(7,8) | FUNC(2) | PU)
#define GPH8_GPIO	(PIN(7,8) | FUNC(0) | PU)
#define GPH9_CLOCKOUT0	(PIN(7,9) | FUNC(2) | PU)
#define GPH9_GPIO	(PIN(7,9) | FUNC(0) | PU)
#define GPH10_CLKOUT1	(PIN(7,10) | FUNC(2) | PU)
#define GPH10_GPIO	(PIN(7,10) | FUNC(0) | PU)

#ifdef CONFIG_CPU_S3C2440
/*
 * Group 1: GPIO 256..287
 * Used GPIO: 0...12
 * These pins can also act as GPIO inputs/outputs
 */
#define GPJ0_CAMDATA0	(PIN(8,0) | FUNC(2) | PU)
#define GPJ0_GPIO	(PIN(8,0) | FUNC(0) | PU)
#define GPJ1_CAMDATA1	(PIN(8,1) | FUNC(2) | PU)
#define GPJ1_GPIO	(PIN(8,1) | FUNC(0) | PU)
#define GPJ2_CAMDATA2	(PIN(8,2) | FUNC(2) | PU)
#define GPJ2_GPIO	(PIN(8,2) | FUNC(0) | PU)
#define GPJ3_CAMDATA3	(PIN(8,3) | FUNC(2) | PU)
#define GPJ3_GPIO	(PIN(8,3) | FUNC(0) | PU)
#define GPJ4_CAMDATA4	(PIN(8,4) | FUNC(2) | PU)
#define GPJ4_GPIO	(PIN(8,4) | FUNC(0) | PU)
#define GPJ5_CAMDATA5	(PIN(8,5) | FUNC(2) | PU)
#define GPJ5_GPIO	(PIN(8,5) | FUNC(0) | PU)
#define GPJ6_CAMDATA6	(PIN(8,6) | FUNC(2) | PU)
#define GPJ6_GPIO	(PIN(8,6) | FUNC(0) | PU)
#define GPJ7_CAMDATA7	(PIN(8,7) | FUNC(2) | PU)
#define GPJ7_GPIO	(PIN(8,7) | FUNC(0) | PU)
#define GPJ8_CAMPCLK	(PIN(8,8) | FUNC(2) | PU)
#define GPJ8_GPIO	(PIN(8,8) | FUNC(0) | PU)
#define GPJ9_CAMVSYNC	(PIN(8,9) | FUNC(2) | PU)
#define GPJ9_GPIO	(PIN(8,9) | FUNC(0) | PU)
#define GPJ10_CAMHREF	(PIN(8,10) | FUNC(2) | PU)
#define GPJ10_GPIO	(PIN(8,10) | FUNC(0) | PU)
#define GPJ11_CAMCLKOUT	(PIN(8,11) | FUNC(2) | PU)
#define GPJ11_GPIO	(PIN(8,11) | FUNC(0) | PU)
#define GPJ12_CAMRESET	(PIN(8,12) | FUNC(0) | PU)
#define GPJ12_GPIO	(PIN(8,12) | FUNC(0) | PU)

#endif

#endif /* __MACH_IOMUX_S3C24x0_H */
