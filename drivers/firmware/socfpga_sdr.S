#include <linux/linkage.h>

	.arch	armv7-a
	.arm

/*
 * r0 : sdram controller staticcfg
 */

ENTRY(socfpga_sdram_apply_static_cfg)
	push {ip,lr}
	ldr r1, [r0]
	orr r1, r1, #8
	str r1, [r0]
	pop {ip,pc}
	.align
ENDPROC(socfpga_sdram_apply_static_cfg)

ENTRY(socfpga_sdram_apply_static_cfg_sz)
	.word . - socfpga_sdram_apply_static_cfg;
