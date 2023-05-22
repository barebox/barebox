# SPDX-License-Identifier: GPL-2.0-only

obj-y				+= boards/
obj-y				+= memory.o
obj-y				+= memory_display.o
pbl-$(CONFIG_PBL_CONSOLE)	+= memory_display.o
obj-y				+= clock.o
obj-y				+= console_common.o
obj-$(CONFIG_OFDEVICE)		+= deep-probe.o
obj-y				+= startup.o
obj-y				+= misc.o
obj-pbl-y			+= memsize.o
obj-y				+= resource.o
obj-pbl-y			+= bootsource.o
obj-$(CONFIG_ELF)		+= elf.o
obj-y				+= restart.o
obj-y				+= poweroff.o
obj-y				+= slice.o
obj-y				+= workqueue.o
obj-$(CONFIG_MACHINE_ID)	+= machine_id.o
obj-$(CONFIG_AUTO_COMPLETE)	+= complete.o
obj-y				+= version.o
obj-$(CONFIG_BAREBOX_UPDATE)	+= bbu.o
obj-$(CONFIG_SYSTEM_PARTITIONS) += system-partitions.o
obj-$(CONFIG_BINFMT)		+= binfmt.o
obj-$(CONFIG_BLOCK)		+= block.o
obj-$(CONFIG_BLSPEC)		+= blspec.o
obj-$(CONFIG_BOOTM)		+= bootm.o booti.o
obj-$(CONFIG_CMD_LOADS)		+= s_record.o
obj-$(CONFIG_MEMTEST)		+= memtest.o
obj-$(CONFIG_COMMAND_SUPPORT)	+= command.o
obj-$(CONFIG_CONSOLE_FULL)	+= console.o
obj-$(CONFIG_CONSOLE_SIMPLE)	+= console_simple.o
obj-y				+= console_countdown.o
obj-pbl-$(CONFIG_DDR_SPD)	+= ddr_spd.o
obj-pbl-$(CONFIG_DDR_SPD)	+= ddr1_dimm_params.o
obj-pbl-$(CONFIG_DDR_SPD)	+= ddr2_dimm_params.o
obj-pbl-$(CONFIG_DDR_SPD)	+= ddr3_dimm_params.o
obj-pbl-$(CONFIG_DDR_SPD)	+= ddr4_dimm_params.o
obj-$(CONFIG_ENV_HANDLING)	+= environment.o envfs-core.o
obj-$(CONFIG_DEFAULT_ENVIRONMENT) += envfs-core.o
obj-$(CONFIG_ENVIRONMENT_VARIABLES) += env.o
obj-pbl-$(CONFIG_FILETYPE)	+= filetype.o
CFLAGS_filetype.o = -I$(srctree)/arch/
CFLAGS_filetype.pbl.o = -I$(srctree)/arch/
obj-$(CONFIG_FLEXIBLE_BOOTARGS)	+= bootargs.o
obj-$(CONFIG_GLOBALVAR)		+= globalvar.o
obj-$(CONFIG_GREGORIAN_CALENDER) += date.o
obj-$(CONFIG_KALLSYMS)		+= kallsyms.o
obj-$(CONFIG_MALLOC_DLMALLOC)	+= dlmalloc.o
obj-$(CONFIG_MALLOC_TLSF)	+= tlsf_malloc.o tlsf.o calloc.o
KASAN_SANITIZE_tlsf.o := n
obj-$(CONFIG_MALLOC_DUMMY)	+= dummy_malloc.o calloc.o
obj-$(CONFIG_MEMINFO)		+= meminfo.o
obj-$(CONFIG_MENU)		+= menu.o
obj-$(CONFIG_MODULES)		+= module.o
extra-$(CONFIG_MODULES)		+= module.lds
obj-$(CONFIG_OFTREE)		+= oftree.o
obj-$(CONFIG_PARTITION_DISK)	+= partitions.o partitions/
obj-$(CONFIG_PASSWORD)		+= password.o
obj-$(CONFIG_HAS_SCHED)		+= sched.o
obj-$(CONFIG_POLLER)		+= poller.o
obj-$(CONFIG_BTHREAD)		+= bthread.o
obj-$(CONFIG_RESET_SOURCE)	+= reset_source.o
obj-$(CONFIG_SHELL_HUSH)	+= hush.o
obj-$(CONFIG_SHELL_SIMPLE)	+= parser.o
obj-$(CONFIG_STATE)		+= state/
obj-$(CONFIG_RATP)		+= ratp/
obj-$(CONFIG_BOOTCHOOSER)	+= bootchooser.o
obj-$(CONFIG_UIMAGE)		+= image.o uimage.o
obj-$(CONFIG_FITIMAGE)		+= image-fit.o
obj-$(CONFIG_MENUTREE)		+= menutree.o
obj-$(CONFIG_EFI)		+= efi/
lwl-$(CONFIG_IMD)		+= imd-barebox.o
obj-$(CONFIG_IMD)		+= imd.o
obj-y				+= file-list.o
obj-$(CONFIG_FIRMWARE)		+= firmware.o
obj-$(CONFIG_UBIFORMAT)		+= ubiformat.o
obj-$(CONFIG_BAREBOX_UPDATE_IMX_NAND_FCB) += imx-bbu-nand-fcb.o
obj-$(CONFIG_BOOT)		+= boot.o
obj-$(CONFIG_SERIAL_DEV_BUS)	+= serdev.o
obj-$(CONFIG_USBGADGET_START)	+= usbgadget.o
pbl-$(CONFIG_PBL_OPTEE)		+= optee.o
obj-$(CONFIG_BOOTM_OPTEE)	+= optee.o
obj-$(CONFIG_FASTBOOT_BASE)	+= fastboot.o

ifdef CONFIG_PASSWORD

ifeq ($(CONFIG_PASSWORD_DEFAULT),"")
define filechk_passwd
	echo "static const char default_passwd[] = \"\";"
endef
else
define filechk_passwd
(							\
echo -n "static const char default_passwd[] = \"";	\
cat $(shell find $(srctree) -name $(CONFIG_PASSWORD_DEFAULT) -type f) | tr -d '\n';\
echo "\";"						\
)
endef
endif

include/generated/passwd.h: FORCE
	$(call filechk,passwd)

$(obj)/password.o: include/generated/passwd.h
endif # CONFIG_PASSWORD

# dependencies on generated files need to be listed explicitly
$(obj)/version.o: include/generated/compile.h
$(obj)/imd-barebox.o: include/generated/compile.h
$(obj)/imd-barebox.pbl.o: include/generated/compile.h

# compile.h changes depending on hostname, generation number, etc,
# so we regenerate it always.
# mkcompile_h will make sure to only update the
# actual file if its content has changed.

       chk_compile.h = :
 quiet_chk_compile.h = echo '  CHK     $@'
silent_chk_compile.h = :
include/generated/compile.h: FORCE
	@$($(quiet)chk_compile.h)
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/mkcompile_h $@ \
	"$(UTS_MACHINE)" "$(CC) $(KBUILD_CFLAGS)"

