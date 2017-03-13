extra-$(CONFIG_GENERIC_LINKER_SCRIPT) += barebox.lds
ifneq ($(CONFIG_X86_EFI),y)
obj-y += memory.o
obj-y += gdt.o
endif

# needed, when running via a 16 bit BIOS
obj-$(CONFIG_CMD_LINUX16) += linux_start.o
