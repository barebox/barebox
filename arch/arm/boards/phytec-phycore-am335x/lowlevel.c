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

static const struct am33xx_cmd_control MT41J256M8HX15E_2x256M8_cmd = {
	.slave_ratio0 = 0x40,
	.dll_lock_diff0 = 0x0,
	.invert_clkout0 = 0x1,
	.slave_ratio1 = 0x40,
	.dll_lock_diff1 = 0x0,
	.invert_clkout1 = 0x1,
	.slave_ratio2 = 0x40,
	.dll_lock_diff2 = 0x0,
	.invert_clkout2 = 0x1,
};

static const struct am33xx_emif_regs MT41J256M8HX15E_2x256M8_regs = {
	.emif_read_latency	= 0x6,
	.emif_tim1		= 0x0668A39B,
	.emif_tim2		= 0x26337FDA,
	.emif_tim3		= 0x501F830F,
	.sdram_config		= 0x61C04832,
	.zq_config		= 0x50074BE4,
	.sdram_ref_ctrl		= 0x0000093B,
};

static const struct am33xx_ddr_data MT41J256M8HX15E_2x256M8_data = {
	.rd_slave_ratio0	= 0x3B,
	.wr_dqs_slave_ratio0	= 0x85,
	.fifo_we_slave_ratio0	= 0x100,
	.wr_slave_ratio0	= 0xC1,
	.use_rank0_delay	= 0x01,
	.dll_lock_diff0		= 0x0,
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
static noinline void pcm051_board_init(void)
{
	void *fdt;

	/* WDT1 is already running when the bootloader gets control
	 * Disable it to avoid "random" resets
	 */
	writel(WDT_DISABLE_CODE1, AM33XX_WDT_REG(WSPR));
	while (readl(AM33XX_WDT_REG(WWPS)) != 0x0);

	writel(WDT_DISABLE_CODE2, AM33XX_WDT_REG(WSPR));
	while (readl(AM33XX_WDT_REG(WWPS)) != 0x0);

	am33xx_pll_init(MPUPLL_M_600, 25, DDRPLL_M_303);

	am335x_sdram_init(0x18B, &MT41J256M8HX15E_2x256M8_cmd,
			&MT41J256M8HX15E_2x256M8_regs,
			&MT41J256M8HX15E_2x256M8_data);

	am33xx_uart_soft_reset((void *)AM33XX_UART0_BASE);
	am33xx_enable_uart0_pin_mux();
	omap_uart_lowlevel_init((void *)AM33XX_UART0_BASE);
	putc_ll('>');

	fdt = __dtb_am335x_phytec_phycore_start - get_runtime_offset();

	barebox_arm_entry(0x80000000, SZ_512M, fdt);
}

ENTRY_FUNCTION(start_am33xx_phytec_phycore_sram, bootinfo, r1, r2)
{
	am33xx_save_bootinfo((void *)bootinfo);

	arm_cpu_lowlevel_init();

	/*
	 * Setup C environment, the board init code uses global variables.
	 * Stackpointer has already been initialized by the ROM code.
	 */
	relocate_to_current_adr();
	setup_c();

	pcm051_board_init();
}

ENTRY_FUNCTION(start_am33xx_phytec_phycore_sdram, r0, r1, r2)
{
	void *fdt;

	fdt = __dtb_am335x_phytec_phycore_start - get_runtime_offset();

	barebox_arm_entry(0x80000000, SZ_512M, fdt);
}
