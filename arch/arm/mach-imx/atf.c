// SPDX-License-Identifier: GPL-2.0-only

#include <asm/sections.h>
#include <common.h>
#include <firmware.h>
#include <mach/atf.h>
#include <mach/generic.h>
#include <mach/xload.h>
#include <mach/romapi.h>
#include <soc/fsl/fsl_udc.h>

/**
 * imx8m_atf_load_bl31 - Load ATF BL31 blob and transfer control to it
 *
 * @fw:		Pointer to the BL31 blob
 * @fw_size:	Size of the BL31 blob
 * @atf_dest:	Place where the BL31 is copied to and executed
 *
 * This function:
 *
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

static void imx8m_atf_load_bl31(const void *fw, size_t fw_size, void *atf_dest)
{
	void __noreturn (*bl31)(void) = atf_dest;

	if (WARN_ON(fw_size > MX8M_ATF_BL31_SIZE_LIMIT))
		return;

	memcpy(bl31, fw, fw_size);

	asm volatile("msr sp_el2, %0" : :
		     "r" (atf_dest - 16) :
		     "cc");
	bl31();
}

void imx8mm_atf_load_bl31(const void *fw, size_t fw_size)
{
	imx8m_atf_load_bl31(fw, fw_size, (void *)MX8MM_ATF_BL31_BASE_ADDR);
}

void imx8mn_atf_load_bl31(const void *fw, size_t fw_size)
{
	imx8m_atf_load_bl31(fw, fw_size, (void *)MX8MN_ATF_BL31_BASE_ADDR);
}

void imx8mp_atf_load_bl31(const void *fw, size_t fw_size)
{
	imx8m_atf_load_bl31(fw, fw_size, (void *)MX8MP_ATF_BL31_BASE_ADDR);
}

void imx8mq_atf_load_bl31(const void *fw, size_t fw_size)
{
	imx8m_atf_load_bl31(fw, fw_size, (void *)MX8MQ_ATF_BL31_BASE_ADDR);
}

void imx8mm_load_and_start_image_via_tfa(void)
{
	size_t bl31_size;
	const u8 *bl31;
	enum bootsource src;
	int instance;
	void *bl33 = (void *)MX8M_ATF_BL33_BASE_ADDR;

	imx8mm_get_boot_source(&src, &instance);
	switch (src) {
	case BOOTSOURCE_MMC:
		imx8m_esdhc_load_image(instance, false);
		break;
	case BOOTSOURCE_SERIAL:
		/*
		 * Traditionally imx-usb-loader sends the PBL twice. The first
		 * PBL is loaded to OCRAM and executed. Then the full barebox
		 * image including the PBL is sent again and received here. We
		 * might change that in the future in imx-usb-loader so that the
		 * PBL is sent only once and we only receive the rest of the
		 * image here. To prepare that step we check if we get a full
		 * barebox image or piggydata only. When it's piggydata only move
		 * it to the place where it would be if it would have been a
		 * full image.
		 */
		imx8mm_barebox_load_usb(bl33);

		if (!strcmp("barebox", bl33 + 0x20)) {
			/* Found the barebox marker, so this is a PBL + piggydata */
			pr_debug("received PBL + piggydata\n");
		} else {
			/* no barebox marker, so this is piggydata only */
			pr_debug("received piggydata\n");
			memmove(bl33 + barebox_pbl_size, bl33,
				barebox_image_size - barebox_pbl_size);
		}

		break;
	default:
		printf("Unhandled bootsource BOOTSOURCE_%d\n", src);
		hang();
	}

	/*
	 * On completion the TF-A will jump to MX8M_ATF_BL33_BASE_ADDR
	 * in EL2. Copy the image there, but replace the PBL part of
	 * that image with ourselves. On a high assurance boot only the
	 * currently running code is validated and contains the checksum
	 * for the piggy data, so we need to ensure that we are running
	 * the same code in DRAM.
	 */
	memcpy(bl33, __image_start, barebox_pbl_size);

	get_builtin_firmware(imx8mm_bl31_bin, &bl31, &bl31_size);

	imx8mm_atf_load_bl31(bl31, bl31_size);

	/* not reached */
}

void imx8mp_load_and_start_image_via_tfa(void)
{
	size_t bl31_size;
	const u8 *bl31;
	enum bootsource src;
	int instance;

	imx8mp_get_boot_source(&src, &instance);
	switch (src) {
	case BOOTSOURCE_MMC:
		imx8mp_esdhc_load_image(instance, false);
		break;
	case BOOTSOURCE_SERIAL:
		imx8mp_bootrom_load_image();
		break;
	default:
		printf("Unhandled bootsource BOOTSOURCE_%d\n", src);
		hang();
	}


	/*
	 * On completion the TF-A will jump to MX8M_ATF_BL33_BASE_ADDR
	 * in EL2. Copy the image there, but replace the PBL part of
	 * that image with ourselves. On a high assurance boot only the
	 * currently running code is validated and contains the checksum
	 * for the piggy data, so we need to ensure that we are running
	 * the same code in DRAM.
	 */
	memcpy((void *)MX8M_ATF_BL33_BASE_ADDR,
	       __image_start, barebox_pbl_size);

	get_builtin_firmware(imx8mp_bl31_bin, &bl31, &bl31_size);

	imx8mp_atf_load_bl31(bl31, bl31_size);

	/* not reached */
}

void imx8mn_load_and_start_image_via_tfa(void)
{
	size_t bl31_size;
	const u8 *bl31;
	enum bootsource src;
	int instance;

	imx8mn_get_boot_source(&src, &instance);
	switch (src) {
	case BOOTSOURCE_MMC:
		imx8mn_esdhc_load_image(instance, false);
		break;
	default:
		printf("Unhandled bootsource BOOTSOURCE_%d\n", src);
		hang();
	}


	/*
	 * On completion the TF-A will jump to MX8M_ATF_BL33_BASE_ADDR
	 * in EL2. Copy the image there, but replace the PBL part of
	 * that image with ourselves. On a high assurance boot only the
	 * currently running code is validated and contains the checksum
	 * for the piggy data, so we need to ensure that we are running
	 * the same code in DRAM.
	 */
	memcpy((void *)MX8M_ATF_BL33_BASE_ADDR,
	       __image_start, barebox_pbl_size);

	get_builtin_firmware(imx8mn_bl31_bin, &bl31, &bl31_size);

	imx8mn_atf_load_bl31(bl31, bl31_size);

	/* not reached */
}
