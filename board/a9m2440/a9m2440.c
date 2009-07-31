/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
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
 *
 */

/**
 * @file
 * @brief a9m2440 Specific Board Initialization routines
 *
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <partition.h>
#include <nand.h>
#include <asm/io.h>
#include <asm/arch/s3c24x0-iomap.h>
#include <asm/arch/s3c24x0-nand.h>

static struct device_d sdram_dev = {
	.name     = "ram",
	.map_base = CS6_BASE,
};

static struct s3c24x0_nand_platform_data nand_info = {
	.nand_timing = CALC_NFCONF_TIMING(A9M2440_TACLS, A9M2440_TWRPH0, A9M2440_TWRPH1)
};

static struct device_d nand_dev = {
	.name     = "s3c24x0_nand",
	.map_base = S3C24X0_NAND_BASE,
	.platform_data	= &nand_info,
};

/*
 * SMSC 91C111 network controller on the baseboard
 * connected to CS line 1 and interrupt line
 * GPIO3, data width is 32 bit
 */
static struct device_d network_dev = {
        .name     = "smc91c111",
        .map_base = CS1_BASE + 0x300,
        .size     = 16,
};

static int a9m2440_devices_init(void)
{
	uint32_t reg;

	/*
	 * detect the current memory size
	 * Note: On this card the second SDRAM page is not used
	 */
	reg = readl(BANKSIZE);

	switch (reg &= 0x7) {
	case 0:
		sdram_dev.size = 32 * 1024 * 1024;
		break;
	case 1:
		sdram_dev.size = 64 * 1024 * 1024;
		break;
	case 2:
		sdram_dev.size = 128 * 1024 * 1024;
		break;
	case 4:
		sdram_dev.size = 2 * 1024 * 1024;
		break;
	case 5:
		sdram_dev.size = 4 * 1024 * 1024;
		break;
	case 6:
		sdram_dev.size = 8 * 1024 * 1024;
		break;
	case 7:
		sdram_dev.size = 16 * 1024 * 1024;
		break;
	}

	/* both banks are present */
	sdram_dev.size <<= 1;

	/* ---------- configure the GPIOs ------------- */
	writel(0x007FFFFF, GPACON);
	writel(0x00000000, GPCCON);
	writel(0x00000000, GPCUP);
	writel(0x00000000, GPDCON);
	writel(0x00000000, GPDUP);
	writel(0xAAAAAAAA, GPECON);
	writel(0x0000E03F, GPEUP);
	writel(0x00000000, GPBCON);	/* all inputs */
	writel(0x00000007, GPBUP);	/* pullup disabled for GPB0..3 */
	writel(0x00009000, GPFCON);	/* GPF7 CLK_INT#, GPF6 Debug-LED */
	writel(0x000000FF, GPFUP);
	writel(readl(GPGDAT) | 0x1010, GPGDAT);	/* switch off IDLE_SW#, switch off LCD backlight */
	writel(0x0100A93A, GPGCON);	/* switch on USB device */
	writel(0x0000F000, GPGUP);
	writel(0x0029FAAA, GPHCON);

	writel((1 << 12) | (0 << 11), GPJDAT);
	writel(0x0016aaaa, GPJCON);
	writel(~((0<<12)| (1<<11)), GPJUP);

	writel((0 << 12) | (0 << 11), GPJDAT);
	writel(0x0016aaaa, GPJCON);
	writel(0x00001fff, GPJUP);

	writel(0x00000000, DSC0);
	writel(0x00000000, DSC1);

	/*
	 * USB port1 normal, USB port0 normal, USB1 pads for device
	 * PCLK output on CLKOUT0, UPLL CLK output on CLKOUT1,
	 */
	writel(0x00140, MISCCR);

	/* ----------- configure the access to the outer space ---------- */
	reg = readl(BWSCON);

	/* CS#1 to access the network controller */
	reg &= ~0xf0;
	reg |= 0xe0;
	writel(0x1350, BANKCON1);

	/* CS#2 to the dual 16550 UART */
	reg &= ~0xf00;
	reg |= 0x400;
	writel(0x0d50, BANKCON2);

	writel(reg, BWSCON);

	/* release the reset signal to the network and UART device */
        reg = readl(MISCCR);
	reg |= 0x10000;
	writel(reg, MISCCR);

	/* ----------- the devices the boot loader should work with -------- */
	register_device(&nand_dev);
	register_device(&sdram_dev);
	register_device(&network_dev);

#ifdef CONFIG_NAND
	/* ----------- add some vital partitions -------- */
	devfs_add_partition("nand0", 0x00000, 0x40000, PARTITION_FIXED, "self_raw");
        dev_add_bb_dev("self_raw", "self0");

        devfs_add_partition("nand0", 0x40000, 0x20000, PARTITION_FIXED, "env_raw");
#endif
	armlinux_set_bootparams((void *)sdram_dev.map_base + 0x100);

	armlinux_set_architecture(MACH_TYPE_A9M2440);

	return 0;
}

device_initcall(a9m2440_devices_init);

#ifdef CONFIG_S3C24XX_NAND_BOOT
void __bare_init nand_boot(void)
{
	s3c24x0_nand_load_image((void *)TEXT_BASE, 256 * 1024, 0, 512);
}
#endif

static struct device_d a9m2440_serial_device = {
	.name     = "s3c24x0_serial",
	.map_base = UART1_BASE,
	.size     = UART1_SIZE,
};

static int a9m2440_console_init(void)
{
	register_device(&a9m2440_serial_device);
	return 0;
}

console_initcall(a9m2440_console_init);

/** @page a9m2440 DIGI's a9m2440

This CPU card is based on a Samsung S3C2440 CPU. The card is shipped with:

- S3C2440\@400 MHz or 533 MHz (ARM920T/ARMv4T)
- 16.9344 MHz crystal reference
- SDRAM 64 MiB
   - Samsung K4M563233E-EE1H
   - 2M x 32Bit x 4 Banks Mobile SDRAM
   - 90 pin FBGA
   - CL3\@133MHz, CL2\@100MHz (CAS/RAS delay 19ns)
   - four banks
   - 32 bit data bits
   - row address size is 11
   - Row cycle time: 69ns
   - collumn address size is 9 bits
   - Extended temperature range (-25°C...85°C)
   - 64ms refresh period (4k)
- NAND Flash 64 MiB
   - Samsung KM29U512T
   - 64MiB 3,3V 8-bit
   - ID: 0xEC, 0x76, 0x??, 0xBD
   - 30ns/40ns/20ns
- I2C interface, 100KHz and 400KHz
  - Real Time Clock
    - Dallas DS1337
    - address 0x68
- LCD interface
- Touch Screen interface
- Camera interface
- I2S interface
- AC97 Audio-CODEC interface
- SD card interface
- 3 serial RS232 interfaces
- Host and device USB interface, USB1.1 compliant
- Ethernet interface
  - 10Mbps, Cirrus Logic, CS8900A (on the CPU card) or
  - 10/100Mbps, SMSC 91C111 (on the baseboard)
- SPI interface
- JTAG interface

*/
