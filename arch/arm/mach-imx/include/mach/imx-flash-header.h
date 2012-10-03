#ifndef __MACH_FLASH_HEADER_H
#define __MACH_FLASH_HEADER_H

#include <asm/sections.h>

#define __flash_header_start		__section(.flash_header_start)

#if defined(CONFIG_ARCH_IMX_INTERNAL_BOOT_NOR)
	#define __flash_header_section		__section(.flash_header_0x1000)
	#define __dcd_entry_section		__section(.dcd_entry_0x1000)
	#define __image_len_section		__section(.image_len_0x1000)
	#define FLASH_HEADER_OFFSET 0x1000
#elif defined(CONFIG_ARCH_IMX_INTERNAL_BOOT_ONENAND)
	#define __flash_header_section		__section(.flash_header_0x0100)
	#define __dcd_entry_section		__section(.dcd_entry_0x0100)
	#define __image_len_section		__section(.image_len_0x0100)
	#define FLASH_HEADER_OFFSET 0x0100
#elif defined(CONFIG_ARCH_IMX_INTERNAL_BOOT_SERIAL)
	#define __flash_header_section		__section(.flash_header_0x0)
	#define __dcd_entry_section		__section(.dcd_entry_0x0)
	#define __image_len_section		__section(.image_len_0x0)
	#define FLASH_HEADER_OFFSET 0x0
#else
	#define __flash_header_section		__section(.flash_header_0x0400)
	#define __dcd_entry_section		__section(.dcd_entry_0x0400)
	#define __image_len_section		__section(.image_len_0x0400)
	#define FLASH_HEADER_OFFSET 0x0400
#endif

#define __flash_header_0x1000	__section(.flash_header_0x1000)
#define __dcd_entry_0x1000	__section(.dcd_entry_0x1000)
#define __image_len_0x1000	__section(.image_len_0x1000)

#define __flash_header_0x0100	__section(.flash_header_0x0100)
#define __dcd_entry_0x0100	__section(.dcd_entry_0x0100)
#define __image_len_0x0100	__section(.image_len_0x0100)

#define __flash_header_0x0400	__section(.flash_header_0x0400)
#define __dcd_entry_0x0400	__section(.dcd_entry_0x0400)
#define __image_len_0x0400	__section(.image_len_0x0400)

#define __flash_header_0x0	__section(.flash_header_0x0)
#define __dcd_entry_0x0		__section(.dcd_entry_0x0)
#define __image_len_0x0		__section(.image_len_0x0)

/*
 * NOR is not automatically copied anywhere by the boot ROM
 */
#if defined (CONFIG_ARCH_IMX_INTERNAL_BOOT_NOR)
	#define DEST_BASE	IMX_CS0_BASE
#else
	#define DEST_BASE	TEXT_BASE
#endif

#define FLASH_HEADER_BASE	(DEST_BASE + FLASH_HEADER_OFFSET)

struct imx_dcd_entry {
	unsigned long ptr_type;
	unsigned long addr;
	unsigned long val;
};

struct imx_dcd_v2_entry {
	__be32 addr;
	__be32 val;
};

#define DCD_BARKER	0xb17219e9

struct imx_rsa_public_key {
	unsigned char	rsa_exponent[4];
	unsigned char	*rsa_modululs;
	unsigned short	*exponent_size;
	unsigned short	modulus_size;
	unsigned char	init_flag;
};

#define APP_CODE_BARKER	0x000000b1

struct imx_flash_header {
	unsigned long			app_code_jump_vector;
	unsigned long			app_code_barker;
	unsigned long			app_code_csf;
	unsigned long			dcd_ptr_ptr;
	unsigned long			super_root_key;
	unsigned long			dcd;
	unsigned long			app_dest;
	unsigned long			dcd_barker;
	unsigned long			dcd_block_len;
};

#define IVT_HEADER_TAG		0xd1
#define IVT_VERSION		0x40

#define DCD_HEADER_TAG		0xd2
#define DCD_VERSION		0x40

#define DCD_COMMAND_WRITE_TAG	0xcc
#define DCD_COMMAND_WRITE_PARAM	0x04

/*
 * At least on i.MX5 the ROM copies only full blocks. Unfortunately
 * it does not round up to the next full block, we have to do it
 * ourselves. Use 4095 which should be enough for the largest NAND
 * pages.
 */
#define DCD_BAREBOX_SIZE	(barebox_image_size + 4095)

struct imx_ivt_header {
	uint8_t tag;
	__be16 length;
	uint8_t version;
} __attribute__((packed));

struct imx_dcd_command {
	uint8_t tag;
	__be16 length;
	uint8_t param;
} __attribute__((packed));

struct imx_dcd {
	struct imx_ivt_header header;
#ifndef IMX_INTERNAL_NAND_BBU
	struct imx_dcd_command command;
#endif
};

struct imx_boot_data {
	uint32_t start;
	uint32_t size;
	uint32_t plugin;
};

struct imx_flash_header_v2 {
	struct imx_ivt_header header;

	uint32_t entry;
	uint32_t reserved1;
	uint32_t dcd_ptr;
	uint32_t boot_data_ptr;
	uint32_t self;
	uint32_t csf;
	uint32_t reserved2;

	struct imx_boot_data boot_data;
	struct imx_dcd dcd;
};

/*
 * A variant of the standard barebox header in the i.MX FCB
 * format. Needed for i.MX53 NAND boot
 */
static inline void barebox_arm_imx_fcb_head(void)
{
	__asm__ __volatile__ (
		".arm\n"
		"	b 1f\n"
		".word 0x20424346\n" /* FCB */
		".word 0x1\n"
#ifdef CONFIG_THUMB2_BAREBOX
		"1:	adr r9, 1f + 1\n"
		"	bx r9\n"
		".thumb\n"
		"1:\n"
		"bl	barebox_arm_reset_vector\n"
#else
		"1:	b barebox_arm_reset_vector\n"
		".word 0x0\n"
		".word 0x0\n"
#endif
		".word 0x0\n"
		".word 0x0\n"

		".asciz \"barebox\"\n"
		".word _text\n"				/* text base. If copied there,
							 * barebox can skip relocation
							 */
		".word _barebox_image_size\n"		/* image size to copy */
	);
}

#endif /* __MACH_FLASH_HEADER_H */
