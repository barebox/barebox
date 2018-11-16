#define SECT(name) __attribute__((section("socfpga_socdk_" #name))) name

#include "sdram_config.h"
#include "pinmux_config.c"
#include "pll_config.h"
#include "sequencer_defines.h"
#include "sequencer_auto.h"
#include "sequencer_auto_inst_init.c"
#include "sequencer_auto_ac_init.c"
#include "iocsr_config_cyclone5.c"

#include <mach/lowlevel.h>

SOCFPGA_C5_ENTRY(start_socfpga_socdk, socfpga_cyclone5_socdk, SZ_1G);
SOCFPGA_C5_XLOAD_ENTRY(start_socfpga_socdk_xload, SZ_1G);
