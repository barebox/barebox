#include <common.h>
#include <sizes.h>
#include <io.h>
#include <init.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/am33xx-silicon.h>
#include <mach/am33xx-clock.h>
#include <mach/generic.h>
#include <mach/sdrc.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/am33xx-mux.h>
#include <mach/am33xx-generic.h>
#include <mach/wdt.h>
#include <debug_ll.h>

static const struct am33xx_cmd_control pcm051_cmd = {
	.slave_ratio0 = 0x80,
	.dll_lock_diff0 = 0x0,
	.invert_clkout0 = 0x0,
	.slave_ratio1 = 0x80,
	.dll_lock_diff1 = 0x0,
	.invert_clkout1 = 0x0,
	.slave_ratio2 = 0x80,
	.dll_lock_diff2 = 0x0,
	.invert_clkout2 = 0x0,
};

struct pcm051_sdram_timings {
	struct am33xx_emif_regs regs;
	struct am33xx_ddr_data data;
};

enum {
	MT41J128M16125IT_256MB,
	MT41J64M1615IT_128MB,
	MT41J256M16HA15EIT_512MB,
	MT41J512M8125IT_2x512MB,
};

struct pcm051_sdram_timings timings[] = {
	/* 256MB */
	[MT41J128M16125IT_256MB] = {
		.regs = {
			.emif_read_latency	= 0x7,
			.emif_tim1		= 0x0AAAD4DB,
			.emif_tim2		= 0x26437FDA,
			.emif_tim3		= 0x501F83FF,
			.sdram_config		= 0x61C052B2,
			.zq_config		= 0x50074BE4,
			.sdram_ref_ctrl		= 0x00000C30,
		},
		.data = {
			.rd_slave_ratio0	= 0x3B,
			.wr_dqs_slave_ratio0	= 0x33,
			.fifo_we_slave_ratio0	= 0x9c,
			.wr_slave_ratio0	= 0x6f,
		},
	},

	/* 128MB */
	[MT41J64M1615IT_128MB] = {
		.regs =  {
			.emif_read_latency	= 0x7,
			.emif_tim1		= 0x0AAAE4DB,
			.emif_tim2		= 0x262F7FDA,
			.emif_tim3		= 0x501F82BF,
			.sdram_config		= 0x61C05232,
			.zq_config		= 0x50074BE4,
			.sdram_ref_ctrl		= 0x00000C30,
		},
		.data = {
			.rd_slave_ratio0	= 0x38,
			.wr_dqs_slave_ratio0	= 0x34,
			.fifo_we_slave_ratio0	= 0xA2,
			.wr_slave_ratio0	= 0x72,
		},
	},

	/* 512MB */
	[MT41J256M16HA15EIT_512MB] = {
		.regs = {
			.emif_read_latency	= 0x7,
			.emif_tim1		= 0x0AAAE4DB,
			.emif_tim2		= 0x266B7FDA,
			.emif_tim3		= 0x501F867F,
			.sdram_config		= 0x61C05332,
			.zq_config		= 0x50074BE4,
			.sdram_ref_ctrl		= 0x00000C30
		},
		.data = {
			.rd_slave_ratio0	= 0x35,
			.wr_dqs_slave_ratio0	= 0x43,
			.fifo_we_slave_ratio0	= 0x97,
			.wr_slave_ratio0	= 0x7b,
		},
	},

	/* 1024MB */
	[MT41J512M8125IT_2x512MB] = {
		.regs = {
			.emif_read_latency	= 0x7,
			.emif_tim1		= 0x0AAAE4DB,
			.emif_tim2		= 0x266B7FDA,
			.emif_tim3		= 0x501F867F,
			.sdram_config		= 0x61C053B2,
			.zq_config		= 0x50074BE4,
			.sdram_ref_ctrl		= 0x00000C30
		},
		.data = {
			.rd_slave_ratio0	= 0x32,
			.wr_dqs_slave_ratio0	= 0x48,
			.fifo_we_slave_ratio0	= 0x99,
			.wr_slave_ratio0	= 0x80,
		},
	},
};

extern char __dtb_am335x_phytec_phycore_start[];

/**
 * @brief The basic entry point for board initialization.
 *
 * This is called as part of machine init (after arch init).
 * This is again called with stack in SRAM, so not too many
 * constructs possible here.
 *
 * @return void
 */
static noinline void pcm051_board_init(int sdram)
{
	void *fdt;
	struct pcm051_sdram_timings *timing = &timings[sdram];

	/* WDT1 is already running when the bootloader gets control
	 * Disable it to avoid "random" resets
	 */
	writel(WDT_DISABLE_CODE1, AM33XX_WDT_REG(WSPR));
	while (readl(AM33XX_WDT_REG(WWPS)) != 0x0);

	writel(WDT_DISABLE_CODE2, AM33XX_WDT_REG(WSPR));
	while (readl(AM33XX_WDT_REG(WWPS)) != 0x0);

	am33xx_pll_init(MPUPLL_M_600, 25, DDRPLL_M_400);

	am335x_sdram_init(0x18B, &pcm051_cmd,
			&timing->regs,
			&timing->data);

	am33xx_uart_soft_reset((void *)AM33XX_UART0_BASE);
	am33xx_enable_uart0_pin_mux();
	omap_uart_lowlevel_init((void *)AM33XX_UART0_BASE);
	putc_ll('>');

	fdt = __dtb_am335x_phytec_phycore_start - get_runtime_offset();

	am335x_barebox_entry(fdt);
}

static noinline void pcm051_board_entry(unsigned long bootinfo, int sdram)
{
	am33xx_save_bootinfo((void *)bootinfo);

	arm_cpu_lowlevel_init();

	/*
	 * Setup C environment, the board init code uses global variables.
	 * Stackpointer has already been initialized by the ROM code.
	 */
	relocate_to_current_adr();
	setup_c();

	pcm051_board_init(sdram);
}

ENTRY_FUNCTION(start_am33xx_phytec_phycore_sram_256mb, bootinfo, r1, r2)
{
	pcm051_board_entry(bootinfo, MT41J128M16125IT_256MB);
}

ENTRY_FUNCTION(start_am33xx_phytec_phycore_sram_128mb, bootinfo, r1, r2)
{
	pcm051_board_entry(bootinfo, MT41J64M1615IT_128MB);
}

ENTRY_FUNCTION(start_am33xx_phytec_phycore_sram_512mb, bootinfo, r1, r2)
{
	pcm051_board_entry(bootinfo, MT41J256M16HA15EIT_512MB);
}

ENTRY_FUNCTION(start_am33xx_phytec_phycore_sram_2x512mb, bootinfo, r1, r2)
{
	pcm051_board_entry(bootinfo, MT41J512M8125IT_2x512MB);
}

ENTRY_FUNCTION(start_am33xx_phytec_phycore_sdram, r0, r1, r2)
{
	void *fdt;

	fdt = __dtb_am335x_phytec_phycore_start - get_runtime_offset();

	am335x_barebox_entry(fdt);
}
