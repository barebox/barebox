
CPPFLAGS += -D__I386__ -fno-strict-aliasing -m32 -g -Os -march=i386 \
	-mregparm=3 -fno-strict-aliasing -fomit-frame-pointer -ffreestanding \
	-fno-toplevel-reorder -fno-unit-at-a-time -fno-stack-protector \
	-mpreferred-stack-boundary=2

obj-$(CONFIG_X86_HDBOOT)	+= boot_main.o boot_hdisk.o

obj-$(CONFIG_X86_BIOS_BRINGUP)	+= prepare_uboot.o a20.o bioscall.o regs.o tty.o pmjump.o main_entry.o

obj-$(CONFIG_X86_VESA) += console_vesa.o
obj-$(CONFIG_X86_VGA) += console_vga.o
obj-$(CONFIG_X86_SERIAL) += console_serial.o
