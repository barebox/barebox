#include <common.h>
#include <mach/imx-flash-header.h>

void __naked __flash_header_start go(void)
{
	__asm__ __volatile__("b exception_vectors\n");
}

struct imx_dcd_entry __dcd_entry_0x400 dcd_entry[] = {
	{ .ptr_type = 4, .addr = 0x53F80004, .val = 0x00821000, },
	{ .ptr_type = 4, .addr = 0x53F80004, .val = 0x00821000, },
	{ .ptr_type = 4, .addr = 0xB8001010, .val = 0x00000004, },
	{ .ptr_type = 4, .addr = 0xB8001010, .val = 0x0000000C, },
	{ .ptr_type = 4, .addr = 0xB8001004, .val = 0x0009572B, },
	{ .ptr_type = 4, .addr = 0xB8001000, .val = 0x92220000, },
	{ .ptr_type = 1, .addr = 0x80000400, .val = 0xda, },
	{ .ptr_type = 4, .addr = 0xB8001000, .val = 0xA2220000, },
	{ .ptr_type = 1, .addr = 0x80000000, .val = 0x87654321, },
	{ .ptr_type = 1, .addr = 0x80000000, .val = 0x87654321, },
	{ .ptr_type = 4, .addr = 0xB8001000, .val = 0xB2220000, },
	{ .ptr_type = 1, .addr = 0x80000033, .val = 0xda, },
	{ .ptr_type = 1, .addr = 0x82000000, .val = 0xda, },
	{ .ptr_type = 4, .addr = 0xB8001000, .val = 0x82224080, },
	{ .ptr_type = 4, .addr = 0xB8001010, .val = 0x00000004, },
};

#define APP_DEST	0x80000000

struct imx_flash_header __flash_header_0x400 eukrea_cpuimx35_header = {
	.app_code_jump_vector	= APP_DEST + 0x1000,
	.app_code_barker	= APP_CODE_BARKER,
	.app_code_csf		= 0,
	.dcd_ptr_ptr		= APP_DEST + 0x400 + offsetof(struct imx_flash_header, dcd),
	.super_root_key		= 0,
	.dcd			= APP_DEST + 0x400 + offsetof(struct imx_flash_header, dcd_barker),
	.app_dest		= APP_DEST,
	.dcd_barker		= DCD_BARKER,
	.dcd_block_len		= sizeof (dcd_entry),
};

unsigned long __image_len_0x400 barebox_len = 0x40000;
