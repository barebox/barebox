#include <common.h>
#include <sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

void __naked barebox_arm_reset_vector(void)
{
	arm_cpu_lowlevel_init();
	barebox_arm_entry(0x10000000, SZ_2G, 0);
}
