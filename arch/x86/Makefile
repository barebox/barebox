CPPFLAGS += -D__X86__ -fno-strict-aliasing

board-y := x86_generic
machine-y := i386

TEXT_BASE = $(CONFIG_TEXT_BASE)

CPPFLAGS += -march=i386 -m32 -DTEXT_BASE=$(TEXT_BASE)
LDFLAGS += -m elf_i386

ifndef CONFIG_MODULES
# Add cleanup flags
CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS_uboot += -static --gc-sections
endif

machdirs := $(patsubst %,arch/x86/mach-%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
CPPFLAGS += $(patsubst %,-I%include,$(machdirs))
else
CPPFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

ifneq ($(board-y),)
BOARD  := arch/x86/boards/$(board-y)/
else
BOARD  :=
endif

ifneq ($(machine-y),)
MACH  := arch/x86/mach-$(machine-y)/
else
MACH  :=
endif

common-y += $(BOARD) $(MACH)
common-y += arch/x86/lib/
common-y += arch/x86/boot/

# arch/x86/cpu/

lds-$(CONFIG_GENERIC_LINKER_SCRIPT)   := arch/x86/lib/barebox.lds
lds-$(CONFIG_BOARD_LINKER_SCRIPT)     := $(BOARD)/barebox.lds

CLEAN_FILES    += arch/x86/lib/barebox.lds barebox.map barebox.S

