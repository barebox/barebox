/*
 * Generate definitions needed by assembly language modules.
 * This code generates raw asm output which is post-processed to extract
 * and format the required data.
 */

#include <linux/kbuild.h>

#ifdef CONFIG_EFI_BOOTUP
int main(void)
{
	return 0;
}
#else
void common(void)
{
}
#endif
