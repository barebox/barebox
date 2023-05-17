// SPDX-License-Identifier: GPL-2.0-only

#include <asm/sections.h>
#include <common.h>
#include <firmware.h>
#include <mach/imx/atf.h>
#include <mach/imx/generic.h>
#include <mach/imx/xload.h>
#include <mach/imx/romapi.h>
#include <mach/imx/esdctl.h>
#include <asm-generic/memory_layout.h>
#include <asm/barebox-arm.h>
#include <mach/imx/imx8m-regs.h>
#include <soc/fsl/fsl_udc.h>
#include <soc/fsl/caam.h>

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
 *        expects to be placed (TF-A v2.8+ is position-independent)
 *
 *     2. Sets up temporary stack pointer for EL2, which is execution
 *        level that BL31 will drop us off at after it completes its
 *        initialization routine
 *
 *     3. Transfers control to BL31
 */

static __noreturn void imx8m_atf_start_bl31(const void *fw, size_t fw_size, void *atf_dest)
{
	void __noreturn (*bl31)(void) = atf_dest;
	int ret;

	BUG_ON(fw_size > MX8M_ATF_BL31_SIZE_LIMIT);

	if (IS_ENABLED(CONFIG_FSL_CAAM_RNG_PBL_INIT)) {
		ret = imx_early_caam_init(MX8M_CAAM_BASE_ADDR);
		if (ret)
			pr_debug("CAAM early init failed: %d\n", ret);
		else
			pr_debug("CAAM early init successful\n");
	}

	memcpy(bl31, fw, fw_size);

	asm volatile("msr sp_el2, %0" : :
		     "r" (atf_dest - 16) :
		     "cc");
	bl31();
	__builtin_unreachable();
}

__noreturn void imx8mm_atf_load_bl31(const void *fw, size_t fw_size)
{
	imx8m_atf_start_bl31(fw, fw_size, (void *)MX8MM_ATF_BL31_BASE_ADDR);
}

__noreturn void imx8mn_atf_load_bl31(const void *fw, size_t fw_size)
{
	imx8m_atf_start_bl31(fw, fw_size, (void *)MX8MN_ATF_BL31_BASE_ADDR);
}

__noreturn void imx8mp_atf_load_bl31(const void *fw, size_t fw_size)
{
	imx8m_atf_start_bl31(fw, fw_size, (void *)MX8MP_ATF_BL31_BASE_ADDR);
}

__noreturn void imx8mq_atf_load_bl31(const void *fw, size_t fw_size)
{
	imx8m_atf_start_bl31(fw, fw_size, (void *)MX8MQ_ATF_BL31_BASE_ADDR);
}

void imx8mm_load_bl33(void *bl33)
{
	enum bootsource src;
	int instance;

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
	case BOOTSOURCE_SPI:
		imx8mm_qspi_load_image(instance, false);
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
}

__noreturn void imx8mm_load_and_start_image_via_tfa(void)
{
	void *bl33 = (void *)MX8M_ATF_BL33_BASE_ADDR;
	unsigned long endmem = MX8M_DDR_CSD1_BASE_ADDR + imx8m_barebox_earlymem_size(32);

	imx8m_save_bootrom_log((void *)arm_mem_scratch(endmem));
	imx8mm_load_bl33(bl33);

	if (IS_ENABLED(CONFIG_FIRMWARE_IMX8MM_OPTEE))
		imx8m_load_and_start_optee_via_tfa(imx8mm, (void *)arm_mem_optee(endmem), bl33);
	else
		imx8mm_load_and_start_tfa(imx8mm_bl31_bin);
}

void imx8mp_load_bl33(void *bl33)
{
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
	case BOOTSOURCE_SPI:
		imx8mp_qspi_load_image(instance, false);
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
}

__noreturn void imx8mp_load_and_start_image_via_tfa(void)
{
	void *bl33 = (void *)MX8M_ATF_BL33_BASE_ADDR;
	unsigned long endmem = MX8M_DDR_CSD1_BASE_ADDR + imx8m_barebox_earlymem_size(32);

	imx8m_save_bootrom_log((void *)arm_mem_scratch(endmem));
	imx8mp_load_bl33(bl33);

	if (IS_ENABLED(CONFIG_FIRMWARE_IMX8MP_OPTEE))
		imx8m_load_and_start_optee_via_tfa(imx8mp, (void *)arm_mem_optee(endmem), bl33);
	else
		imx8mp_load_and_start_tfa(imx8mp_bl31_bin);
}


void imx8mn_load_bl33(void *bl33)
{
	enum bootsource src;
	int instance;

	imx8mn_get_boot_source(&src, &instance);
	switch (src) {
	case BOOTSOURCE_MMC:
		imx8mn_esdhc_load_image(instance, false);
		break;
	case BOOTSOURCE_SERIAL:
		imx8mn_bootrom_load_image();
		break;
	case BOOTSOURCE_SPI:
		imx8mn_qspi_load_image(instance, false);
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
}

__noreturn void imx8mn_load_and_start_image_via_tfa(void)
{
	void *bl33 = (void *)MX8M_ATF_BL33_BASE_ADDR;
	unsigned long endmem = MX8M_DDR_CSD1_BASE_ADDR + imx8m_barebox_earlymem_size(16);

	imx8m_save_bootrom_log((void *)arm_mem_scratch(endmem));
	imx8mn_load_bl33(bl33);

	if (IS_ENABLED(CONFIG_FIRMWARE_IMX8MN_OPTEE))
		imx8m_load_and_start_optee_via_tfa(imx8mn, (void *)arm_mem_optee(endmem), bl33);
	else
		imx8mn_load_and_start_tfa(imx8mn_bl31_bin);
}
