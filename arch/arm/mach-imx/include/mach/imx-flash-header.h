#ifndef __MACH_FLASH_HEADER_H
#define __MACH_FLASH_HEADER_H

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

#endif /* __MACH_FLASH_HEADER_H */
