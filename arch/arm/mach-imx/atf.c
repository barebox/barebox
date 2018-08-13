#include <common.h>
#include <mach/atf.h>

/**
 * imx8mq_atf_load_bl31 - Load ATF BL31 blob and transfer control to it
 *
 * @fw:		Pointer to the BL31 blob
 * @fw_size:	Size of the BL31 blob
 *
 * This function:

 *     1. Copies built-in BL31 blob to an address i.MX8M's BL31
 *        expects to be placed
 *
 *     2. Sets up temporary stack pointer for EL2, which is execution
 *        level that BL31 will drop us off at after it completes its
 *        initialization routine
 *
 *     3. Transfers control to BL31
 *
 * NOTE: This function expects NXP's implementation of ATF that can be
 * found at:
 *     https://source.codeaurora.org/external/imx/imx-atf
 *
 * any other implementation may or may not work
 *
 */
void imx8mq_atf_load_bl31(const void *fw, size_t fw_size)
{
	void __noreturn (*bl31)(void) = (void *)MX8MQ_ATF_BL31_BASE_ADDR;

	if (WARN_ON(fw_size > MX8MQ_ATF_BL31_SIZE_LIMIT))
		return;

	memcpy(bl31, fw, fw_size);

	asm volatile("msr sp_el2, %0" : :
		     "r" (MX8MQ_ATF_BL33_BASE_ADDR - 16) :
		     "cc");
	bl31();
}