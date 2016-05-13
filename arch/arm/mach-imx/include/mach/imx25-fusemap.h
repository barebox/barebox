#ifndef __MACH_IMX25_FUSEMAP_H
#define __MACH_IMX25_FUSEMAP_H

#include <mach/iim.h>

/* Fuse bank write protect */
#define IMX25_IIM_FBWP(bank)		(IIM_BANK(bank) | IIM_BYTE(0) | IIM_BIT(7))
/* Fuse Bank Override Protect */
#define IMX25_IIM_FBOP(bank)		(IIM_BANK(bank) | IIM_BYTE(0) | IIM_BIT(6))
/* Fuse Bank Read Protect */
#define IMX25_IIM_FBRP(bank)		(IIM_BANK(bank) | IIM_BYTE(0) | IIM_BIT(5))
/* Tester fuses. Burnt on the tester at the end of the wafer sort, locks bank0, rows 001C-003C */
#define IMX25_IIM_TESTER_LOCK		(IIM_BANK(0) | IIM_BYTE(0) | IIM_BIT(4))
/* Fuse Banks Explicit Sense Protect */
#define IMX25_IIM_FBESP(bank)		(IIM_BANK(bank) | IIM_BYTE(0) | IIM_BIT(3))
/* Locking row 0068-007C, fusebank0 */
#define IMX25_IIM_MAC_ADDR_LOCK		(IIM_BANK(0) | IIM_BYTE(0) | IIM_BIT(2))
/* Locking rows 0008 0054-0064, fusebank0 */
#define IMX25_IIM_TRIM_LOCK		(IIM_BANK(0) | IIM_BYTE(0) | IIM_BIT(1))
/* Locking rows 0004, 000C-0018, 0040-0044, fusebank0 */
#define IMX25_IIM_BOOT_LOCK		(IIM_BANK(0) | IIM_BYTE(0) | IIM_BIT(0))
/* Disabling the Secure JTAG Controller module clock */
#define IMX25_IIM_SJC_DISABLE		(IIM_BANK(0) | IIM_BYTE(4) | IIM_BIT(7))

/* Controls the security mode of the JTAG debug interface */
#define IMX25_IIM_JTAG_SMODE		(IIM_BANK(0) | IIM_BYTE(4) | IIM_BIT(5) | IIM_WIDTH(2))

/* Disable SCC debug through SJC */
#define IMX25_IIM_JTAG_SCC		(IIM_BANK(0) | IIM_BYTE(4) | IIM_BIT(4))

/* JTAG HAB Enable Override (1 = HAB may not enable JTAG debug access */
#define IMX25_IIM_JTAG_HEO		(IIM_BANK(0) | IIM_BYTE(4) | IIM_BIT(3))

/* Secure JTAG Re-enable.
 * 0 Secure JTAG Bypass fuse is not overridden (secure JTAG bypass is allowed)
 * 1 Secure JTAG Bypass fuse is overridden (secure JTAG bypass is not allowed)
 */
#define IMX25_IIM_SEC_JTAG_RE		(IIM_BANK(0) | IIM_BYTE(4) | IIM_BIT(1))

/* JTAG Debug Security Bypass.
 * 0 JTAG Security bypass is not active
 * 1 JTAG Security bypass is active
 */
#define IMX25_IIM_JTAG_BP		(IIM_BANK(0) | IIM_BYTE(4) | IIM_BIT(0))

/*  High Temperature Detect Configuration. A field in DryIce Analog Configuration Register (DACR) */
#define IMX25_IIM_HTDC			(IIM_BANK(0) | IIM_BYTE(8) | IIM_BIT(3) | IIM_WIDTH(3))

/*  Low Temperature Detect Configuration. A field in DryIce Analog Configuration Register (DACR) */
#define IMX25_IIM_LTDC			(IIM_BANK(0) | IIM_BYTE(8) | IIM_BIT(0) | IIM_WIDTH(3))

 /*  Choosing the specific eSDHC, CSPI or I2C controller for booting from. */
#define IMX25_IIM_BT_SRC		(IIM_BANK(0) | IIM_BYTE(0xc) | IIM_BIT(6) | IIM_WIDTH(2))

/* SLC/MLC NAND device. (Former BT_ECC_SEL fuse) Also used as a fast boot mode indication for eMMC 4.3 protocol.
 *	If the bootable device is NAND then
 *		0 SLC NAND device
 *		1 MLC NAND device
 *	If the bootable device is MMC then
 *		0 Do not use eMMC fast boot mode
 *		1 Use eMMC fast boot mode
 */
#define IMX25_IIM_BT_MLC_SEL		(IIM_BANK(0) | IIM_BYTE(0xc) | IIM_BIT(5))

/* Specifies the size of spare bytes for 4KB page size NAND Flash devices.
 *	If the bootable device is NAND then
 *		0 128 bytes spare (Samsung) (4-IIM_BIT ECC)
 *		1 218 bytes spare (Micron, Toshiba) (8-IIM_BIT ECC)
 *	If the bootable device is SD then
 *		0 ‘FAST_BOOT’ IIM_BIT 29 in ACMD41 argument is 0
 *		1 ‘FAST_BOOT’ IIM_BIT 29 in ACMD41 argument is 1
 */
#define IMX25_IIM_BT_SPARE_SIZE		(IIM_BANK(0) | IIM_BYTE(0xc) | IIM_BIT(2))

 /* Bypassing a pullup on D+ line in case of LPB.
  *	1 No pullup on D+ line.
  */
#define IMX25_IIM_BT_DPLUS_BYPASS	(IIM_BANK(0) | IIM_BYTE(0xc) | IIM_BIT(1))

/* USB boot source selection. Has a corresponding GPIO pin.
 *	0 USB OTG Internal PHY (UTMI)
 *	1 USB OTG External PHY (ULPI)
 */
#define IMX25_IIM_BT_USB_SRC		(IIM_BANK(0) | IIM_BYTE(0xc) | IIM_BIT(0))

/* NAND Flash Page Size.
 *	If BT_MEM_CTL = NAND Flash, then
 *		00 512 bytes
 *		01 2K bytes
 *		10 4K bytes
 *		11 Reserved
 */
#define IMX25_IIM_BT_PAGE_SIZE		(IIM_BANK(0) | IIM_BYTE(0x10) | IIM_BIT(5) | IIM_WIDTH(2))

/* Selects whether EEPROM device is used for load of configuration DCD data
 *	0 Use EEPROM DCD
 *	1 Do not use EEPROM DCD
 */
#define IMX25_IIM_BT_EEPROM_CFG		(IIM_BANK(0) | IIM_BYTE(0x10) | IIM_BIT(4))

/*  GPIO Boot Select. Determines whether certain boot fuse values are controlled from GPIO pins or IIM.
 *	0 The fuse values are determined by GPIO pins
 *	1 The fuse values are determined by fuses
 */
#define IMX25_IIM_GPIO_BT_SEL		(IIM_BANK(0) | IIM_BYTE(0x10) | IIM_BIT(3))

/* Security Type.
 *	001 Engineering (allows any code to be flashed and executed, even if does not have a valid signature)
 *	100 Security Disabled (forinternal/testing use)
 *	Others Production (Security On)
 */
#define IMX25_IIM_HAB_TYPE		(IIM_BANK(0) | IIM_BYTE(0x10) | IIM_BIT(0) | IIM_WIDTH(3))

/* Boot Memory Type.
 *	If BT_MEM_CTL = WEIM, then
 *		00 NOR
 *		01 Reserved
 *		10 OneNand
 *		11 Reserved
 *	If BT_MEM_CTL = NAND Flash
 *		00 3 address cycles
 *		01 4 address cycles
 *		10 5 address cycles
 *		11 Reserved
 *	If BT_MEM_CTL = Expansion Card Device
 *		00 SD/MMC/MoviNAND HDD
 *		01 Reserved
 *		10 Serial ROM via I2C
 *		11 Serial ROM via SPI
 */
#define IMX25_IIM_BT_MEM_TYPE		(IIM_BANK(0) | IIM_BYTE(0x14) | IIM_BIT(5) | IIM_WIDTH(2))

/* Bus IIM_WIDTH and muxed/unmuxed interface. Has a corresponding GPIO pin.
 *	If BT_MEM_CTL=NAND then
 *		00 8 IIM_BIT bus,
 *		01 16 IIM_BIT bus
 *		1x Reserved
 *	If BT_MEM_CTL=WEIM then
 *		00 16 IIM_BIT addr/data muxed
 *		01 16 IIM_BIT addr/data unmuxed
 *		1x Reserved
 *	If BT_MEM_CTL=SPI then
 *		00 2-addr word SPI (16-IIM_BIT)
 *		01 3-addr word SPI (24-IIM_BIT)
 *		1x Reserved
 */
#define IMX25_IIM_BT_BUS_WIDTH		(IIM_BANK(0) | IIM_BYTE(0x14) | IIM_BIT(3) | IIM_WIDTH(2))

/* Boot Memory Control Type. (memory device)
 *	00 WEIM
 *	01 NAND Flash
 *	10 ATA HDD
 *	11 Expansion Device
 *	(SD/MMC, support high storage, EEPROMs. See BT_MEM_TYPE[1:0] settings for details.
 */
#define IMX25_IIM_BT_MEM_CTL		(IIM_BANK(0) | IIM_BYTE(0x14) | IIM_BIT(1) | IIM_WIDTH(2))

/* Direct External Memory Boot Disable.
 *	0 Direct boot from external memory is allowed
 *	1 Direct boot from external memory is not allowed
 */
#define IMX25_IIM_DIR_BT_DIS		(IIM_BANK(0) | IIM_BYTE(0x14) | IIM_BIT(0))

/* HAB Customer Code. Select customer code as input to HAB. */
#define IMX25_IIM_HAB_CUS		(IIM_BANK(0) | IIM_BYTE(0x18) | IIM_BIT(0) | IIM_WIDTH(8))

/* Silicon revision number. 0 Rev1.0 1 Rev1.1 */
#define IMX25_IIM_SI_REV		(IIM_BANK(0) | IIM_BYTE(0x1c) | IIM_BIT(0) | IIM_WIDTH(8))

/* 64-IIM_BIT Unique ID. 0 <= n <= 7 */
#define IMX25_IIM_UID(n)		(IIM_BANK(0) | IIM_BYTE(0x20 + 0x4 * (n)) | IIM_BIT(0) | IIM_WIDTH(8))

/* LPB ARM core frequency. Has a corresponding GPIO pin.
 *	000 133 MHz (Default)
 *	001 24MHz
 *	010 55.33 MHz
 *	011 66 MHz
 *	100 83 MHz
 *	101 166 MHz
 *	110 266 MHz
 *	111 Normal boot frequency
 */
#define IMX25_IIM_BT_LPB_FREQ		(IIM_BANK(0) | IIM_BYTE(0x44) | IIM_BIT(5) | IIM_WIDTH(3))

/* Choosing the specific UART controller for booting from. */
#define IMX25_IIM_BT_UART_SRC		(IIM_BANK(0) | IIM_BYTE(0x44) | IIM_BIT(2) | IIM_WIDTH(3))

/* Options for Low Power Boot mode.
 *	00 LPB disabled
 *	01 Generic PMIC and one GPIO input (Low battery)
 *	10 Generic PMIC and two GPIO inputs (Low battery and Charger detect)
 *	11 Atlas AP Power Management IC.
 */
#define IMX25_IIM_BT_LPB		(IIM_BANK(0) | IIM_BYTE(0x44) | IIM_BIT(0) | IIM_WIDTH(2))

/* Application Processor Boot Image Version. */
#define IMX25_IIM_AP_BI_VER_15_8	(IIM_BANK(0) | IIM_BYTE(0x48) | IIM_BIT(0) | IIM_WIDTH(8))

/* Application Processor Boot Image Version. */
#define IMX25_IIM_AP_BI_VER_7_0		(IIM_BANK(0) | IIM_BYTE(0x4c) | IIM_BIT(0) | IIM_WIDTH(8))

/* Most significant IIM_BYTE of 256-IIM_BIT hash value of AP super root key (SRK0_HASH) */
#define IMX25_IIM_SRK0_HASH_0		(IIM_BANK(0) | IIM_BYTE(0x50) | IIM_BIT(0) | IIM_WIDTH(8))

/* For SPC statistics during production. */
#define IMX25_IIM_STORE_COUNT		(IIM_BANK(0) | IIM_BYTE(0x54) | IIM_BIT(0) | IIM_WIDTH(8))

/* Use for adjustment the compensator delays on silicon and the system works as a whole at 1.0V and 1.2V (DVFS) */
#define IMX25_IIM_DVFS_DELAY_ADJUST	(IIM_BANK(0) | IIM_BYTE(0x58) | IIM_BIT(0) | IIM_WIDTH(8))

/* PTC version control number. */
#define IMX25_IIM_PTC_VER		(IIM_BANK(0) | IIM_BYTE(0x5c) | IIM_BIT(5) | IIM_WIDTH(3))

#define IMX25_IIM_GDPTCV_VALID		(IIM_BANK(0) | IIM_BYTE(0x5c) | IIM_BIT(4))

/* GP domain DPTC/SPC Test Voltage. */
#define IMX25_IIM_GDPTCV		(IIM_BANK(0) | IIM_BYTE(0x5c) | IIM_BIT(0) | IIM_WIDTH(4))

/* Voltage Reference Configuration. A field in DryIce Analog Configuration Register (DACR) */
#define IMX25_IIM_VRC			(IIM_BANK(0) | IIM_BYTE(0x60) | IIM_BIT(5) | IIM_WIDTH(3))

#define IMX25_IIM_LDPTCV_VALID		(IIM_BANK(0) | IIM_BYTE(0x60) | IIM_BIT(4))

/* LP domain DPTC Test Voltage. */
#define IMX25_IIM_LDPTCV		(IIM_BANK(0) | IIM_BYTE(0x60) | IIM_BIT(0) | IIM_WIDTH(4))

/* Well Bias Charge Pump Frequency Adjust. Adjusting the frequency of the internal free-running oscillator. */
#define IMX25_IIM_CPFA			(IIM_BANK(0) | IIM_BYTE(0x64) | IIM_BIT(4))

/* Well Bias Charge Pump Set Point Adjustment. */
#define IMX25_IIM_CPSPA			(IIM_BANK(0) | IIM_BYTE(0x64) | IIM_BIT(0) | IIM_WIDTH(4))

/* Ethernet MAC Address, 0 <= n <= 5 */
#define IMX25_IIM_MAC_ADDR(n)		(IIM_BANK(1) | IIM_BYTE(0x68 + 0x4 * (n)) | IIM_BIT(0) | IIM_WIDTH(8))

/* Locking row 0058, fusebank 1 */
#define IMX25_IIM_USR5_LOCK		(IIM_BANK(1) | IIM_BYTE(0) | IIM_BIT(4))

/* Lock for rows 0078–007C of fusebank 1 */
#define IMX25_IIM_USR6_LOCK		(IIM_BANK(1) | IIM_BYTE(0) | IIM_BIT(2))

/* Locking 0008-0020, fusebank1 */
#define IMX25_IIM_SJC_RESP_LOCK		(IIM_BANK(1) | IIM_BYTE(0) | IIM_BIT(1))

/* Locking SCC_KEY[255:0] */
#define IMX25_IIM_SCC_LOCK		(IIM_BANK(1) | IIM_BYTE(0) | IIM_BIT(0))

/* SCC Secret Key, 0 <= n <= 20 */
#define IMX25_IIM_SCC_KEY(n)		(IIM_BANK(1) | IIM_BYTE(0x4 + 0x4 * (n)) | IIM_BIT(0) | IIM_WIDTH(8))

/* Fuses available for software/customers */
#define IMX25_IIM_USR5			(IIM_BANK(1) | IIM_BYTE(0x58) | IIM_BIT(0) | IIM_WIDTH(8))

/* Response reference value for the secure JTAG controller, 0 <= n <= 7 */
#define IMX25_IIM_SJC_RESP(n)		(IIM_BANK(1) | IIM_BYTE(0x5c + 0x4 * (n)) | IIM_BIT(0) | IIM_WIDTH(8))

/* Fuses available for software/customers. 0 <= n <= 1 */
#define IMX25_IIM_USR6(n)		(IIM_BANK(1) | IIM_BYTE(0x78 + 0x4 * (n)) | IIM_BIT(0) | IIM_WIDTH(8))

/* Lock for SRK_HASH[255:160] fuses in row 0x0050, fusebank0 and in rows 0x0004-0x002C, fusebank3 */
#define IMX25_IIM_SRK0_LOCK96		(IIM_BANK(2) | IIM_BYTE(0) | IIM_BIT(1))

/* Lock for SRK0_HASH[159:0] fuses in rows 0x0030-0x007C */
#define IMX25_IIM_SRK0_LOCK160		(IIM_BANK(2) | IIM_BYTE(0) | IIM_BIT(0))

/* AP Super Root Key hash, bits [247:0].
 * Most significant IIM_BYTE SRK_HASH[255:248] is in the fuse bank #0, 0050
 * 1 <= n <= 31
 */
#define IMX25_IIM_SRK0_HASH_1_31(n)	(IIM_BANK(2) | IIM_BYTE(0x4 * (n)) | IIM_BIT(0) | IIM_WIDTH(8))

#endif /* __MACH_IMX25_FUSEMAP_H */
