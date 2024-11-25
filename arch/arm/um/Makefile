# SPDX-License-Identifier: GPL-2.0

obj- := __dummy__.o

ifeq ($(CONFIG_32BIT),y)
	BITS := 32
else
	BITS := 64
endif

AFLAGS_../lib$(BITS)/setjmp.pbl.o := -Dsetjmp=barebox_setjmp -Dlongjmp=barebox_longjmp
pbl-$(CONFIG_SANDBOX_SJLJ_ASM) += ../lib$(BITS)/setjmp.o
