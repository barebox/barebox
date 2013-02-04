#include <common.h>
#include <mach/esdctl.h>
#include <asm/barebox-arm-head.h>

void __naked reset(void)
{
	common_reset();
	imx53_barebox_entry(0);
}
