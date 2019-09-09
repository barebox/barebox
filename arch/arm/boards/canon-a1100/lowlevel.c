#include <common.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

extern char __dtb_canon_a1100_start[];

void __naked barebox_arm_reset_vector(uint32_t r0, uint32_t r1, uint32_t r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	fdt = __dtb_canon_a1100_start + get_runtime_offset();

	/* FIXME: can we determine RAM size using CP15 register?
	 *
	 * see http://chdk.setepontos.com/index.php?topic=5980.90
	 *
	 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0363e/Bgbcdeca.html
	 * 4.2.19. c6, MPU memory region programming registers
	 *
	 * But the 'cpuinfo' command says that the Protection
	 * unit is disabled.
	 * The Control Register value (mrc    p15, 0, %0, c0, c1, 4)
	 * is 0x00051078.
	 */

	barebox_arm_entry(0x0, SZ_64M, fdt);
}
