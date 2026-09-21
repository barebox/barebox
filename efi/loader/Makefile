# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_EFI_PE_PARSER) += pe.o

ifeq ($(CONFIG_EFI_LOADER),y)

obj-y += protocols/
obj-y += memory.o pool_alloc.o
obj-y += trace.o
obj-y += table.o
obj-y += devicepath.o
obj-y += debug_support.o
obj-y += boot.o
obj-y += runtime.o
obj-y += setup.o
obj-y += watchdog.o
obj-y += loadopts.o
obj-y += efi_var_common.o
obj-y += efi_variable.o
obj-y += efi_var_mem.o
obj-y += efi_var_file.o
obj-y += defaultvars.o

obj-$(CONFIG_BOOT) += bootesp.o
obj-$(CONFIG_EFI_LOADER_BOOTMGR) += efibootmgr.o
obj-$(CONFIG_BOOTM) += bootm.o
obj-$(CONFIG_GENERATE_SMBIOS_TABLE) += smbios.o

endif
