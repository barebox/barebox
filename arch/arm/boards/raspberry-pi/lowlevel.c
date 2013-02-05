#include <common.h>
#include <sizes.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/platform.h>

void __naked barebox_arm_reset_vector(void)
{
        arm_cpu_lowlevel_init();
        barebox_arm_entry(BCM2835_SDRAM_BASE, SZ_128M, 0);
}
