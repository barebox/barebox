#ifndef __MACH_ARM_HEAD_H
#define __MACH_ARM_HEAD_H

#ifdef CONFIG_AT91_LOAD_BAREBOX_SRAM
#define AT91_EXV6	".word _barebox_image_size\n"
#else
#define AT91_EXV6	".word _barebox_bare_init_size\n"
#endif

static inline void __barebox_arm_head(void)
{
	__asm__ __volatile__ (
#ifdef CONFIG_THUMB2_BAREBOX
#error Thumb2 is not supported
#else
		"b 2f\n"
		"1: b 1b\n"
		"1: b 1b\n"
		"1: b 1b\n"
		"1: b 1b\n"
		AT91_EXV6				/* image size to load by the bootrom */
		"1: b 1b\n"
		"1: b 1b\n"
#endif
		".asciz \"barebox\"\n"
		".word _text\n"				/* text base. If copied there,
							 * barebox can skip relocation
							 */
		".word _barebox_image_size\n"		/* image size to copy */
		".rept 8\n"
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

#endif /* __MACH_ARM_HEAD_H */
