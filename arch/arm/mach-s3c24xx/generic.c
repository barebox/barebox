/*
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
/**
 * @file
 * @brief Basic clock, sdram and timer handling for S3C24xx CPUs
 */

#include <config.h>
#include <common.h>
#include <init.h>
#include <clock.h>
#include <asm/io.h>
#include <mach/s3c24x0-iomap.h>

/**
 * Calculate the current M-PLL clock.
 * @return Current frequency in Hz
 */
uint32_t s3c24xx_get_mpllclk(void)
{
	uint32_t m, p, s, reg_val;

	reg_val = readl(MPLLCON);
	m = ((reg_val & 0xFF000) >> 12) + 8;
	p = ((reg_val & 0x003F0) >> 4) + 2;
	s = reg_val & 0x3;
#ifdef CONFIG_CPU_S3C2410
	return (S3C24XX_CLOCK_REFERENCE * m) / (p << s);
#endif
#ifdef CONFIG_CPU_S3C2440
	return 2 * m * (S3C24XX_CLOCK_REFERENCE / (p << s));
#endif
}

/**
 * Calculate the current U-PLL clock
 * @return Current frequency in Hz
 */
uint32_t s3c24xx_get_upllclk(void)
{
	uint32_t m, p, s, reg_val;

	reg_val = readl(UPLLCON);
	m = ((reg_val & 0xFF000) >> 12) + 8;
	p = ((reg_val & 0x003F0) >> 4) + 2;
	s = reg_val & 0x3;

	return (S3C24XX_CLOCK_REFERENCE * m) / (p << s);
}

/**
 * Calculate the FCLK frequency used for the ARM CPU core
 * @return Current frequency in Hz
 */
uint32_t s3c24xx_get_fclk(void)
{
	return s3c24xx_get_mpllclk();
}

/**
 * Calculate the HCLK frequency used for the AHB bus (CPU to main peripheral)
 * @return Current frequency in Hz
 */
uint32_t s3c24xx_get_hclk(void)
{
	uint32_t f_clk;

	f_clk = s3c24xx_get_fclk();
#ifdef CONFIG_CPU_S3C2410
	if (readl(CLKDIVN) & 0x02)
		return f_clk >> 1;
#endif
#ifdef CONFIG_CPU_S3C2440
	switch(readl(CLKDIVN) & 0x06) {
	case 2:
		return f_clk >> 1;
	case 4:
		return f_clk >> 2;	/* TODO consider CAMDIVN */
	case 6:
		return f_clk / 3;	/* TODO consider CAMDIVN */
	}
#endif
	return f_clk;
}

/**
 * Calculate the PCLK frequency used for the slower peripherals
 * @return Current frequency in Hz
 */
uint32_t s3c24xx_get_pclk(void)
{
	uint32_t p_clk;

	p_clk = s3c24xx_get_hclk();
	if (readl(CLKDIVN) & 0x01)
		return p_clk >> 1;
	return p_clk;
}

/**
 * Calculate the UCLK frequency used by the USB host device
 * @return Current frequency in Hz
 */
uint32_t s3c24xx_get_uclk(void)
{
    return s3c24xx_get_upllclk();
}

/**
 * Calculate the amount of connected and available memory
 * @return Memory size in bytes
 */
uint32_t s3c24x0_get_memory_size(void)
{
	uint32_t reg, size;

	/*
	 * detect the current memory size
	 */
	reg = readl(BANKSIZE);

	switch (reg & 0x7) {
	case 0:
		size = 32 * 1024 * 1024;
		break;
	case 1:
		size = 64 * 1024 * 1024;
		break;
	case 2:
		size = 128 * 1024 * 1024;
		break;
	case 4:
		size = 2 * 1024 * 1024;
		break;
	case 5:
		size = 4 * 1024 * 1024;
		break;
	case 6:
		size = 8 * 1024 * 1024;
		break;
	default:
		size = 16 * 1024 * 1024;
		break;
	}

	/*
	 * Is bank7 also configured for SDRAM usage?
	 */
	if ((readl(BANKCON7) & (0x3 << 15)) == (0x3 << 15))
		size <<= 1;	/* also count this bank */

	return size;
}

/**
 * Show the user the current clock settings
 */
int s3c24xx_dump_clocks(void)
{
	printf("refclk:  %7d kHz\n", S3C24XX_CLOCK_REFERENCE / 1000);
	printf("mpll:    %7d kHz\n", s3c24xx_get_mpllclk() / 1000);
	printf("upll:    %7d kHz\n", s3c24xx_get_upllclk() / 1000);
	printf("fclk:    %7d kHz\n", s3c24xx_get_fclk() / 1000);
	printf("hclk:    %7d kHz\n", s3c24xx_get_hclk() / 1000);
	printf("pclk:    %7d kHz\n", s3c24xx_get_pclk() / 1000);
	printf("SDRAM1:   CL%d@%dMHz\n", ((readl(BANKCON6) & 0xc) >> 2) + 2, s3c24xx_get_hclk() / 1000000);
	if ((readl(BANKCON7) & (0x3 << 15)) == (0x3 << 15))
		printf("SDRAM2:   CL%d@%dMHz\n", ((readl(BANKCON7) & 0xc) >> 2) + 2,
			s3c24xx_get_hclk() / 1000000);
	return 0;
}

late_initcall(s3c24xx_dump_clocks);

static uint64_t s3c24xx_clocksource_read(void)
{
	/* note: its a down counter */
	return 0xFFFF - readw(TCNTO4);
}

static struct clocksource cs = {
	.read	= s3c24xx_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(16),
	.shift	= 10,
};

static int clocksource_init (void)
{
	uint32_t p_clk = s3c24xx_get_pclk();

	writel(0x00000000, TCON);	/* stop all timers */
	writel(0x00ffffff, TCFG0);	/* PCLK / (255 + 1) for timer 4 */
	writel(0x00030000, TCFG1);	/* /16 */

	writew(0xffff, TCNTB4);		/* reload value is TOP */

	writel(0x00600000, TCON);	/* force a first reload */
	writel(0x00400000, TCON);
	writel(0x00500000, TCON);	/* enable timer 4 with auto reload */

	cs.mult = clocksource_hz2mult(p_clk / ((255 + 1) * 16), cs.shift);
	init_clock(&cs);

	return 0;
}
core_initcall(clocksource_init);

void __noreturn reset_cpu(unsigned long addr)
{
	/* Disable watchdog */
	writew(0x0000, WTCON);

	/* Initialize watchdog timer count register */
	writew(0x0001, WTCNT);

	/* Enable watchdog timer; assert reset at timer timeout */
	writew(0x0021, WTCON);

	/* loop forever and wait for reset to happen */
	while(1)
		;
}
EXPORT_SYMBOL(reset_cpu);

/**

@page dev_s3c24xx_arch Samsung's S3C24xx Platforms in barebox

@section s3c24xx_boards Boards using S3C24xx Processors

@li @subpage arch/arm/boards/a9m2410/a9m2410.c
@li @subpage arch/arm/boards/a9m2440/a9m2440.c

@section s3c24xx_arch Documentation for S3C24xx Architectures Files

@li @subpage arch/arm/mach-s3c24xx/generic.c

@section s3c24xx_mem_map SDRAM Memory Map

SDRAM starts at address 0x3000.0000 up to the available amount of connected
SDRAM memory. Physically this CPU can handle up to 256MiB (two areas with
up to 128MiB each).

@subsection s3c24xx_mem_generic_map Generic Map
- 0x0000.0000 Start of the internal SRAM when booting from NAND flash memory or CS signal to a NOR flash memory.
- 0x0800.0000 Start of I/O space.
- 0x3000.0000 Start of SDRAM area.
  - 0x3000.0100 Start of the TAG list area.
  - 0x3000.8000 Start of the linux kernel (physical address).
- 0x4000.0000 Start of internal SRAM, when booting from NOR flash memory
- 0x4800.0000 Start of the internal I/O area

@section s3c24xx_asm_arm include/asm-arm/arch-s3c24xx directory guidelines
All S3C24xx common headers are located here.

@note Do not add board specific header files/information here.
*/

/** @page dev_s3c24xx_mach Samsung's S3C24xx based platforms

@par barebox Map

The location of the @a barebox itself depends on the available amount of
installed SDRAM memory:

- 0x30fc.0000 Start of @a barebox when 16MiB SDRAM is available
- 0x31fc.0000 Start of @a barebox when 32MiB SDRAM is available
- 0x33fc.0000 Start of @a barebox when 64MiB SDRAM is available

Adjust the @p CONFIG_TEXT_BASE/CONFIG_ARCH_TEXT_BASE symbol in accordance to
the available memory.

@note The RAM based filesystem and the stack resides always below the
@a barebox start address.

@li @subpage dev_s3c24xx_wd_handling
@li @subpage dev_s3c24xx_pll_handling
@li @subpage dev_s3c24xx_sdram_handling
@li @subpage dev_s3c24xx_nandboot_handling
*/
