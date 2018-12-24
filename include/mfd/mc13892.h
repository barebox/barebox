/*
 * (C) Copyright 2010
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de.
 *
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */


#ifndef __MC13892_H__
#define __MC13892_H__

/* REG_CHARGE */

#define MC13782_CHARGE_VCHRG_3800	(0 << 0)
#define MC13782_CHARGE_VCHRG_4100	(1 << 0)
#define MC13782_CHARGE_VCHRG_4150	(2 << 0)
#define MC13782_CHARGE_VCHRG_4200	(3 << 0)
#define MC13782_CHARGE_VCHRG_4250	(4 << 0)
#define MC13782_CHARGE_VCHRG_4300	(5 << 0)
#define MC13782_CHARGE_VCHRG_4375	(6 << 0)
#define MC13782_CHARGE_VCHRG_4450	(7 << 0)
#define MC13782_CHARGE_VCHRG_MASK	(7 << 0)
#define MC13782_CHARGE_ICHRG_0		(0 << 3)
#define MC13782_CHARGE_ICHRG_80		(1 << 3)
#define MC13782_CHARGE_ICHRG_240	(2 << 3)
#define MC13782_CHARGE_ICHRG_320	(3 << 3)
#define MC13782_CHARGE_ICHRG_400	(4 << 3)
#define MC13782_CHARGE_ICHRG_480	(5 << 3)
#define MC13782_CHARGE_ICHRG_560	(6 << 3)
#define MC13782_CHARGE_ICHRG_640	(7 << 3)
#define MC13782_CHARGE_ICHRG_720	(8 << 3)
#define MC13782_CHARGE_ICHRG_800	(9 << 3)
#define MC13782_CHARGE_ICHRG_880	(10 << 3)
#define MC13782_CHARGE_ICHRG_960	(11 << 3)
#define MC13782_CHARGE_ICHRG_1040	(12 << 3)
#define MC13782_CHARGE_ICHRG_1200	(13 << 3)
#define MC13782_CHARGE_ICHRG_1600	(14 << 3)
#define MC13782_CHARGE_ICHRG_FULL	(15 << 3)
#define MC13782_CHARGE_ICHRG_MASK	(15 << 3)
#define MC13782_CHARGE_TREN		(1 << 7)
#define MC13782_CHARGE_ACKLPB		(1 << 8)
#define MC13782_CHARGE_THCHKB		(1 << 9)
#define MC13782_CHARGE_FETOVRD		(1 << 10)
#define MC13782_CHARGE_FETCTRL		(1 << 11)
#define MC13782_CHARGE_RVRSMODE		(1 << 13)
#define MC13782_CHARGE_PLIM_600		(0 << 15)
#define MC13782_CHARGE_PLIM_800		(1 << 15)
#define MC13782_CHARGE_PLIM_1000	(2 << 15)
#define MC13782_CHARGE_PLIM_1200	(3 << 15)
#define MC13782_CHARGE_PLIM_MASK	(3 << 15)
#define MC13782_CHARGE_PLIMDIS		(1 << 17)
#define MC13782_CHARGE_CHRGLEDEN	(1 << 18)
#define MC13782_CHARGE_CHGTMRRST	(1 << 19)
#define MC13782_CHARGE_CHGRESTART	(1 << 20)
#define MC13782_CHARGE_CHGAUTOB		(1 << 21)
#define MC13782_CHARGE_CYCLB		(1 << 22)
#define MC13782_CHARGE_CHGAUTOVIB	(1 << 23)

/* SWxMode for Normal/Standby Mode */
#define MC13892_SWMODE_OFF_OFF		0
#define MC13892_SWMODE_PWM_OFF		1
#define MC13892_SWMODE_PWMPS_OFF	2
#define MC13892_SWMODE_PFM_OFF		3
#define MC13892_SWMODE_AUTO_OFF		4
#define MC13892_SWMODE_PWM_PWM		5
#define MC13892_SWMODE_PWM_AUTO		6
#define MC13892_SWMODE_AUTO_AUTO	8
#define MC13892_SWMODE_PWM_PWMPS	9
#define MC13892_SWMODE_PWMS_PWMPS	10
#define MC13892_SWMODE_PWMS_AUTO	11
#define MC13892_SWMODE_AUTO_PFM		12
#define MC13892_SWMODE_PWM_PFM		13
#define MC13892_SWMODE_PWMS_PFM		14
#define MC13892_SWMODE_PFM_PFM		15
#define MC13892_SWMODE_MASK		0x0F

#define MC13892_SWMODE1_SHIFT		0
#define MC13892_SWMODE2_SHIFT		10
#define MC13892_SWMODE3_SHIFT		0
#define MC13892_SWMODE4_SHIFT		8

/* Fields in REG_SETTING_1 */
#define MC13892_SETTING_1_VVIDEO_2_7	(0 << 2)
#define MC13892_SETTING_1_VVIDEO_2_775	(1 << 2)
#define MC13892_SETTING_1_VVIDEO_2_5	(2 << 2)
#define MC13892_SETTING_1_VVIDEO_2_6	(3 << 2)
#define MC13892_SETTING_1_VVIDEO_MASK	(3 << 2)
#define MC13892_SETTING_1_VAUDIO_2_3	(0 << 4)
#define MC13892_SETTING_1_VAUDIO_2_5	(1 << 4)
#define MC13892_SETTING_1_VAUDIO_2_775	(2 << 4)
#define MC13892_SETTING_1_VAUDIO_3_0	(3 << 4)
#define MC13892_SETTING_1_VAUDIO_MASK	(3 << 4)
#define MC13892_SETTING_1_VSD_1_8	(0 << 6)
#define MC13892_SETTING_1_VSD_2_0	(1 << 6)
#define MC13892_SETTING_1_VSD_2_6	(2 << 6)
#define MC13892_SETTING_1_VSD_2_7	(3 << 6)
#define MC13892_SETTING_1_VSD_2_8	(4 << 6)
#define MC13892_SETTING_1_VSD_2_9	(5 << 6)
#define MC13892_SETTING_1_VSD_3_0	(6 << 6)
#define MC13892_SETTING_1_VSD_3_15	(7 << 6)
#define MC13892_SETTING_1_VSD_MASK	(7 << 6)

/* Fields in REG_SETTING_0 */
#define MC13892_SETTING_0_VGEN1_1_2	(0 << 0)
#define MC13892_SETTING_0_VGEN1_1_5	(1 << 0)
#define MC13892_SETTING_0_VGEN1_2_775	(2 << 0)
#define MC13892_SETTING_0_VGEN1_3_15	(3 << 0)
#define MC13892_SETTING_0_VGEN1_MASK	(3 << 0)
#define MC13892_SETTING_0_VGEN2_1_2	(0 << 6)
#define MC13892_SETTING_0_VGEN2_1_5	(1 << 6)
#define MC13892_SETTING_0_VGEN2_1_6	(2 << 6)
#define MC13892_SETTING_0_VGEN2_1_8	(3 << 6)
#define MC13892_SETTING_0_VGEN2_2_7	(4 << 6)
#define MC13892_SETTING_0_VGEN2_2_8	(5 << 6)
#define MC13892_SETTING_0_VGEN2_3_0	(6 << 6)
#define MC13892_SETTING_0_VGEN2_3_15	(7 << 6)
#define MC13892_SETTING_0_VGEN2_MASK	(7 << 6)
#define MC13892_SETTING_0_VGEN3_1_8	(0 << 14)
#define MC13892_SETTING_0_VGEN3_2_9	(1 << 14)
#define MC13892_SETTING_0_VGEN3_MASK	(1 << 14)
#define MC13892_SETTING_0_VDIG_1_05	(0 << 4)
#define MC13892_SETTING_0_VDIG_1_25	(1 << 4)
#define MC13892_SETTING_0_VDIG_1_65	(2 << 4)
#define MC13892_SETTING_0_VDIG_1_8	(3 << 4)
#define MC13892_SETTING_0_VDIG_MASK	(3 << 4)
#define MC13892_SETTING_0_VCAM_2_5	(0 << 16)
#define MC13892_SETTING_0_VCAM_2_6	(1 << 16)
#define MC13892_SETTING_0_VCAM_2_75	(2 << 16)
#define MC13892_SETTING_0_VCAM_3_0	(3 << 16)
#define MC13892_SETTING_0_VCAM_MASK	(3 << 16)

/* Reg Mode 0 */
#define MC13892_MODE_0_VGEN1EN		(1 << 0)
#define MC13892_MODE_0_VGEN1STBY	(1 << 1)
#define MC13892_MODE_0_VGEN1MODE	(1 << 2)
#define MC13892_MODE_0_VIOHIEN		(1 << 3)
#define MC13892_MODE_0_VIOHISTBY	(1 << 4)
#define MC13892_MODE_0_VDIGEN		(1 << 9)
#define MC13892_MODE_0_VDIGSTBY		(1 << 10)
#define MC13892_MODE_0_VGEN2EN		(1 << 12)
#define MC13892_MODE_0_VGEN2STBY	(1 << 13)
#define MC13892_MODE_0_VGEN2MODE	(1 << 14)
#define MC13892_MODE_0_VPLLEN		(1 << 15)
#define MC13892_MODE_0_VPLLSTBY		(1 << 16)
#define MC13892_MODE_0_VUSBEN		(1 << 18)
#define MC13892_MODE_0_VUSBSTBY		(1 << 19)

/* Reg Mode 1 */
#define MC13892_MODE_1_VGEN3EN		(1 << 0)
#define MC13892_MODE_1_VGEN3STBY	(1 << 1)
#define MC13892_MODE_1_VGEN3MODE	(1 << 2)
#define MC13892_MODE_1_VGEN3CONFIG	(1 << 3)
#define MC13892_MODE_1_VCAMEN		(1 << 6)
#define MC13892_MODE_1_VCAMSTBY		(1 << 7)
#define MC13892_MODE_1_VCAMMODE		(1 << 8)
#define MC13892_MODE_1_VCAMCONFIG	(1 << 9)
#define MC13892_MODE_1_VVIDEOEN		(1 << 12)
#define MC13892_MODE_1_VIDEOSTBY	(1 << 13)
#define MC13892_MODE_1_VVIDEOMODE	(1 << 14)
#define MC13892_MODE_1_VAUDIOEN		(1 << 15)
#define MC13892_MODE_1_VAUDIOSTBY	(1 << 16)
#define MC13892_MODE_1_VSDEN		(1 << 18)
#define MC13892_MODE_1_VSDSTBY		(1 << 19)
#define MC13892_MODE_1_VSDMODE		(1 << 20)

/* Reg Power misc */
#define MC13892_POWER_MISC_REGSCPEN	(1 << 0)
#define MC13892_POWER_MISC_GPO1EN	(1 << 6)
#define MC13892_POWER_MISC_GPO1STBY	(1 << 7)
#define MC13892_POWER_MISC_GPO2EN	(1 << 8)
#define MC13892_POWER_MISC_GPO2STBY	(1 << 9)
#define MC13892_POWER_MISC_GPO3EN	(1 << 10)
#define MC13892_POWER_MISC_GPO3STBY	(1 << 11)
#define MC13892_POWER_MISC_GPO4EN	(1 << 12)
#define MC13892_POWER_MISC_GPO4STBY	(1 << 13)
#define MC13892_POWER_MISC_PWGT1SPIEN	(1 << 15)
#define MC13892_POWER_MISC_PWGT2SPIEN	(1 << 16)
#define MC13892_POWER_MISC_GPO4ADIN	(1 << 21)

/* Reg Power Control 2*/
#define MC13892_POWER_CONTROL_2_WDIRESET	(1 << 12)

/* Buck Switchers (SW1,2,3,4) Output Voltage */
/*
 * NOTE: These values are for SWxHI = 0,
 * SWxHI = 1 adds 0.5V to the desired voltage
 */
#define MC13892_SWx_0_600V	0
#define MC13892_SWx_SWx_0_625V	1
#define MC13892_SWx_SWx_0_650V	2
#define MC13892_SWx_SWx_0_675V	3
#define MC13892_SWx_SWx_0_700V	4
#define MC13892_SWx_SWx_0_725V	5
#define MC13892_SWx_SWx_0_750V	6
#define MC13892_SWx_SWx_0_775V	7
#define MC13892_SWx_SWx_0_800V	8
#define MC13892_SWx_SWx_0_825V	9
#define MC13892_SWx_SWx_0_850V	10
#define MC13892_SWx_SWx_0_875V	11
#define MC13892_SWx_SWx_0_900V	12
#define MC13892_SWx_SWx_0_925V	13
#define MC13892_SWx_SWx_0_950V	14
#define MC13892_SWx_SWx_0_975V	15
#define MC13892_SWx_SWx_1_000V	16
#define MC13892_SWx_SWx_1_025V	17
#define MC13892_SWx_SWx_1_050V	18
#define MC13892_SWx_SWx_1_075V	19
#define MC13892_SWx_SWx_1_100V	20
#define MC13892_SWx_SWx_1_125V	21
#define MC13892_SWx_SWx_1_150V	22
#define MC13892_SWx_SWx_1_175V	23
#define MC13892_SWx_SWx_1_200V	24
#define MC13892_SWx_SWx_1_225V	25
#define MC13892_SWx_SWx_1_250V	26
#define MC13892_SWx_SWx_1_275V	27
#define MC13892_SWx_SWx_1_300V	28
#define MC13892_SWx_SWx_1_325V	29
#define MC13892_SWx_SWx_1_350V	30
#define MC13892_SWx_SWx_1_375V	31
#define MC13892_SWx_SWx_VOLT_MASK	0x1F

#endif
