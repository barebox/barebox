# SPDX-License-Identifier: GPL-2.0

obj- := __dummy__.o

AFLAGS_../lib/setjmp.pbl.o := -Dsetjmp=barebox_setjmp -Dlongjmp=barebox_longjmp
pbl-$(CONFIG_SANDBOX_SJLJ_ASM) += ../lib/setjmp.o
