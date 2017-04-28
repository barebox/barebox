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

		".rept 10\n"
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
