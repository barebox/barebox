#include <common.h>
#include <mach/imx-flash-header.h>
#include <mach/imx35-regs.h>
#include <asm/barebox-arm-head.h>

void __naked __flash_header_start go(void)
{
	barebox_arm_head();
}

struct imx_dcd_entry __dcd_entry_section dcd_entry[] = {
	{ .ptr_type = 4, .addr = 0x53F80004, .val = 0x00821000, },
	{ .ptr_type = 4, .addr = 0x53F80004, .val = 0x00821000, },
	{ .ptr_type = 4, .addr = 0xb8001010, .val = 0x00000004, },
	{ .ptr_type = 4, .addr = 0xB8001010, .val = 0x0000000C, },
	{ .ptr_type = 4, .addr = 0xb8001004, .val = 0x0009572B, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0x92220000, },
	{ .ptr_type = 1, .addr = 0x80000400, .val = 0xda, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0xa2220000, },
	{ .ptr_type = 4, .addr = 0x80000000, .val = 0x12344321, },
	{ .ptr_type = 4, .addr = 0x80000000, .val = 0x12344321, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0xb2220000, },
	{ .ptr_type = 1, .addr = 0x80000033, .val = 0xda, },
	{ .ptr_type = 1, .addr = 0x82000000, .val = 0xda, },
	{ .ptr_type = 4, .addr = 0xb8001000, .val = 0x82224080, },
	{ .ptr_type = 4, .addr = 0xb8001010, .val = 0x00000004, },
};

#define CPUIMX35_DEST_BASE 0x80000000
#define CPUIMX35_FLASH_HEADER_BASE (CPUIMX35_DEST_BASE + FLASH_HEADER_OFFSET)
struct imx_flash_header __flash_header_section flash_header = {
	.app_code_jump_vector	= CPUIMX35_DEST_BASE + 0x1000,
	.app_code_barker	= APP_CODE_BARKER,
	.app_code_csf		= 0,
	.dcd_ptr_ptr		= CPUIMX35_FLASH_HEADER_BASE + offsetof(struct imx_flash_header, dcd),
	.super_root_key		= 0,
	.dcd			= CPUIMX35_FLASH_HEADER_BASE + offsetof(struct imx_flash_header, dcd_barker),
	.app_dest		= CPUIMX35_DEST_BASE,
	.dcd_barker		= DCD_BARKER,
	.dcd_block_len		= sizeof(dcd_entry),
};

unsigned long __image_len_section barebox_len = DCD_BAREBOX_SIZE;
