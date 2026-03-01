// SPDX-License-Identifier: GPL-2.0-only

#include <asm/atf_common.h>
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
#include <mach/imx/xload.h>
#include <mach/imx/snvs.h>
#include <pbl.h>

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

static __noreturn void bl31_via_bl_params(void *bl31, void *bl32, void *bl33,
					  void *fdt)
{
	struct bl2_to_bl31_params_mem_v2 *params;

	/* Prepare bl_params for BL32 */
	params = bl2_plat_get_bl31_params_v2((uintptr_t)bl32,
			(uintptr_t)bl33, (uintptr_t)fdt);

	pr_debug("Jump to BL31 with bl-params (%s BL32-FDT)\n",
		 fdt ? "including" : "excluding");
	/*
	 * Start BL31 without passing the FDT via x1 since the mainline
	 * TF-A doesn't support it yet.
	 */
	bl31_entry_v2((uintptr_t)bl31, &params->bl_params, NULL);

	__builtin_unreachable();
}

static __noreturn void start_bl31_via_bl_params(void *bl31, void *bl32,
						void *bl33, void *fdt)
{
	unsigned long mem_base = MX8M_DDR_CSD1_BASE_ADDR;
	unsigned long mem_sz;
	unsigned int bufsz = 0;
	int error;
	u8 *buf;

	if (!fdt)
		bl31_via_bl_params(bl31, bl32, bl33, NULL);

	buf = imx_scratch_get_fdt(&bufsz);
	if (IS_ERR_OR_NULL(buf)) {
		if (!buf)
			pr_debug("No FDT scratch mem configured, continue without FDT\n");
		else
			pr_warn("Failed to get FDT scratch mem, continue without FDT\n");
		bl31_via_bl_params(bl31, bl32, bl33, NULL);
	}

	error = pbl_load_fdt(fdt, buf, bufsz);
	if (error) {
		pr_warn("Failed to load FDT, continue without FDT\n");
		bl31_via_bl_params(bl31, bl32, bl33, NULL);
	}

	if (cpu_is_mx8mn())
		mem_sz = imx8m_ddrc_sdram_size(16);
	else
		mem_sz = imx8m_ddrc_sdram_size(32);

	fdt = buf;
	error = fdt_fixup_mem(fdt, &mem_base, &mem_sz, 1);
	if (error) {
		pr_warn("Failed to fixup FDT memory node, continue without FDT\n");
		bl31_via_bl_params(bl31, bl32, bl33, NULL);
	}

	bl31_via_bl_params(bl31, bl32, bl33, fdt);
}

/**
 * imx8m_tfa_start_bl31 - Load TF-A BL31 blob and transfer control to it
 *
 * @tfa:	Pointer to the BL31 blob
 * @tfa_size:	Size of the BL31 blob
 * @tfa_dest:	Place where the BL31 is copied to and executed
 * @tee:	Pointer to the optional BL32 blob
 * @tee_size:	Size of the optional BL32 blob
 * @bl33:	Pointer to the already loaded BL33 blob
 * @fdt:	Pointer to the barebox internal DT (either compressed or not
 *		compressed)
 *
 * This function:
 *
 *     1. Copies built-in BL32 blob (if any) to the well-known OP-TEE address
 *        (end of RAM) or the address specified via the OP-TEE v2 header if
 *        found.
 *
 *     2. Copies built-in BL31 blob to an address i.MX8M's BL31
 *        expects to be placed (TF-A v2.8+ is position-independent)
 *
 *     3. Sets up temporary stack pointer for EL2, which is execution
 *        level that BL31 will drop us off at after it completes its
 *        initialization routine
 *
 *     4. Transfers control to BL31
 */

static __noreturn void
imx8m_tfa_start_bl31(const void *tfa_bin, size_t tfa_size, void *tfa_dest,
		     void *tee_bin, size_t tee_size, void *bl33, void *fdt)
{
	void __noreturn (*bl31)(void) = tfa_dest;
	unsigned long endmem;
	void *bl32 = NULL;
	int ret;

	endmem = MX8M_DDR_CSD1_BASE_ADDR;
	if (cpu_is_mx8mn())
		endmem += imx8m_barebox_earlymem_size(16);
	else
		endmem += imx8m_barebox_earlymem_size(32);

	/* TEE (BL32) handling */
	if (tee_bin && tee_size) {
		imx8m_tzc380_init();

		bl32 = (void *)arm_mem_optee(endmem);
		imx_adjust_optee_memory(&bl32, &tee_bin, &tee_size);

		memcpy(bl32, tee_bin, tee_size);
	}

	/* TF-A (BL31) handling */
	BUG_ON(tfa_size > MX8M_ATF_BL31_SIZE_LIMIT);

	if (IS_ENABLED(CONFIG_FSL_CAAM_RNG_PBL_INIT)) {
		ret = imx_early_caam_init(MX8M_CAAM_BASE_ADDR);
		if (ret)
			pr_debug("CAAM early init failed: %d\n", ret);
		else
			pr_debug("CAAM early init successful\n");
	}

	pr_debug("Loading BL31 to %p ", tfa_bin);

	memcpy(bl31, tfa_bin, tfa_size);

	/*
	 * If all works fine (and TF-A has debug output enabled), the next
	 * serial output should start with:
	 *
	 *   NOTICE:  BL31:
	 *
	 * If not, set IMX_BOOT_UART_BASE=auto when building TF-A if supported,
	 * check if DRAM configuration is correct and that the PMIC correctly
	 * configured the DRAM rails. For very old TF-A versions, also compare
	 * barebox tfa_dest against whatever value your TF-A hardcodes.
	 */
	pr_debug("and jumping with SP: %p\n", tfa_dest - 16);

	/* Stack setup and BL31 jump */
	asm volatile("msr sp_el2, %0" : :
		     "r" (tfa_dest - 16) :
		     "cc");

	/*
	 * If enabled the bl_params are passed via x0 to the TF-A, except for
	 * the i.MX8MQ which doesn't support bl_params yet.
	 * Passing the bl_params must be explicit enabled to be backward
	 * compatible with downstream TF-A versions, which may have problems
	 * with the bl_params.
	 */
	if (!IS_ENABLED(CONFIG_ARCH_IMX_ATF_PASS_BL_PARAMS) || cpu_is_mx8mq()) {
		pr_debug("Jump to BL31 without bl-params\n");
		bl31();
	} else {
		start_bl31_via_bl_params(bl31, bl32, bl33, fdt);
	}

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
	case BOOTSOURCE_SPI_NOR:
		imx8m_ecspi_load_image(instance, bl33);
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

__noreturn void imx8mm_load_and_start_image_via_tfa(void)
{
	__imx8mm_load_and_start_image_via_tfa(NULL, (void *)MX8M_ATF_BL33_BASE_ADDR);
}

__noreturn void __imx8mm_load_and_start_image_via_tfa(void *fdt, void *bl33)
{
	const void *bl31;
	size_t bl31_size;
	void *bl32 = NULL;
	size_t bl32_size = 0;

	imx_set_cpu_type(IMX_CPU_IMX8MM);
	imx8mm_init_scratch_space();
	imx8m_save_bootrom_log();
	imx8m_setup_snvs();
	imx8mm_load_bl33(bl33);

	if (IS_ENABLED(CONFIG_FIRMWARE_IMX8MM_OPTEE)) {
		get_builtin_firmware_ext(imx8mm_bl32_bin, bl33, &bl32, &bl32_size);
		get_builtin_firmware(imx8mm_bl31_bin_optee, &bl31, &bl31_size);
	} else {
		get_builtin_firmware(imx8mm_bl31_bin, &bl31, &bl31_size);
	}

	imx8m_tfa_start_bl31(bl31, bl31_size, (void *)MX8MM_ATF_BL31_BASE_ADDR,
			     bl32, bl32_size, bl33, fdt);
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
	case BOOTSOURCE_SPI_NOR:
		imx8m_ecspi_load_image(instance, bl33);
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
	__imx8mp_load_and_start_image_via_tfa(NULL, (void *)MX8M_ATF_BL33_BASE_ADDR);
}

__noreturn void __imx8mp_load_and_start_image_via_tfa(void *fdt, void *bl33)
{
	const void *bl31;
	size_t bl31_size;
	void *bl32 = NULL;
	size_t bl32_size = 0;

	imx_set_cpu_type(IMX_CPU_IMX8MP);
	imx8mp_init_scratch_space();
	imx8m_save_bootrom_log();
	imx8m_setup_snvs();
	imx8mp_load_bl33(bl33);

	if (IS_ENABLED(CONFIG_FIRMWARE_IMX8MP_OPTEE)) {
		get_builtin_firmware_ext(imx8mp_bl32_bin, bl33, &bl32, &bl32_size);
		get_builtin_firmware(imx8mp_bl31_bin_optee, &bl31, &bl31_size);
	} else {
		get_builtin_firmware(imx8mp_bl31_bin, &bl31, &bl31_size);
	}

	imx8m_tfa_start_bl31(bl31, bl31_size, (void *)MX8MP_ATF_BL31_BASE_ADDR,
			     bl32, bl32_size, bl33, fdt);
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
	case BOOTSOURCE_SPI_NOR:
		imx8m_ecspi_load_image(instance, bl33);
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
	__imx8mn_load_and_start_image_via_tfa(NULL, (void *)MX8M_ATF_BL33_BASE_ADDR);
}

__noreturn void __imx8mn_load_and_start_image_via_tfa(void *fdt, void *bl33)
{
	const void *bl31;
	size_t bl31_size;
	void *bl32 = NULL;
	size_t bl32_size = 0;

	imx_set_cpu_type(IMX_CPU_IMX8MN);
	imx8mn_init_scratch_space();
	imx8m_save_bootrom_log();
	imx8m_setup_snvs();
	imx8mn_load_bl33(bl33);

	if (IS_ENABLED(CONFIG_FIRMWARE_IMX8MN_OPTEE)) {
		get_builtin_firmware_ext(imx8mn_bl32_bin, bl33, &bl32, &bl32_size);
		get_builtin_firmware(imx8mn_bl31_bin_optee, &bl31, &bl31_size);
	} else {
		get_builtin_firmware(imx8mn_bl31_bin, &bl31, &bl31_size);
	}

	imx8m_tfa_start_bl31(bl31, bl31_size, (void *)MX8MN_ATF_BL31_BASE_ADDR,
			     bl32, bl32_size, bl33, fdt);
}

void imx8mq_load_bl33(void *bl33)
{
	enum bootsource src;
	int instance;

	imx8mq_get_boot_source(&src, &instance);
	switch (src) {
	case BOOTSOURCE_MMC:
		imx8m_esdhc_load_image(instance, bl33);
		break;
	case BOOTSOURCE_SPI_NOR:
		imx8m_ecspi_load_image(instance, bl33);
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
	__imx8mq_load_and_start_image_via_tfa(NULL, (void *)MX8M_ATF_BL33_BASE_ADDR);
}

__noreturn void __imx8mq_load_and_start_image_via_tfa(void *fdt, void *bl33)
{
	const void *bl31;
	size_t bl31_size;
	void *bl32 = NULL;
	size_t bl32_size = 0;

	imx_set_cpu_type(IMX_CPU_IMX8MQ);
	imx8mq_init_scratch_space();
	imx8m_save_bootrom_log();
	imx8m_setup_snvs();
	imx8mq_load_bl33(bl33);

	if (IS_ENABLED(CONFIG_FIRMWARE_IMX8MQ_OPTEE)) {
		get_builtin_firmware_ext(imx8mq_bl32_bin, bl33, &bl32, &bl32_size);
		get_builtin_firmware(imx8mq_bl31_bin_optee, &bl31, &bl31_size);
	} else {
		get_builtin_firmware(imx8mq_bl31_bin, &bl31, &bl31_size);
	}

	imx8m_tfa_start_bl31(bl31, bl31_size, (void *)MX8MQ_ATF_BL31_BASE_ADDR,
			     bl32, bl32_size, bl33, fdt);
}

void __noreturn imx93_load_and_start_image_via_tfa(void)
{
	__imx93_load_and_start_image_via_tfa((void *)MX93_ATF_BL33_BASE_ADDR);
}

void __noreturn __imx93_load_and_start_image_via_tfa(void *bl33)
{
	unsigned long atf_dest = MX93_ATF_BL31_BASE_ADDR;
	void __noreturn (*bl31)(void) = (void *)atf_dest;
	const void *tfa;
	size_t tfa_size;
	unsigned long endmem = MX9_DDR_CSD1_BASE_ADDR + imx9_ddrc_sdram_size();

	imx_set_cpu_type(IMX_CPU_IMX93);
	imx93_init_scratch_space(true);
	imx93_romapi_load_image(bl33);

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

	handoff_data_move(bl33 - ALIGN(handoff_data_size(), 0x1000));

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
	memcpy(bl33, __image_start, ALIGN(barebox_pbl_size, 1024));

	memcpy(bl31, tfa, tfa_size);

	asm volatile("msr sp_el2, %0" : :
		     "r" (bl33 - 16) :
		     "cc");
	bl31();
	__builtin_unreachable();
}
