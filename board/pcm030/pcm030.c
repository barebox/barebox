/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2004
 * Mark Jonas, Freescale Semiconductor, mark.jonas@motorola.com.
 *
 * (C) Copyright 2006
 * Eric Schumann, Phytec Messtechnik GmbH
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

#include <common.h>
#include <driver.h>
#include <cfi_flash.h>
#include <init.h>
#include <asm/arch/mpc5xxx.h>
#include <asm/arch/fec.h>
#include <types.h>
#include <partition.h>
#include <mem_malloc.h>
#include <reloc.h>

#ifdef CONFIG_VIDEO_OPENIP
#include <openip.h>
#endif

struct device_d cfi_dev = {
	.name     = "cfi_flash",
	.id       = "nor0",

	.map_base = 0xff000000,
	.size     = 16 * 1024 * 1024,
};

struct device_d sdram_dev = {
	.name     = "ram",
	.id       = "ram0",

	.map_base = 0x0,
	.size     = 64 * 1024 * 1024,

	.type     = DEVICE_TYPE_DRAM,
};

struct device_d scratch_dev = {
	.name     = "ram",
	.id       = "scratch0",
	.type     = DEVICE_TYPE_DRAM,
};

static struct mpc5xxx_fec_platform_data fec_info = {
	.xcv_type = MII100,
};

struct device_d eth_dev = {
	.name		= "fec_mpc5xxx",
	.id		= "eth0",
	.map_base	= MPC5XXX_FEC,
	.platform_data	= &fec_info,
	.type		= DEVICE_TYPE_ETHER,
};

#define SCRATCHMEM_SIZE (1024 * 1024 * 4)

static int devices_init (void)
{
	register_device(&cfi_dev);
	register_device(&sdram_dev);
	register_device(&eth_dev);

	scratch_dev.map_base = (unsigned long)sbrk_no_zero(SCRATCHMEM_SIZE);
	scratch_dev.size = SCRATCHMEM_SIZE;
	register_device(&scratch_dev);

	devfs_add_partition("nor0", 0x00f00000, 0x40000, PARTITION_FIXED, "self");
	devfs_add_partition("nor0", 0x00f60000, 0x20000, PARTITION_FIXED, "env");

	return 0;
}

device_initcall(devices_init);

static struct device_d psc3 = {
	.name     = "mpc5xxx_serial",
	.id       = "psc3",
	.map_base = MPC5XXX_PSC3,
	.size     = 4096,
	.type     = DEVICE_TYPE_CONSOLE,
};

static struct device_d psc6 = {
	.name     = "mpc5xxx_serial",
	.id       = "psc6",
	.map_base = MPC5XXX_PSC6,
	.size     = 4096,
	.type     = DEVICE_TYPE_CONSOLE,
};

static int console_init(void)
{
	register_device(&psc3);
	register_device(&psc6);
	return 0;
}

console_initcall(console_init);

void *get_early_console_base(const char *name)
{
	if (!strcmp(name, RELOC("psc3")))
		return (void *)MPC5XXX_PSC3;
	if (!strcmp(name, RELOC("psc6")))
		return (void *)MPC5XXX_PSC6;
	return NULL;
}

#include "mt46v32m16-75.h"

static void sdram_start (int hi_addr)
{
	long hi_addr_bit = hi_addr ? 0x01000000 : 0;

	/* unlock mode register */
	*(vu_long *)MPC5XXX_SDRAM_CTRL = SDRAM_CONTROL | 0x80000000 | hi_addr_bit;
	__asm__ volatile ("sync");

	/* precharge all banks */
	*(vu_long *)MPC5XXX_SDRAM_CTRL = SDRAM_CONTROL | 0x80000002 | hi_addr_bit;
	__asm__ volatile ("sync");

#if SDRAM_DDR
	/* set mode register: extended mode */
	*(vu_long *)MPC5XXX_SDRAM_MODE = SDRAM_EMODE;
	__asm__ volatile ("sync");

	/* set mode register: reset DLL */
	*(vu_long *)MPC5XXX_SDRAM_MODE = SDRAM_MODE | 0x04000000;
	__asm__ volatile ("sync");
#endif

	/* precharge all banks */
	*(vu_long *)MPC5XXX_SDRAM_CTRL = SDRAM_CONTROL | 0x80000002 | hi_addr_bit;
	__asm__ volatile ("sync");

	/* auto refresh */
	*(vu_long *)MPC5XXX_SDRAM_CTRL = SDRAM_CONTROL | 0x80000004 | hi_addr_bit;
	__asm__ volatile ("sync");

	/* set mode register */
	*(vu_long *)MPC5XXX_SDRAM_MODE = SDRAM_MODE;
	__asm__ volatile ("sync");

	/* normal operation */
	*(vu_long *)MPC5XXX_SDRAM_CTRL = SDRAM_CONTROL | hi_addr_bit;
	__asm__ volatile ("sync");
}

/*
 * ATTENTION: Although partially referenced initdram does NOT make real use
 *            use of CFG_SDRAM_BASE. The code does not work if CFG_SDRAM_BASE
 *            is something else than 0x00000000.
 */

long int initdram (int board_type)
{
	ulong dramsize = 0;
	ulong dramsize2 = 0;

	ulong test1, test2;

	if ((ulong)RELOC(initdram) > (2 << 30)) {
		/* setup SDRAM chip selects */
		*(vu_long *)MPC5XXX_SDRAM_CS0CFG = 0x0000001b;/* 256MB at 0x0 */
		*(vu_long *)MPC5XXX_SDRAM_CS1CFG = 0x10000000;/* disabled */
		__asm__ volatile ("sync");

		/* setup config registers */
		*(vu_long *)MPC5XXX_SDRAM_CONFIG1 = SDRAM_CONFIG1;
		*(vu_long *)MPC5XXX_SDRAM_CONFIG2 = SDRAM_CONFIG2;
		__asm__ volatile ("sync");

#if SDRAM_DDR && SDRAM_TAPDELAY
		/* set tap delay */
		*(vu_long *)MPC5XXX_CDM_PORCFG = SDRAM_TAPDELAY;
		__asm__ volatile ("sync");
#endif

		/* find RAM size using SDRAM CS0 only */
		sdram_start(0);
		test1 = get_ram_size((ulong *)CFG_SDRAM_BASE, 0x10000000);
		sdram_start(1);
		test2 = get_ram_size((ulong *)CFG_SDRAM_BASE, 0x10000000);
		if (test1 > test2) {
			sdram_start(0);
			dramsize = test1;
		} else {
			dramsize = test2;
		}

		/* memory smaller than 1MB is impossible */
		if (dramsize < (1 << 20)) {
			dramsize = 0;
		}

		/* set SDRAM CS0 size according to the amount of RAM found */
		if (dramsize > 0) {
			*(vu_long *)MPC5XXX_SDRAM_CS0CFG = 0x13 + __builtin_ffs(dramsize >> 20) - 1;
		} else {
			*(vu_long *)MPC5XXX_SDRAM_CS0CFG = 0; /* disabled */
		}
	} else
		puts(RELOC("skipping sdram initialization\n"));

	/* retrieve size of memory connected to SDRAM CS0 */
	dramsize = *(vu_long *)MPC5XXX_SDRAM_CS0CFG & 0xFF;
	if (dramsize >= 0x13) {
		dramsize = (1 << (dramsize - 0x13)) << 20;
	} else {
		dramsize = 0;
	}

	/* retrieve size of memory connected to SDRAM CS1 */
	dramsize2 = *(vu_long *)MPC5XXX_SDRAM_CS1CFG & 0xFF;
	if (dramsize2 >= 0x13) {
		dramsize2 = (1 << (dramsize2 - 0x13)) << 20;
	} else {
		dramsize2 = 0;
	}

	return dramsize + dramsize2;
}

#ifdef	CONFIG_PCI
static struct pci_controller hose;

extern void pci_mpc5xxx_init(struct pci_controller *);

void pci_init_board(void)
{
	pci_mpc5xxx_init(&hose);
}
#endif

#if defined(CONFIG_OF_FLAT_TREE) && defined(CONFIG_OF_BOARD_SETUP)
void
ft_board_setup(void *blob, bd_t *bd)
{
	ft_cpu_setup(blob, bd);
}
#endif

#if defined (CFG_CMD_IDE) && defined (CONFIG_IDE_RESET)

#define GPIO_PSC2_4	0x02000000UL

void init_ide_reset (void)
{
	debug ("init_ide_reset\n");

    	/* Configure PSC2_4 as GPIO output for ATA reset */
	*(vu_long *) MPC5XXX_WU_GPIO_ENABLE |= GPIO_PSC2_4;
	*(vu_long *) MPC5XXX_WU_GPIO_DIR    |= GPIO_PSC2_4;
	/* Deassert reset */
	*(vu_long *) MPC5XXX_WU_GPIO_DATA   |= GPIO_PSC2_4;
}

void ide_set_reset (int idereset)
{
	debug ("ide_reset(%d)\n", idereset);

	if (idereset) {
		*(vu_long *) MPC5XXX_WU_GPIO_DATA &= ~GPIO_PSC2_4;
		/* Make a delay. MPC5200 spec says 25 usec min */
		udelay(500000);
	} else {
		*(vu_long *) MPC5XXX_WU_GPIO_DATA |=  GPIO_PSC2_4;
	}
}
#endif /* defined (CFG_CMD_IDE) && defined (CONFIG_IDE_RESET) */

#ifdef CONFIG_VIDEO_OPENIP

#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240

#ifdef CONFIG_VIDEO_OPENIP_8BPP
#error CONFIG_VIDEO_OPENIP_8BPP not supported.
#endif /* CONFIG_VIDEO_OPENIP_8BPP */

#ifdef CONFIG_VIDEO_OPENIP_16BPP
#error CONFIG_VIDEO_OPENIP_16BPP not supported.
#endif /* CONFIG_VIDEO_OPENIP_16BPP */
#ifdef CONFIG_VIDEO_OPENIP_32BPP



static const SMI_REGS init_regs [] =
{
	{0x00008, 0x0248013f},
	{0x0000c, 0x021100f0},
	{0x00010, 0x018c0106},
	{0x00014, 0x00800000},
	{0x00018, 0x00800000},
	{0x00000, 0x00003701},
	{0, 0}
};
#endif /* CONFIG_VIDEO_OPENIP_32BPP */

#ifdef CONFIG_CONSOLE_EXTRA_INFO
/*
 * Return text to be printed besides the logo.
 */
void video_get_info_str (int line_number, char *info)
{
	if (line_number == 1) {
		strcpy (info, " Board: phyCORE-MPC5200B tiny (Phytec Messtechnik GmbH)");
	} else if (line_number == 2) {
		strcpy (info, "    on a PCM-980 baseboard");
	}
	else {
		info [0] = '\0';
	}
}
#endif

/*
 * Returns OPENIP register base address. First thing called in the driver.
 */
unsigned int board_video_init (void)
{
ulong dummy;
dummy  = *(vu_long *)OPENIP_MMIO_BASE; /*dummy read*/
dummy  = *(vu_long *)OPENIP_MMIO_BASE; /*dummy read*/
	return OPENIP_MMIO_BASE;
}

/*
 * Returns OPENIP framebuffer address
 */
unsigned int board_video_get_fb (void)
{

	return OPENIP_FB_BASE;
}

/*
 * Called after initializing the OPENIP and before clearing the screen.
 */
void board_validate_screen (unsigned int base)
{
}

/*
 * Return a pointer to the initialization sequence.
 */
const SMI_REGS *board_get_regs (void)
{
	return init_regs;
}

int board_get_width (void)
{
	return DISPLAY_WIDTH;
}

int board_get_height (void)
{
	return DISPLAY_HEIGHT;
}

#endif /* CONFIG_VIDEO_OPENIP */
