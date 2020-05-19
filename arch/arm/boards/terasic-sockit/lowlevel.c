#include "sdram_config.h"
#include "pinmux_config.c"
#include "pll_config.h"
#include "sequencer_defines.h"
#include "sequencer_auto.h"
#include "sequencer_auto_inst_init.c"
#include "sequencer_auto_ac_init.c"
#include "iocsr_config_cyclone5.c"

#include <mach/lowlevel.h>

static inline void ledon(int led)
{
	u32 val;

	val = readl(0xFF709000);
	val |= 1 << (led + 24);
	writel(val, 0xFF709000);

	val = readl(0xFF709004);
	val |= 1 << (led + 24);
	writel(val, 0xFF709004);
}

static inline void ledoff(int led)
{
	u32 val;

	val = readl(0xFF709000);
	val &= ~(1 << (led + 24));
	writel(val, 0xFF709000);

	val = readl(0xFF709004);
	val &= ~(1 << (led + 24));
	writel(val, 0xFF709004);
}

SOCFPGA_C5_ENTRY(start_socfpga_sockit, socfpga_cyclone5_sockit, SZ_1G);
SOCFPGA_C5_XLOAD_ENTRY(start_socfpga_sockit_xload, SZ_1G);
