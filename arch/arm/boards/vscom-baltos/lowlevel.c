#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <io.h>
#include <linux/string.h>
#include <debug_ll.h>
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

static const struct am33xx_ddr_data ddr3_data = {
	.rd_slave_ratio0        = 0x38,
	.wr_dqs_slave_ratio0    = 0x44,
	.fifo_we_slave_ratio0	= 0x94,
	.wr_slave_ratio0        = 0x7D,
	.use_rank0_delay	= 0x01,
	.dll_lock_diff0		= 0x0,
};

static const struct am33xx_cmd_control ddr3_cmd_ctrl = {
	.slave_ratio0	= 0x80,
	.dll_lock_diff0	= 0x1,
	.invert_clkout0	= 0x0,
	.slave_ratio1	= 0x80,
	.dll_lock_diff1	= 0x1,
	.invert_clkout1	= 0x0,
	.slave_ratio2	= 0x80,
	.dll_lock_diff2	= 0x1,
	.invert_clkout2	= 0x0,
};

static const struct am33xx_emif_regs ddr3_regs = {
	.emif_read_latency	= 0x100007,
	.emif_tim1		= 0x0AAAD4DB,
	.emif_tim2		= 0x266B7FDA,
	.emif_tim3		= 0x501F867F,
	.zq_config		= 0x50074BE4,
	.sdram_config		= 0x61C05332,
	.sdram_config2		= 0x0,
	.sdram_ref_ctrl		= 0xC30,
};

static const struct am33xx_ddr_data ddr3_data_256mb = {
	.rd_slave_ratio0	= 0x36,
	.wr_dqs_slave_ratio0	= 0x38,
	.fifo_we_slave_ratio0	= 0x99,
	.wr_slave_ratio0	= 0x73,
};

static const struct am33xx_emif_regs ddr3_regs_256mb = {
	.emif_read_latency	= 0x7,
	.emif_tim1		= 0x0AAAD4DB,
	.emif_tim2		= 0x26437FDA,
	.emif_tim3		= 0x501F83FF,
	.sdram_config		= 0x61C052B2,
	.zq_config		= 0x50074BE4,
	.sdram_ref_ctrl		= 0x00000C30,

};

extern char __dtb_am335x_baltos_minimal_start[];

/**
 * @brief The basic entry point for board initialization.
 *
 * This is called as part of machine init (after arch init).
 * This is again called with stack in SRAM, so not too many
 * constructs possible here.
 *
 * @return void
 */
static noinline void baltos_sram_init(void)
{
	uint32_t sdram_size;
	void *fdt;

	fdt = __dtb_am335x_baltos_minimal_start;

	/* WDT1 is already running when the bootloader gets control
	 * Disable it to avoid "random" resets
	 */
	__raw_writel(WDT_DISABLE_CODE1, AM33XX_WDT_REG(WSPR));
	while (__raw_readl(AM33XX_WDT_REG(WWPS)) != 0x0);
	__raw_writel(WDT_DISABLE_CODE2, AM33XX_WDT_REG(WSPR));
	while (__raw_readl(AM33XX_WDT_REG(WWPS)) != 0x0);

	/* Setup the PLLs and the clocks for the peripherals */
	am33xx_pll_init(MPUPLL_M_600, DDRPLL_M_400);
	am335x_sdram_init(0x18B, &ddr3_cmd_ctrl, &ddr3_regs, &ddr3_data);
	sdram_size = get_ram_size((void *)0x80000000, (1024 << 20));
	if (sdram_size == SZ_256M)
		am335x_sdram_init(0x18B, &ddr3_cmd_ctrl, &ddr3_regs_256mb,
				&ddr3_data_256mb);

	am33xx_uart_soft_reset((void *)AM33XX_UART0_BASE);
	am33xx_enable_uart0_pin_mux();
	omap_uart_lowlevel_init((void *)AM33XX_UART0_BASE);
	putc_ll('>');

	am335x_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_am33xx_baltos_sram, bootinfo, r1, r2)
{
	am33xx_save_bootinfo((void *)bootinfo);

	/*
	 * Setup C environment, the board init code uses global variables.
	 * Stackpointer has already been initialized by the ROM code.
	 */
	relocate_to_current_adr();
	setup_c();

	baltos_sram_init();
}

ENTRY_FUNCTION(start_am33xx_baltos_sdram, r0, r1, r2)
{
	void *fdt;

	/*
	 * Prolong global reset duration to the max. value (0xff)
	 * and leave power domain reset to its default value (0x10).
	 */
	__raw_writel(0x000010ff, AM33XX_PRM_RSTTIME);

	fdt = __dtb_am335x_baltos_minimal_start;

	fdt -= get_runtime_offset();

	am335x_barebox_entry(fdt);
}
