#include <common.h>
#include <mach/imx-flash-header.h>
#include <asm/barebox-arm-head.h>

void __naked __flash_header_start go(void)
{
	barebox_arm_head();
}

struct imx_dcd_entry __dcd_entry_section dcd_entry[] = {
	{ .ptr_type = 4, .addr = 0x73fa88a0, .val = 0x00000200, },
	{ .ptr_type = 4, .addr = 0x73fa850c, .val = 0x000020c5, },
	{ .ptr_type = 4, .addr = 0x73fa8510, .val = 0x000020c5, },
	{ .ptr_type = 4, .addr = 0x73fa883c, .val = 0x00000002, },
	{ .ptr_type = 4, .addr = 0x73fa8848, .val = 0x00000002, },
	{ .ptr_type = 4, .addr = 0x73fa84b8, .val = 0x000000e7, },
	{ .ptr_type = 4, .addr = 0x73fa84bc, .val = 0x00000045, },
	{ .ptr_type = 4, .addr = 0x73fa84c0, .val = 0x00000045, },
	{ .ptr_type = 4, .addr = 0x73fa84c4, .val = 0x00000045, },
	{ .ptr_type = 4, .addr = 0x73fa84c8, .val = 0x00000045, },
	{ .ptr_type = 4, .addr = 0x73fa8820, .val = 0x00000000, },
	{ .ptr_type = 4, .addr = 0x73fa84a4, .val = 0x00000003, },
	{ .ptr_type = 4, .addr = 0x73fa84a8, .val = 0x00000003, },
	{ .ptr_type = 4, .addr = 0x73fa84ac, .val = 0x000000e3, },
	{ .ptr_type = 4, .addr = 0x73fa84b0, .val = 0x000000e3, },
	{ .ptr_type = 4, .addr = 0x73fa84b4, .val = 0x000000e3, },
	{ .ptr_type = 4, .addr = 0x73fa84cc, .val = 0x000000e3, },
	{ .ptr_type = 4, .addr = 0x73fa84d0, .val = 0x000000e2, },
	{ .ptr_type = 4, .addr = 0x73fa882c, .val = 0x00000004, },
	{ .ptr_type = 4, .addr = 0x73fa88a4, .val = 0x00000004, },
	{ .ptr_type = 4, .addr = 0x73fa88ac, .val = 0x00000004, },
	{ .ptr_type = 4, .addr = 0x73fa88b8, .val = 0x00000004, },
	{ .ptr_type = 4, .addr = 0x83fd9000, .val = 0x82a20000, },
	{ .ptr_type = 4, .addr = 0x83fd9008, .val = 0x82a20000, },
	{ .ptr_type = 4, .addr = 0x83fd9010, .val = 0x000ad0d0, },
	{ .ptr_type = 4, .addr = 0x83fd9004, .val = 0x3f3584ab, },
	{ .ptr_type = 4, .addr = 0x83fd900c, .val = 0x3f3584ab, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x04008008, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x0000801a, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x0000801b, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x00448019, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x07328018, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x04008008, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x00008010, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x00008010, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x06328018, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x03808019, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x00408019, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x00008000, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x0400800c, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x0000801e, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x0000801f, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x0000801d, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x0732801c, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x0400800c, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x00008014, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x00008014, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x0632801c, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x0380801d, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x0040801d, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x00008004, },
	{ .ptr_type = 4, .addr = 0x83fd9000, .val = 0xb2a20000, },
	{ .ptr_type = 4, .addr = 0x83fd9008, .val = 0xb2a20000, },
	{ .ptr_type = 4, .addr = 0x83fd9010, .val = 0x000ad6d0, },
	{ .ptr_type = 4, .addr = 0x83fd9034, .val = 0x90000000, },
	{ .ptr_type = 4, .addr = 0x83fd9014, .val = 0x00000000, },
};

#define APP_DEST	0x90000000

struct imx_flash_header __flash_header_section flash_header = {
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

unsigned long __image_len_section barebox_len = DCD_BAREBOX_SIZE;
