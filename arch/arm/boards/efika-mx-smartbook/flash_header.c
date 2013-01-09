#include <common.h>
#include <mach/imx-flash-header.h>
#include <asm/barebox-arm-head.h>

void __naked __flash_header_start go(void)
{
	barebox_arm_head();
}

#define DCD_NAME struct imx_dcd_entry __dcd_entry_section dcd_entry

#include "dcd-data.h"

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

