# SPDX-License-Identifier: GPL-2.0-only

obj-y += stacktrace.o
obj-$(CONFIG_ARM_LINUX)	+= armlinux.o
obj-y	+= div0.o
obj-$(CONFIG_ARM_OPTIMZED_STRING_FUNCTIONS)	+= memcpy.o
obj-$(CONFIG_ARM_OPTIMZED_STRING_FUNCTIONS)	+= memset.o string.o
extra-y += barebox.lds
obj-pbl-y   += runtime-offset.o
obj-pbl-y   += setjmp.o
obj-pbl-y   += reloc.o
obj-y += io.o
pbl-$(CONFIG_CLOCKSOURCE_ARM_ARCHITECTED_TIMER)	+= clocksource.o
pbl-y	+= div0.o
