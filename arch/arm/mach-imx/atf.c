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
#include <mach/imx/imx9-regs.h>
#include <soc/fsl/fsl_udc.h>
#include <soc/fsl/caam.h>
#include <tee/optee.h>
#include <mach/imx/ele.h>

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

void imx8mm_load_bl33(void *bl33)
{
	enum bootsource src;
	int instance;

	imx8mm_get_boot_source(&src, &instance);
	switch (src) {
	case BOOTSOURCE_MMC:
		imx8m_esdhc_load_image(instance, bl33);
		break;
	case BOOTSOURCE_SERIAL:
		if (!IS_ENABLED(CONFIG_USB_GADGET_DRIVER_ARC_PBL)) {
			printf("Serial bootmode not supported\n");
			break;
		}

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
		imx8mm_qspi_load_image(instance, bl33);
		break;
	default:
		printf("Unsupported bootsource BOOTSOURCE_%d\n", src);
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

static void imx_adjust_optee_memory(void **bl32, void **bl32_image, size_t *bl32_size)
{
	struct optee_header *hdr = *bl32_image;
	u64 membase;

	if (optee_verify_header(hdr))
		return;

	imx_scratch_save_optee_hdr(hdr);

	membase = (u64)hdr->init_load_addr_hi << 32;
	membase |= hdr->init_load_addr_lo;

	*bl32 = (void *)membase;
	*bl32_size -= sizeof(*hdr);
	*bl32_image += sizeof(*hdr);
}

__noreturn void imx8mm_load_and_start_image_via_tfa(void)
{
	__imx8mm_load_and_start_image_via_tfa((void *)MX8M_ATF_BL33_BASE_ADDR);
}

__noreturn void __imx8mm_load_and_start_image_via_tfa(void *bl33)
{
	const void *bl31;
	size_t bl31_size;
	unsigned long endmem = MX8M_DDR_CSD1_BASE_ADDR + imx8m_barebox_earlymem_size(32);

	imx8mm_init_scratch_space();
	imx8m_save_bootrom_log();
	imx8mm_load_bl33(bl33);

	if (IS_ENABLED(CONFIG_FIRMWARE_IMX8MM_OPTEE)) {
		void *bl32 = (void *)arm_mem_optee(endmem);
		size_t bl32_size;
		void *bl32_image;

		imx8mm_tzc380_init();
		get_builtin_firmware_ext(imx8mm_bl32_bin,
				bl33, &bl32_image,
				&bl32_size);

		imx_adjust_optee_memory(&bl32, &bl32_image, &bl32_size);

		memcpy(bl32, bl32_image, bl32_size);

		get_builtin_firmware(imx8mm_bl31_bin_optee, &bl31, &bl31_size);
	} else {
		get_builtin_firmware(imx8mm_bl31_bin, &bl31, &bl31_size);
	}

	imx8m_atf_start_bl31(bl31, bl31_size, (void *)MX8MM_ATF_BL31_BASE_ADDR);
}

void imx8mp_load_bl33(void *bl33)
{
	enum bootsource src;
	int instance;

	imx8mp_get_boot_source(&src, &instance);
	switch (src) {
	case BOOTSOURCE_MMC:
		imx8mp_esdhc_load_image(instance, bl33);
		break;
	case BOOTSOURCE_SERIAL:
		imx8mp_romapi_load_image(bl33);
		break;
	case BOOTSOURCE_SPI:
		imx8mp_qspi_load_image(instance, bl33);
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
	__imx8mp_load_and_start_image_via_tfa((void *)MX8M_ATF_BL33_BASE_ADDR);
}

__noreturn void __imx8mp_load_and_start_image_via_tfa(void *bl33)
{
	const void *bl31;
	size_t bl31_size;
	unsigned long endmem = MX8M_DDR_CSD1_BASE_ADDR + imx8m_barebox_earlymem_size(32);

	imx8mp_init_scratch_space();
	imx8m_save_bootrom_log();
	imx8mp_load_bl33(bl33);

	if (IS_ENABLED(CONFIG_FIRMWARE_IMX8MP_OPTEE)) {
		void *bl32 = (void *)arm_mem_optee(endmem);
		size_t bl32_size;
		void *bl32_image;

		imx8mp_tzc380_init();
		get_builtin_firmware_ext(imx8mp_bl32_bin,
				bl33, &bl32_image,
				&bl32_size);

		imx_adjust_optee_memory(&bl32, &bl32_image, &bl32_size);

		memcpy(bl32, bl32_image, bl32_size);

		get_builtin_firmware(imx8mp_bl31_bin_optee, &bl31, &bl31_size);
	} else {
		get_builtin_firmware(imx8mp_bl31_bin, &bl31, &bl31_size);
	}

	imx8m_atf_start_bl31(bl31, bl31_size, (void *)MX8MP_ATF_BL31_BASE_ADDR);
}


void imx8mn_load_bl33(void *bl33)
{
	enum bootsource src;
	int instance;

	imx8mn_get_boot_source(&src, &instance);
	switch (src) {
	case BOOTSOURCE_MMC:
		imx8mn_esdhc_load_image(instance, bl33);
		break;
	case BOOTSOURCE_SERIAL:
		imx8mn_romapi_load_image(bl33);
		break;
	case BOOTSOURCE_SPI:
		imx8mn_qspi_load_image(instance, bl33);
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
	__imx8mn_load_and_start_image_via_tfa((void *)MX8M_ATF_BL33_BASE_ADDR);
}

__noreturn void __imx8mn_load_and_start_image_via_tfa(void *bl33)
{
	const void *bl31;
	size_t bl31_size;
	unsigned long endmem = MX8M_DDR_CSD1_BASE_ADDR + imx8m_barebox_earlymem_size(16);

	imx8mn_init_scratch_space();
	imx8m_save_bootrom_log();
	imx8mn_load_bl33(bl33);

	if (IS_ENABLED(CONFIG_FIRMWARE_IMX8MN_OPTEE)) {
		void *bl32 = (void *)arm_mem_optee(endmem);
		size_t bl32_size;
		void *bl32_image;

		imx8mn_tzc380_init();
		get_builtin_firmware_ext(imx8mn_bl32_bin,
				bl33, &bl32_image,
				&bl32_size);

		imx_adjust_optee_memory(&bl32, &bl32_image, &bl32_size);

		memcpy(bl32, bl32_image, bl32_size);

		get_builtin_firmware(imx8mn_bl31_bin_optee, &bl31, &bl31_size);
	} else {
		get_builtin_firmware(imx8mn_bl31_bin, &bl31, &bl31_size);
	}

	imx8m_atf_start_bl31(bl31, bl31_size, (void *)MX8MN_ATF_BL31_BASE_ADDR);
}

void imx8mq_load_bl33(void *bl33)
{
	enum bootsource src;
	int instance;

	imx8mn_get_boot_source(&src, &instance);
	switch (src) {
	case BOOTSOURCE_MMC:
		imx8m_esdhc_load_image(instance, bl33);
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

__noreturn void imx8mq_load_and_start_image_via_tfa(void)
{
	__imx8mq_load_and_start_image_via_tfa((void *)MX8M_ATF_BL33_BASE_ADDR);
}

__noreturn void __imx8mq_load_and_start_image_via_tfa(void *bl33)
{
	const void *bl31;
	size_t bl31_size;
	unsigned long endmem = MX8M_DDR_CSD1_BASE_ADDR + imx8m_barebox_earlymem_size(32);

	imx8mq_init_scratch_space();
	imx8m_save_bootrom_log();
	imx8mq_load_bl33(bl33);

	if (IS_ENABLED(CONFIG_FIRMWARE_IMX8MQ_OPTEE)) {
		void *bl32 = (void *)arm_mem_optee(endmem);
		size_t bl32_size;
		void *bl32_image;

		imx8mq_tzc380_init();
		get_builtin_firmware_ext(imx8mq_bl32_bin,
				bl33, &bl32_image,
				&bl32_size);

		imx_adjust_optee_memory(&bl32, &bl32_image, &bl32_size);

		memcpy(bl32, bl32_image, bl32_size);

		get_builtin_firmware(imx8mq_bl31_bin_optee, &bl31, &bl31_size);
	} else {
		get_builtin_firmware(imx8mq_bl31_bin, &bl31, &bl31_size);
	}

	imx8m_atf_start_bl31(bl31, bl31_size, (void *)MX8MQ_ATF_BL31_BASE_ADDR);
}

void __noreturn imx93_load_and_start_image_via_tfa(void)
{
	unsigned long atf_dest = MX93_ATF_BL31_BASE_ADDR;
	void __noreturn (*bl31)(void) = (void *)atf_dest;
	const void *tfa;
	size_t tfa_size;
	void *bl33 = (void *)MX93_ATF_BL33_BASE_ADDR;
	unsigned long endmem = MX9_DDR_CSD1_BASE_ADDR + imx9_ddrc_sdram_size();

	imx93_init_scratch_space(true);

	/*
	 * On completion the TF-A will jump to MX93_ATF_BL33_BASE_ADDR
	 * in EL2. Copy the image there, but replace the PBL part of
	 * that image with ourselves. On a high assurance boot only the
	 * currently running code is validated and contains the checksum
	 * for the piggy data, so we need to ensure that we are running
	 * the same code in DRAM.
	 *
	 * The second purpose of this memcpy is for USB booting. When booting
	 * from USB the image comes in as a stream, so the PBL is transferred
	 * only once. As we jump into the PBL again in SDRAM we need to copy
	 * it there. The USB protocol transfers data in chunks of 1024 bytes,
	 * so align the copy size up to the next 1KiB boundary.
	 */
	memcpy((void *)MX93_ATF_BL33_BASE_ADDR, __image_start, ALIGN(barebox_pbl_size, 1024));

	if (IS_ENABLED(CONFIG_FIRMWARE_IMX93_OPTEE)) {
		void *bl32 = (void *)arm_mem_optee(endmem);
		size_t bl32_size;
		void *bl32_image;

		imx93_ele_load_fw(bl33);

		get_builtin_firmware_ext(imx93_bl32_bin,
				bl33, &bl32_image,
				&bl32_size);

		imx_adjust_optee_memory(&bl32, &bl32_image, &bl32_size);

		memcpy(bl32, bl32_image, bl32_size);

		get_builtin_firmware(imx93_bl31_bin_optee, &tfa, &tfa_size);
	} else {
		get_builtin_firmware(imx93_bl31_bin, &tfa, &tfa_size);
	}

	memcpy(bl31, tfa, tfa_size);

	asm volatile("msr sp_el2, %0" : :
		     "r" (MX93_ATF_BL33_BASE_ADDR - 16) :
		     "cc");
	bl31();
	__builtin_unreachable();
}
