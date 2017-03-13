#include <linux/stringify.h>
#include <mach/common.h>

static inline void __barebox_arm_head(void)
{
	__asm__ __volatile__ (
#ifdef CONFIG_THUMB2_BAREBOX
		".arm\n"
		"adr r9, 1f + 1\n"
		"bx r9\n"
		".thumb\n"
		"1:\n"
		"bl 2f\n"
		".rept 10\n"
		"1: b 1b\n"
		".endr\n"
#else
		"b 2f\n"
		"1: b 1b\n"
		"1: b 1b\n"
		"1: b 1b\n"
		"1: b 1b\n"
		"1: b 1b\n"
		"1: b 1b\n"
		"1: b 1b\n"
#endif
		".asciz \"barebox\"\n"
		".word _text\n"				/* text base. If copied there,
							 * barebox can skip relocation
							 */
		".word _barebox_image_size\n"		/* image size to copy */

		/*
		 * The following entry (at offset 0x30) is the only intended
		 * difference to the original arm __barebox_arm_head. This value
		 * holds the address of the internal register window when the
		 * image is started. If the window is not at the reset default
		 * position any more the caller can pass the actual value here.
		 */
		".word " __stringify(MVEBU_BOOTUP_INT_REG_BASE) "\n"
		".rept 7\n"
		".word 0x55555555\n"
		".endr\n"
		"2:\n"
	);
}

static inline void barebox_arm_head(void)
{
	__barebox_arm_head();
	__asm__ __volatile__ (
		"b barebox_arm_reset_vector\n"
	);
}
