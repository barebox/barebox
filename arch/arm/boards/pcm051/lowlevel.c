#include <common.h>
#include <sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

void __naked reset(void)
{
	common_reset();

	barebox_arm_entry(0x80000000, SZ_512M, 0);
}
