#include <common.h>
#include <sizes.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/s3c-iomap.h>

void __naked barebox_arm_reset_vector(void)
{
	arm_cpu_lowlevel_init();
	barebox_arm_entry(S3C_SDRAM_BASE, SZ_128M, 0);
}
