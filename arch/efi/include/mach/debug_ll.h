#ifndef __MACH_DEBUG_LL_H__
#define __MACH_DEBUG_LL_H__

#define EFI_DEBUG 0
#define EFI_DEBUG_CLEAR_MEMORY 0

#include <efi.h>
#include <efi/efi.h>

static inline void PUTC_LL(char c)
{
	uint16_t str[2] = {};
	struct efi_simple_text_output_protocol *con_out = efi_sys_table->con_out;

	str[0] = c;

	con_out->output_string(con_out, str);
}

#endif
