extra-$(CONFIG_GENERIC_LINKER_SCRIPT) += barebox.lds
obj-y += memory.o
obj-y += gdt.o

# needed, when running via a 16 bit BIOS
obj-$(CONFIG_X86_BIOS_BRINGUP) += memory16.o
obj-$(CONFIG_X86_BIOS_BRINGUP) += traveler.o
