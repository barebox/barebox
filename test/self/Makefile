# SPDX-License-Identifier: GPL-2.0

obj-$(CONFIG_SELFTEST) += core.o
obj-$(CONFIG_SELFTEST_MALLOC) += malloc.o
obj-$(CONFIG_SELFTEST_PRINTF) += printf.o
CFLAGS_printf.o += -Wno-format-security -Wno-format
obj-$(CONFIG_SELFTEST_PROGRESS_NOTIFIER) += progress-notifier.o
obj-$(CONFIG_SELFTEST_OF_MANIPULATION) += of_manipulation.o of_manipulation.dtb.o
obj-$(CONFIG_SELFTEST_ENVIRONMENT_VARIABLES) += envvar.o
obj-$(CONFIG_SELFTEST_FS_RAMFS) += ramfs.o
obj-$(CONFIG_SELFTEST_DIRFD) += dirfd.o
obj-$(CONFIG_SELFTEST_JSON) += json.o
obj-$(CONFIG_SELFTEST_JWT) += jwt.o jwt_test.pem.o
obj-$(CONFIG_SELFTEST_DIGEST) += digest.o
obj-$(CONFIG_SELFTEST_MMU) += mmu.o
obj-$(CONFIG_SELFTEST_STRING) += string.o
obj-$(CONFIG_SELFTEST_SETJMP) += setjmp.o
obj-$(CONFIG_SELFTEST_REGULATOR) += regulator.o test_regulator.dtbo.o
obj-$(CONFIG_SELFTEST_TEST_COMMAND) += test_command.o
obj-$(CONFIG_SELFTEST_IDR) += idr.o

ifdef REGENERATE_RSATOC

$(obj)/jwt_test.pem.c_shipped: $(src)/jwt_test.pem FORCE
	$(call if_changed,rsa_keys,$(basename $(target-stem)):$<,-s)

endif

clean-files := *.dtb *.dtb.S .*.dtc .*.pre .*.dts *.dtb.z
clean-files += *.dtbo *.dtbo.S .*.dtso
clean-files += *.pem.c
