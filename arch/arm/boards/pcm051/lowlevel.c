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

/**
 * @brief The basic entry point for board initialization.
 *
 * This is called as part of machine init (after arch init).
 * This is again called with stack in SRAM, so not too many
 * constructs possible here.
 *
 * @return void
 */
static int pcm051_board_init(void)
{
	/* WDT1 is already running when the bootloader gets control
	 * Disable it to avoid "random" resets
	 */
	writel(WDT_DISABLE_CODE1, AM33XX_WDT_REG(WSPR));
	while (readl(AM33XX_WDT_REG(WWPS)) != 0x0);

	writel(WDT_DISABLE_CODE2, AM33XX_WDT_REG(WSPR));
	while (readl(AM33XX_WDT_REG(WWPS)) != 0x0);

	if (running_in_sdram())
		return 0;

	pll_init(MPUPLL_M_600, 25, DDRPLL_M_303);

	am335x_sdram_init(0x18B, &MT41J256M8HX15E_2x256M8_cmd,
			&MT41J256M8HX15E_2x256M8_regs,
			&MT41J256M8HX15E_2x256M8_data);

	am33xx_uart0_soft_reset();
	am33xx_enable_uart0_pin_mux();

	return 0;
}

void __naked __bare_init barebox_arm_reset_vector(uint32_t *data)
{
	am33xx_save_bootinfo(data);

	arm_cpu_lowlevel_init();

	pcm051_board_init();

	barebox_arm_entry(0x80000000, SZ_512M, 0);
}
