/**
 * @file
 * @brief Provide Generic implementations for OMAP3 architecture
 *
 * This file contains the generic implementations of various OMAP3
 * relevant functions
 * For more info on OMAP34XX, see http://focus.ti.com/pdfs/wtbu/swpu114g.pdf
 *
 * Important one is @ref a_init which is architecture init code.
 * The implemented functions are present in sys_info.h
 *
 * Originally from http://linux.omap.com/pub/bootloader/3430sdp/u-boot-v1.tar.gz
 *
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
 */

#include <common.h>
#include <bootsource.h>
#include <init.h>
#include <io.h>
#include <restart.h>
#include <mach/omap3-silicon.h>
#include <mach/gpmc.h>
#include <mach/generic.h>
#include <mach/sdrc.h>
#include <mach/control.h>
#include <mach/omap3-smx.h>
#include <mach/clocks.h>
#include <mach/omap3-clock.h>
#include <mach/wdt.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/omap3-generic.h>
#include <reset_source.h>

/**
 * @brief Reset the CPU
 *
 * In case of crashes, reset the CPU
 *
 * @param addr Cause of crash
 *
 * @return void
 */
static void __noreturn omap3_restart_soc(struct restart_handler *rst)
{
	writel(OMAP3_PRM_RSTCTRL_RESET, OMAP3_PRM_REG(RSTCTRL));

	hang();
}

/**
 * @brief Low level CPU type
 *
 * @return Detected CPU type
 */
u32 get_cpu_type(void)
{
	u32 idcode_val;
	u16 hawkeye;

	idcode_val = readl(OMAP3_IDCODE_REG);

	hawkeye = get_hawkeye(idcode_val);

	if (hawkeye == OMAP_HAWKEYE_34XX)
		return CPU_3430;

	if (hawkeye == OMAP_HAWKEYE_AM35XX)
		return CPU_AM35XX;

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

	idcode_val = readl(OMAP3_IDCODE_REG);

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
	size = readl(OMAP3_SDRC_REG(MCFG_0) + offset) >> 8;
	size &= 0x3FF;		/* remove unwanted bits */
	size *= 2 * (1024 * 1024);	/* find size in MB */
	return size;
}
EXPORT_SYMBOL(get_sdr_cs_size);
/**
 * @brief base address of chip select 1 (cs0 is defined at 0x80000000)
 *
 * @return return the CS1 base address.
 */
u32 get_sdr_cs1_base(void)
{
	u32 base;
	u32 cs_cfg;

	cs_cfg = readl(OMAP3_SDRC_REG(CS_CFG));
	/* get ram size field */
	base = (cs_cfg & 0x0000000F) << 2; /* get CS1STARTHIGH */
	base = base | ((cs_cfg & 0x00000300) >> 8); /* get CS1STARTLOW */
	base = base << 25;
	base += 0x80000000;
	return base;
}
EXPORT_SYMBOL(get_sdr_cs1_base);
/**
 * @brief Get the initial SYSBOOT value
 *
 * SYSBOOT is useful to know which state OMAP booted from.
 *
 * @return - Return the value of SYSBOOT.
 */
u32 get_sysboot_value(void)
{
	return (0x0000003F & readl(OMAP3_CONTROL_REG(STATUS)));
}

/**
 * @brief Get the upper address of current execution
 *
 * we can use this to figure out if we are running in SRAM /
 * XIP Flash or in SDRAM
 *
 * @return base address
 */
static u32 get_base(void)
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
u32 omap3_running_in_flash(void)
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
u32 omap3_running_in_sram(void)
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
u32 omap3_running_in_sdram(void)
{
	if (get_base() > 4)
		return 1;	/* in sdram */
	return 0;		/* running in SRAM or FLASH */
}
EXPORT_SYMBOL(omap3_running_in_sdram);

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
	mode = readl(OMAP3_CONTROL_REG(STATUS)) & (DEVICE_MASK);
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

	sr32(OMAP3_CM_REG(FCLKEN_WKUP), 5, 1, 1);
	sr32(OMAP3_CM_REG(ICLKEN_WKUP), 5, 1, 1);
	wait_on_value((0x1 << 5), 0x20, OMAP3_CM_REG(IDLEST_WKUP), 5);

	writel(WDT_DISABLE_CODE1, OMAP3_WDT_REG(WSPR));

	do {
		pending = readl(OMAP3_WDT_REG(WWPS));
	} while (pending);

	writel(WDT_DISABLE_CODE2, OMAP3_WDT_REG(WSPR));
}

/**
 * @brief Write to AuxCR desired value using SMI.
 *  general use.
 *
 * @return void
 */
void setup_auxcr(void);

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
	int in_sdram = omap3_running_in_sdram();

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
 * @warning Called path is with SRAM stack
 *
 * @return void
 */
void omap3_core_init(void)
{
	watchdog_init();

	try_unlock_memory();

	/* Writing to AuxCR in barebox using SMI for GP DEV */
	/* Currently SMI in Kernel on ES2 devices seems to have an isse
	 * Once that is resolved, we can postpone this config to kernel
	 */
	if (get_device_type() == GP_DEVICE) {
		setup_auxcr();
		omap3_gp_romcode_call(OMAP3_GP_ROMCODE_API_L2_INVAL, 0);
	}

	sdelay(100);

	prcm_init();
}

#define OMAP3_TRACING_VECTOR1 0x4020ffb4

static int omap3_bootsource(void)
{
	enum bootsource src = BOOTSOURCE_UNKNOWN;
	uint32_t *omap3_bootinfo = (void *)OMAP3_SRAM_SCRATCH_SPACE;

	switch (omap3_bootinfo[1] & 0xFF) {
	case 0x02:
		src = BOOTSOURCE_NAND;
		break;
	case 0x06:
		src = BOOTSOURCE_MMC;
		break;
	case 0x11:
		src = BOOTSOURCE_USB;
		break;
	default:
		src = BOOTSOURCE_UNKNOWN;
	}

	bootsource_set(src);
	bootsource_set_instance(0);

	return 0;
}

#define OMAP3_PRM_RSTST_OFF 0x8
#define OMAP3_REG_PRM_RSTST (OMAP3_PRM_REG(RSTCTRL) + OMAP3_PRM_RSTST_OFF)

#define OMAP3_ICECRUSHER_RST	BIT(10)
#define OMAP3_ICEPICK_RST	BIT(9)
#define OMAP3_EXTERNAL_WARM_RST	BIT(6)
#define OMAP3_SECURE_WD_RST	BIT(5)
#define OMAP3_MPU_WD_RST	BIT(4)
#define OMAP3_SECURITY_VIOL_RST	BIT(3)
#define OMAP3_GLOBAL_SW_RST	BIT(1)
#define OMAP3_GLOBAL_COLD_RST	BIT(0)

static void omap3_detect_reset_reason(void)
{
	uint32_t val = 0;

	val = readl(OMAP3_REG_PRM_RSTST);
	/* clear OMAP3_PRM_RSTST - must be cleared by software */
	writel(val, OMAP3_REG_PRM_RSTST);

	switch (val) {
	case OMAP3_ICECRUSHER_RST:
	case OMAP3_ICEPICK_RST:
		reset_source_set(RESET_JTAG);
		break;
	case OMAP3_EXTERNAL_WARM_RST:
		reset_source_set(RESET_EXT);
		break;
	case OMAP3_SECURE_WD_RST:
	case OMAP3_MPU_WD_RST:
	case OMAP3_SECURITY_VIOL_RST:
		reset_source_set(RESET_WDG);
		break;
	case OMAP3_GLOBAL_SW_RST:
		reset_source_set(RESET_RST);
		break;
	case OMAP3_GLOBAL_COLD_RST:
		reset_source_set(RESET_POR);
		break;
	default:
		reset_source_set(RESET_UKWN);
		break;
	}
}

int omap3_init(void)
{
	omap_gpmc_base = (void *)OMAP3_GPMC_BASE;

	restart_handler_register_fn(omap3_restart_soc);

	if (IS_ENABLED(CONFIG_RESET_SOURCE))
		omap3_detect_reset_reason();

	return omap3_bootsource();
}

/* GPMC timing for OMAP3 nand device */
const struct gpmc_config omap3_nand_cfg = {
	.cfg = {
		0x00000000,	/* CONF1 */
		0x00141400,	/* CONF2 */
		0x00141400,	/* CONF3 */
		0x0F010F01,	/* CONF4 */
		0x010C1414,	/* CONF5 */
		0x1F040000 |
		0x00000A80,	/* CONF6 */
	},
	/* GPMC address map as small as possible */
	.base = 0x28000000,
	.size = GPMC_SIZE_16M,
};

#ifndef __PBL__
static int omap3_gpio_init(void)
{
	add_generic_device("omap-gpio", 0, NULL, OMAP3_GPIO1_BASE,
					0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 1, NULL, OMAP3_GPIO2_BASE,
					0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 2, NULL, OMAP3_GPIO3_BASE,
					0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 3, NULL, OMAP3_GPIO4_BASE,
					0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 4, NULL, OMAP3_GPIO5_BASE,
					0xf00, IORESOURCE_MEM, NULL);
	add_generic_device("omap-gpio", 5, NULL, OMAP3_GPIO6_BASE,
					0xf00, IORESOURCE_MEM, NULL);

	return 0;
}

int omap3_devices_init(void)
{
	omap3_gpio_init();
	add_generic_device("omap-32ktimer", 0, NULL, OMAP3_32KTIMER_BASE, 0x400,
			   IORESOURCE_MEM, NULL);
	return 0;
}
#endif
