/**
 * @file
 * @brief Provide Generic implementations for OMAP3 architecture
 *
 * FileName: arch/arm/mach-omap/omap3_generic.c
 *
 * This file contains the generic implementations of various OMAP3
 * relevant functions
 * For more info on OMAP34XX, see http://focus.ti.com/pdfs/wtbu/swpu114g.pdf
 *
 * Important one is @ref a_init which is architecture init code.
 * The implemented functions are present in sys_info.h
 *
 * Originally from http://linux.omap.com/pub/bootloader/3430sdp/u-boot-v1.tar.gz
 */
/*
 * (C) Copyright 2006-2008
 * Texas Instruments, <www.ti.com>
 * Richard Woodruff <r-woodruff2@ti.com>
 * Nishanth Menon <x0nishan@ti.com>
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
#include <init.h>
#include <asm/io.h>
#include <mach/silicon.h>
#include <mach/gpmc.h>
#include <mach/sdrc.h>
#include <mach/control.h>
#include <mach/omap3-smx.h>
#include <mach/clocks.h>
#include <mach/wdt.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/xload.h>

/**
 * @brief Reset the CPU
 *
 * In case of crashes, reset the CPU
 *
 * @param addr Cause of crash
 *
 * @return void
 */
void __noreturn reset_cpu(unsigned long addr)
{
	/* FIXME: Enable WDT and cause reset */
	hang();
}
EXPORT_SYMBOL(reset_cpu);

/**
 * @brief Low level CPU type
 *
 * @return Detected CPU type
 */
u32 get_cpu_type(void)
{
	u32 idcode_val;
	u16 hawkeye;

	idcode_val = readl(IDCODE_REG);

	hawkeye = get_hawkeye(idcode_val);

	if (hawkeye == OMAP_HAWKEYE_34XX)
		return CPU_3430;

	if (hawkeye == OMAP_HAWKEYE_36XX)
		return CPU_3630;

	/*
	 * Fallback to OMAP3430 as default.
	 */
	return CPU_3430;
}

/**
 * @brief Extract the OMAP ES revision
 *
 * The significance of the CPU revision depends upon the cpu type.
 * Latest known revision is considered default.
 *
 * @return silicon version
 */
u32 get_cpu_rev(void)
{
	u32 idcode_val;
	u32 version, retval;

	idcode_val = readl(IDCODE_REG);

	version = get_version(idcode_val);

	switch (get_cpu_type()) {
	case CPU_3630:
		switch (version) {
		case 0:
			retval = OMAP36XX_ES1;
			break;
		case 1:
			retval = OMAP36XX_ES1_1;
			break;
		case 2:
			/*
			 * Fall through the default case.
			 */
		default:
			retval = OMAP36XX_ES1_2;
		}
		break;
	case CPU_3430:
		/*
		 * Same as default case
		 */
	default:
		/*
		 * On OMAP3430 ES1.0 the IDCODE register is not exposed on L4.
		 * Use CPU ID to check for the same.
		 */
		__asm__ __volatile__("mrc p15, 0, %0, c0, c0, 0":"=r"(retval));
		if ((retval & 0xf) == 0x0) {
			retval = OMAP34XX_ES1;
		} else {
			switch (version) {
			case 0: /* This field was not set in early samples */
			case 1:
				retval = OMAP34XX_ES2;
				break;
			case 2:
				retval = OMAP34XX_ES2_1;
				break;
			case 3:
				retval = OMAP34XX_ES3;
				break;
			case 4:
				/*
				 * Same as default case
				 */
			default:
				retval = OMAP34XX_ES3_1;
			}
		}
	}

	return retval;
}

/**
 * @brief Get size of chip select 0/1
 *
 * @param[in] offset  give the offset if we need CS1
 *
 * @return return the sdram size.
 */
u32 get_sdr_cs_size(u32 offset)
{
	u32 size;
	/* get ram size field */
	size = readl(SDRC_REG(MCFG_0) + offset) >> 8;
	size &= 0x3FF;		/* remove unwanted bits */
	size *= 2 * (1024 * 1024);	/* find size in MB */
	return size;
}

/**
 * @brief Get the initial SYSBOOT value
 *
 * SYSBOOT is useful to know which state OMAP booted from.
 *
 * @return - Return the value of SYSBOOT.
 */
inline u32 get_sysboot_value(void)
{
	return (0x0000003F & readl(CONTROL_REG(STATUS)));
}

/**
 * @brief Return the current CS0 base address
 *
 * Return current address hardware will be
 * fetching from. The below effectively gives what is correct, its a bit
 * mis-leading compared to the TRM.  For the most general case the mask
 * needs to be also taken into account this does work in practice.
 *
 * @return  base address
 */
u32 get_gpmc0_base(void)
{
	u32 b;
	b = readl(GPMC_REG(CONFIG7_0));
	b &= 0x1F;		/* keep base [5:0] */
	b = b << 24;		/* ret 0x0b000000 */
	return b;
}

/**
 * @brief Get the upper address of current execution
 *
 * we can use this to figure out if we are running in SRAM /
 * XIP Flash or in SDRAM
 *
 * @return base address
 */
u32 get_base(void)
{
	u32 val;
	__asm__ __volatile__("mov %0, pc \n":"=r"(val)::"memory");
	val &= 0xF0000000;
	val >>= 28;
	return val;
}

/**
 * @brief Are we running in Flash XIP?
 *
 * If the base is in GPMC address space, we probably are!
 *
 * @return 1 if we are running in XIP mode, else return 0
 */
u32 running_in_flash(void)
{
	if (get_base() < 4)
		return 1;	/* in flash */
	return 0;		/* running in SRAM or SDRAM */
}

/**
 * @brief Are we running in OMAP internal SRAM?
 *
 * If in SRAM address, then yes!
 *
 * @return  1 if we are running in SRAM, else return 0
 */
u32 running_in_sram(void)
{
	if (get_base() == 4)
		return 1;	/* in SRAM */
	return 0;		/* running in FLASH or SDRAM */
}

/**
 * @brief Are we running in SDRAM?
 *
 * if we are not in GPMC nor in SRAM address space,
 * we are in SDRAM execution area
 *
 * @return 1 if we are running from SDRAM, else return 0
 */
u32 running_in_sdram(void)
{
	if (get_base() > 4)
		return 1;	/* in sdram */
	return 0;		/* running in SRAM or FLASH */
}
EXPORT_SYMBOL(running_in_sdram);

/**
 * @brief Is this an XIP type device or a stream one
 *
 * Sysboot bits 4-0 specify type.  Bit 5, sys mem/perif
 *
 * @return Boot type
 */
u32 get_boot_type(void)
{
	u32 v;
	v = get_sysboot_value() & ((0x1 << 4) | (0x1 << 3) | (0x1 << 2) |
				   (0x1 << 1) | (0x1 << 0));
	return v;
}

/**
 * @brief What type of device are we?
 *
 * are we on a GP/HS/EMU/TEST device?
 *
 * @return  device type
 */
u32 get_device_type(void)
{
	int mode;
	mode = readl(CONTROL_REG(STATUS)) & (DEVICE_MASK);
	return (mode >>= 8);
}

/**
 * @brief Setup security registers for access
 *
 * This can be done for GP Device only. for HS/EMU devices, read TRM.
 *
 * @return void
 */
static void secure_unlock_mem(void)
{
	/* Permission values for registers -Full fledged permissions to all */
#define UNLOCK_1 0xFFFFFFFF
#define UNLOCK_2 0x00000000
#define UNLOCK_3 0x0000FFFF
	/* Protection Module Register Target APE (PM_RT) */
	writel(UNLOCK_1, RT_REQ_INFO_PERMISSION_1);
	writel(UNLOCK_1, RT_READ_PERMISSION_0);
	writel(UNLOCK_1, RT_WRITE_PERMISSION_0);
	writel(UNLOCK_2, RT_ADDR_MATCH_1);

	writel(UNLOCK_3, GPMC_REQ_INFO_PERMISSION_0);
	writel(UNLOCK_3, GPMC_READ_PERMISSION_0);
	writel(UNLOCK_3, GPMC_WRITE_PERMISSION_0);

	writel(UNLOCK_3, OCM_REQ_INFO_PERMISSION_0);
	writel(UNLOCK_3, OCM_READ_PERMISSION_0);
	writel(UNLOCK_3, OCM_WRITE_PERMISSION_0);
	writel(UNLOCK_2, OCM_ADDR_MATCH_2);

	/* IVA Changes */
	writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_0);
	writel(UNLOCK_3, IVA2_READ_PERMISSION_0);
	writel(UNLOCK_3, IVA2_WRITE_PERMISSION_0);

	writel(UNLOCK_1, SMS_RG_ATT0);	/* SDRC region 0 public */
}

/**
 * @brief Come out of secure mode
 * If chip is EMU and boot type is external configure
 * secure registers and exit secure world general use.
 *
 * @return void
 */
static void secureworld_exit(void)
{
	unsigned long i;

	/* configrue non-secure access control register */
	__asm__ __volatile__("mrc p15, 0, %0, c1, c1, 2":"=r"(i));
	/* enabling co-processor CP10 and CP11 accesses in NS world */
	__asm__ __volatile__("orr %0, %0, #0xC00":"=r"(i));
	/* allow allocation of locked TLBs and L2 lines in NS world */
	/* allow use of PLE registers in NS world also */
	__asm__ __volatile__("orr %0, %0, #0x70000":"=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c1, 2":"=r"(i));

	/* Enable ASA in ACR register */
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 1":"=r"(i));
	__asm__ __volatile__("orr %0, %0, #0x10":"=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 1":"=r"(i));

	/* Exiting secure world */
	__asm__ __volatile__("mrc p15, 0, %0, c1, c1, 0":"=r"(i));
	__asm__ __volatile__("orr %0, %0, #0x31":"=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c1, 0":"=r"(i));
}

/**
 * @brief Shut down the watchdogs
 *
 * There are 3 watch dogs WD1=Secure, WD2=MPU, WD3=IVA. WD1 is
 * either taken care of by ROM (HS/EMU) or not accessible (GP).
 * We need to take care of WD2-MPU or take a PRCM reset.  WD3
 * should not be running and does not generate a PRCM reset.
 *
 * @return void
 */
static void watchdog_init(void)
{
	int pending = 1;

	sr32(CM_REG(FCLKEN_WKUP), 5, 1, 1);
	sr32(CM_REG(ICLKEN_WKUP), 5, 1, 1);
	wait_on_value((0x1 << 5), 0x20, CM_REG(IDLEST_WKUP), 5);

	writel(WDT_DISABLE_CODE1, WDT_REG(WSPR));

	do {
		pending = readl(WDT_REG(WWPS));
	} while (pending);

	writel(WDT_DISABLE_CODE2, WDT_REG(WSPR));
}

/**
 * @brief Write to AuxCR desired value using SMI.
 *  general use.
 *
 * @return void
 */
static void setup_auxcr(void)
{
	unsigned long i;
	volatile unsigned int j;
	/* Save r0, r12 and restore them after usage */
	__asm__ __volatile__("mov %0, r12":"=r"(j));
	__asm__ __volatile__("mov %0, r0":"=r"(i));

	/* GP Device ROM code API usage here */
	/* r12 = AUXCR Write function and r0 value */
	__asm__ __volatile__("mov r12, #0x3");
	__asm__ __volatile__("mrc p15, 0, r0, c1, c0, 1");
	/* Enabling ASA */
	__asm__ __volatile__("orr r0, r0, #0x10");
	/* SMI instruction to call ROM Code API */
	__asm__ __volatile__(".word 0xE1600070");
	__asm__ __volatile__("mov r0, %0":"=r"(i));
	__asm__ __volatile__("mov r12, %0":"=r"(j));
}

/**
 * @brief Try to unlock the SRAM for general use
 *
 * If chip is GP/EMU(special) type, unlock the SRAM for
 * general use.
 *
 * @return void
 */
static void try_unlock_memory(void)
{
	int mode;
	int in_sdram = running_in_sdram();

	/* if GP device unlock device SRAM for general use */
	/* secure code breaks for Secure/Emulation device - HS/E/T */
	mode = get_device_type();
	if (mode == GP_DEVICE)
		secure_unlock_mem();
	/* If device is EMU and boot is XIP external booting
	 * Unlock firewalls and disable L2 and put chip
	 * out of secure world
	 */
	/* Assuming memories are unlocked by the demon who put us in SDRAM */
	if ((mode <= EMU_DEVICE) && (get_boot_type() == 0x1F)
	    && (!in_sdram)) {
		secure_unlock_mem();
		secureworld_exit();
	}

	return;
}

/**
 * @brief OMAP3 Architecture specific Initialization
 *
 * Does early system init of disabling the watchdog, enable
 * memory and configuring the clocks.
 *
 * prcm_init is called only if CONFIG_OMAP3_CLOCK_CONFIG is defined.
 * We depend on link time clean up to remove a_init if no caller exists.
 *
 * @warning Called path is with SRAM stack
 *
 * @return void
 */
void a_init(void)
{
	watchdog_init();

	try_unlock_memory();

	/* Writing to AuxCR in barebox using SMI for GP DEV */
	/* Currently SMI in Kernel on ES2 devices seems to have an isse
	 * Once that is resolved, we can postpone this config to kernel
	 */
	if (get_device_type() == GP_DEVICE)
		setup_auxcr();

	sdelay(100);

#ifdef CONFIG_OMAP3_CLOCK_CONFIG
	prcm_init();
#endif

}

#define OMAP3_TRACING_VECTOR1 0x4020ffb4

enum omap_boot_src omap3_bootsrc(void)
{
	u32 bootsrc = readl(OMAP3_TRACING_VECTOR1);

	if (bootsrc & (1 << 2))
		return OMAP_BOOTSRC_NAND;
	if (bootsrc & (1 << 6))
		return OMAP_BOOTSRC_MMC1;
	return OMAP_BOOTSRC_UNKNOWN;
}
