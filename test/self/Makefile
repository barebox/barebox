# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_SELFTEST) += core.o
obj-$(CONFIG_SELFTEST_MALLOC) += malloc.o
obj-$(CONFIG_SELFTEST_PRINTF) += printf.o
CFLAGS_printf.o += -Wno-format-security
obj-$(CONFIG_SELFTEST_PROGRESS_NOTIFIER) += progress-notifier.o
obj-$(CONFIG_SELFTEST_OF_MANIPULATION) += of_manipulation.o of_manipulation.dtb.o
obj-$(CONFIG_SELFTEST_ENVIRONMENT_VARIABLES) += envvar.o
obj-$(CONFIG_SELFTEST_FS_RAMFS) += ramfs.o
obj-$(CONFIG_SELFTEST_JSON) += json.o
obj-$(CONFIG_SELFTEST_DIGEST) += digest.o
obj-$(CONFIG_SELFTEST_MMU) += mmu.o

clean-files := *.dtb *.dtb.S .*.dtc .*.pre .*.dts *.dtb.z
clean-files += *.dtbo *.dtbo.S .*.dtso
