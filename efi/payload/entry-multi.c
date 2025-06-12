// SPDX-License-Identifier: GPL-2.0-only

#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/sizes.h>
#include <stdio.h>
#include <efi.h>
#include <asm/common.h>
#include <efi/efi-util.h>
#include <efi/efi-payload.h>
#include <pbl.h>
#include <pbl/handoff-data.h>

asmlinkage void __efistub_efi_pe_entry(void *image, struct efi_system_table *sys_table);

static void efi_putc(void *ctx, int ch)
{
	struct efi_system_table *sys_table = ctx;
	wchar_t ws[2] = { ch, L'\0' };

	sys_table->con_out->output_string(sys_table->con_out, ws);
}

void __efistub_efi_pe_entry(void *image, struct efi_system_table *sys_table)
{
	size_t memsize;
	void *mem;
	static struct barebox_efi_data efidata;

#ifdef DEBUG
	sys_table->con_out->output_string(sys_table->con_out, L"\nbarebox\n");
#endif
	pbl_set_putc(efi_putc, sys_table);

	efidata.image = image;
	efidata.sys_table = sys_table;

	handoff_data_add(HANDOFF_DATA_EFI, &efidata, sizeof(efidata));

	mem = efi_earlymem_alloc(sys_table, &memsize);

	barebox_pbl_entry((uintptr_t)mem, memsize, NULL);
}
