/*
 * Copyright (C) 2010 Marek Belisko <marek.belisko@open-nandra.com>
 *
 * Based on a9m2440.c board init by Juergen Beisert, Pengutronix
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
 *
 */

/**
 * @file
 * @brief mini2440 Specific Board Initialization routines
 *
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <dm9000.h>
#include <nand.h>
#include <mci.h>
#include <fb.h>
#include <asm/armlinux.h>
#include <asm/sections.h>
#include <io.h>
#include <gpio.h>
#include <mach/iomux.h>
#include <mach/s3c-iomap.h>
#include <mach/devices-s3c24xx.h>
#include <mach/s3c24xx-nand.h>
#include <mach/s3c-generic.h>
#include <mach/s3c-mci.h>
#include <mach/s3c24xx-fb.h>
#include <mach/s3c-busctl.h>
#include <mach/s3c24xx-gpio.h>

static struct s3c24x0_nand_platform_data nand_info = {
	.nand_timing = CALC_NFCONF_TIMING(MINI2440_TACLS, MINI2440_TWRPH0,
							MINI2440_TWRPH1),
	.flash_bbt = 1,	/* same as the kernel */
};

/*
 * dm9000 network controller onboard
 * Connected to CS line 4 and interrupt line EINT7,
 * data width is 16 bit
 * Area 1: Offset 0x300...0x303
 * Area 2: Offset 0x304...0x307
 */
static struct dm9000_platform_data dm9000_data = {
	.srom     = 1,
};

static struct s3c_mci_platform_data mci_data = {
	.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED,
	.voltages = MMC_VDD_32_33 | MMC_VDD_33_34,
	.gpio_detect = 232,	/* GPG8_GPIO */
	.detect_invert = 0,
};

static struct fb_videomode s3c24x0_fb_modes[] = {
#ifdef CONFIG_MINI2440_VIDEO_N35
	{
		.name		= "N35",
		.refresh	= 60,
		.xres		= 240,
		.left_margin	= 21,
		.right_margin	= 38,
		.hsync_len	= 6,
		.yres		= 320,
		.upper_margin	= 4,
		.lower_margin	= 4,
		.vsync_len	= 2,
		.pixclock	= 115913,
		.sync		= FB_SYNC_USE_PWREN,
		.vmode		= FB_VMODE_NONINTERLACED,
		.flag		= 0,
	},
#endif
#ifdef CONFIG_MINI2440_VIDEO_A70
	{
		.name		= "A70",
		.refresh	= 50,
		.xres		= 800,
		.left_margin	= 40,
		.right_margin	= 40,
		.hsync_len	= 48,
		.yres		= 480,
		.upper_margin	= 29,
		.lower_margin	= 3,
		.vsync_len	= 3,
		.pixclock	= 41848,
		.sync		= FB_SYNC_USE_PWREN | FB_SYNC_DE_HIGH_ACT,
		.vmode		= FB_VMODE_NONINTERLACED,
		.flag		= 0,
	},
#endif
#ifdef CONFIG_MINI2440_VIDEO_SVGA
	{
		.name		= "SVGA",
		.refresh	= 24,
		.xres		= 1024,
		.left_margin	= 1,
		.right_margin	= 2,
		.hsync_len	= 2,
		.yres		= 768,
		.upper_margin	= 200,
		.lower_margin	= 16,
		.vsync_len	= 16,
		.pixclock	= 40492,
		.sync		= FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT | FB_SYNC_DE_HIGH_ACT
                                  /* | FB_SYNC_SWAP_HW */ /* FIXME maybe */ ,
		.vmode		= FB_VMODE_NONINTERLACED,
		.flag		= 0,
	},
#endif
};

static struct s3c_fb_platform_data s3c24x0_fb_data = {
	.mode_list		= s3c24x0_fb_modes,
	.mode_cnt		= sizeof(s3c24x0_fb_modes) / sizeof(struct fb_videomode),
	.bits_per_pixel		= 16,
	.passive_display	= 0,
};

static const unsigned pin_usage[] = {
	/* address bus, used by NOR, SDRAM */
	GPA1_ADDR16,
	GPA2_ADDR17,
	GPA3_ADDR18,
	GPA4_ADDR19,
	GPA5_ADDR20,
	GPA6_ADDR21,
	GPA7_ADDR22,

	GPA8_ADDR23_GPIO | GPIO_IN,
	GPA9_ADDR24,	/* BA0 */
	GPA10_ADDR25,	/* BA1 */
	GPA11_ADDR26_GPIO | GPIO_IN,	/* not connected */

	/* DM9000 requirements */
	GPA15_NGCS4,
	GPF7_EINT7,

	/* de-activate the speaker */
	GPB0_GPIO | GPIO_OUT | GPIO_VAL(0),

	/* SD socket */
	GPE5_SDCLK,
	GPE6_SDCMD,
	GPE7_SDDAT0,
	GPE8_SDDAT1,
	GPE9_SDDAT2,
	GPE10_SDDAT3,
	GPG8_GPIO | GPIO_IN,	/* change detection */
	GPH8_GPIO | GPIO_IN,	/* write protection sense */

	/* NAND requirements */
	GPA17_CLE,
	GPA18_ALE,
	GPA19_NFWE,
	GPA20_NFRE,
	GPA21_NRSTOUT,
	GPA22_NFCE,

	/* Video out */
	GPC0_LEND,
	GPC1_VCLK,
	GPC2_VLINE,
	GPC3_VFRAME,
	GPC4_VM,
	GPC5_LPCOE,
	GPC6_LPCREV,
	GPC7_LPCREVB,
	GPG4_LCD_PWREN,

	GPC8_VD0,
	GPC9_VD1,
	GPC10_VD2,
	GPC11_VD3,
	GPC12_VD4,
	GPC13_VD5,
	GPC14_VD6,
	GPC15_VD7,
	GPD0_VD8,
	GPD1_VD9,
	GPD2_VD10,
	GPD3_VD11,
	GPD4_VD12,
	GPD5_VD13,
	GPD6_VD14,
	GPD7_VD15,
	GPD8_VD16,
	GPD9_VD17,
	GPD10_VD18,
	GPD11_VD19,
	GPD12_VD20,
	GPD13_VD21,
	GPD14_VD22,
	GPD15_VD23,

	/* K6 or CON12, pin 6, external pull up  */
	GPG11_EINT19 | GPIO_IN,
	/* K5 or CON12, pin 5*/
	GPG7_EINT15 | GPIO_IN,
	/* K4 or CON12, pin 4 */
	GPG6_EINT14 | GPIO_IN,
	/* K3 or CON12, pin 3 */
	GPG5_EINT13 | GPIO_IN,
	/* K2 or CON12, pin 2 */
	GPG3_EINT11 | GPIO_IN,
	/* K1 or CON12, pin 1, external pull up */
	GPG0_EINT8 | GPIO_IN,

	/* LED 1 1=off */
	GPB5_GPIO | GPIO_OUT | GPIO_VAL(1),
	/* LED 2 1=off */
	GPB6_GPIO | GPIO_OUT | GPIO_VAL(1),
	/* LED 3 1=off */
	GPB7_GPIO | GPIO_OUT | GPIO_VAL(1),
	/* LED 4 1=off */
	GPB8_GPIO | GPIO_OUT | GPIO_VAL(1),

	/* camera interface (ignore it) */
	GPJ0_GPIO | GPIO_IN,
	GPJ1_GPIO | GPIO_IN,
	GPJ2_GPIO | GPIO_IN,
	GPJ3_GPIO | GPIO_IN,
	GPJ4_GPIO | GPIO_IN,
	GPJ5_GPIO | GPIO_IN,
	GPJ6_GPIO | GPIO_IN,
	GPJ7_GPIO | GPIO_IN,
	GPJ8_GPIO | GPIO_IN,
	GPJ9_GPIO | GPIO_IN,
	GPJ10_GPIO | GPIO_IN,
	GPJ11_GPIO | GPIO_IN,
	GPJ12_GPIO | GPIO_IN,

	/* I2C bus */
	GPE14_IICSCL,	/* external pull up */
	GPE15_IICSDA,	/* external pull up */

	GPA12_NGCS1,	/* CON5, pin 7 */
	GPA13_NGCS2,	/* CON5, pin 8 */
	GPA14_NGCS3,	/* CON5, pin 9 */
	GPA16_NGCS5,	/* CON5, pin 10 */

	/* UART2 (spare) */
	GPH4_TXD1,
	GPH5_RXD1,

	/* UART3 (spare) */
	GPH6_TXD2,
	GPH7_RXD2,
};

static int mini2440_mem_init(void)
{
	arm_add_mem_device("ram0", S3C_SDRAM_BASE, s3c24xx_get_memory_size());

	return 0;
}
mem_initcall(mini2440_mem_init);

static int mini2440_devices_init(void)
{
	uint32_t reg;
	int i;

	/* ----------- configure the access to the outer space ---------- */
	for (i = 0; i < ARRAY_SIZE(pin_usage); i++)
		s3c_gpio_mode(pin_usage[i]);

	reg = readl(S3C_BWSCON);

	/* CS#4 to access the network controller */
	reg &= ~0x000f0000;
	reg |=  0x000d0000;	/* 16 bit */
	writel(0x1f4c, S3C_BANKCON4);

	writel(reg, S3C_BWSCON);

	/* release the reset signal to external devices */
	reg = readl(S3C_MISCCR);
	reg |= 0x10000;
	writel(reg, S3C_MISCCR);

	s3c24xx_add_nand(&nand_info);

	add_dm9000_device(0, S3C_CS4_BASE + 0x300, S3C_CS4_BASE + 0x304,
			  IORESOURCE_MEM_16BIT, &dm9000_data);
#ifdef CONFIG_NAND
	/* ----------- add some vital partitions -------- */
	devfs_del_partition("self_raw");
	devfs_add_partition("nand0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_del_partition("env_raw");
	devfs_add_partition("nand0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
#endif
	s3c24xx_add_mci(&mci_data);
	s3c24xx_add_fb(&s3c24x0_fb_data);
	s3c24xx_add_ohci();
	armlinux_set_bootparams((void*)S3C_SDRAM_BASE + 0x100);
	armlinux_set_architecture(MACH_TYPE_MINI2440);

	return 0;
}

device_initcall(mini2440_devices_init);

static int mini2440_console_init(void)
{
	/*
	 * configure the UART1 right now, as barebox will
	 * start to send data immediately
	 */
	s3c_gpio_mode(GPH0_NCTS0);
	s3c_gpio_mode(GPH1_NRTS0);
	s3c_gpio_mode(GPH2_TXD0);
	s3c_gpio_mode(GPH3_RXD0);

	barebox_set_model("Friendlyarm mini2440");
	barebox_set_hostname("mini2440");

	s3c24xx_add_uart1();
	return 0;
}

console_initcall(mini2440_console_init);

/** @page mini2440 FriendlyARM's mini2440

This system is based on a Samsung S3C2440 CPU. The card is shipped with:

- S3C2440\@400 MHz or 533 MHz (ARM920T/ARMv4T)
- 12 MHz crystal reference
- 32.768 kHz crystal reference
- SDRAM 64 MiB (one bank only)
   - HY57V561620 (two devices for 64 MiB to form a 32 bit bus)
     - 4M x 16bit x 4 Banks Mobile SDRAM
     - 8192 refresh cycles / 64 ms
     - CL2\@100 MHz
     - 133 MHz max
     - collumn address size is 9 bits
     - row address size is 13 bits
   - MT48LC16M16 (two devices for 64 MiB to form a 32 bit bus)
     - 4M x 16bit x 4 Banks Mobile SDRAM
     - commercial & industrial type
     - 8192 refresh cycles / 64 ms
     - CL2\@100 MHz
     - 133 MHz max
     - collumn address size is 9 bits
     - row address size is 13 bits
- NAND Flash 128MiB...1GiB
   - K9Fxx08
- NOR Flash (up to 22 address lines available)
   - AM29LV160DB, 2 MiB
   - SST39VF1601, 2 MiB
   - 16 bit data bus
- SD card interface, 3.3V (fixed voltage)
- Host and device USB interface, USB1.1 compliant
- UDA1341TS Audio
- DM9000 Ethernet interface
  - uses CS#4
  - uses EINT7
  - 16 bit data bus
- I2C interface, 100 KHz and 400 KHz
  - EEPROM
    - ST M24C08
    - address 0x50
- Speaker on GPB0 ("low" = inactive)
- LCD interface
- Touch Screen interface
- Camera interface
- I2S interface
- AC97 Audio-CODEC interface
- three serial RS232 interfaces (one with level converter)
- SPI interface
- JTAG interface

How to get the binary image:

Using the default configuration:

@code
make ARCH=arm mini2440_defconfig
@endcode

Build the binary image:

@code
make ARCH=arm CROSS_COMPILE=armv4compiler
@endcode

@note replace the armv4compiler with your ARM v4 cross compiler.

How to bring in \a barebox ?

First run it as a second stage bootloader. There are two known working ways to
do so:

One way is to use the "device firmware update" feature of the 'supervivi'.
 - connect a terminal application to the mini2440's serial connector
 - switch S2 to 'boot from NOR' to boot into 'supervivi'
 - connect your host to the usb device connector on the mini2440
 - switch on your mini2440
 - in 'supervivi' type q (command line) then:
@code
load ram 0x31000000 \<barebox-size\> u
@endcode
 - use a tool for DFU update (for example from openkomo) to transfer the 'barebox.bin' binary
 - then in 'supervivi' just run
@code
go 0x31000000
@endcode

A second way is to use any kind of JTAG adapter. For this case I'm using the
'JTAKkey tiny' from Amontec and OpenOCD. First you need an adapter for this
kind of Dongle as it uses a 20 pin connector with 2.54 mm grid, and the
mini2440 uses a 10 pin connector with 2 mm grid.

@code
             Amontec JTAGkey tiny               mini2440
           -------------------------------------------------------
              VREF  1   2  n.c.             VREF  1   2  VREF
            TRST_N  3   4  GND            TRST_N  3   4  SRST_N
               TDI  5   6  GND               TDI  5   6  TDO
               TMS  7   8  GND               TMS  7   8  GND
               TCK  9  10  GND               TCK  9  10  GND
              n.c. 11  12  GND
               TDO 13  14  GND
            SRST_N 15  16  GND
              n.c. 17  18  GND
              n.c. 19  20  GND
@endcode

Create a simple board description file. I did it this way:

@code
source [find interface/jtagkey-tiny.cfg]
source [find target/samsung_s3c2440.cfg]

adapter_khz 12000
@endcode

And then the following steps:
 - connect a terminal application to the mini2440's serial connector
 - connect the mini2440 to a working network
 - switch S2 to boot from NOR to boot into 'supervivi'
 - switch on your mini2440
 - run the OpenOCD daemon configured with the file shown above
 - connect to the OpenOCD daemon via 'telnet'.
 - run the following commands to download @a barebox into your target
@code
> halt
> load_image \<path to the 'barebox.bin'\> 0x31000000 bin
> resume 0x31000000
@endcode

Now @a barebox is starting from an already initialized CPU and SDRAM (done by
'supervivi').

Change to your terminal console and configure the network first. Adapt the
following settings to your network:
@code
eth0.ipaddr=192.168.1.240
eth0.netmask=255.255.255.0
eth0.gateway=192.168.23.2
eth0.serverip=192.168.1.7
eth0.ethaddr=00:04:f3:00:06:35
@endcode

A 'ping' to your TFTP server should bring a "...is alive" message now.

We are ready now to program @a barebox into the NAND flash:

@code
erase /dev/nand0.barebox.bb
tftp barebox.bin /dev/nand0.barebox.bb
@endcode

*/
