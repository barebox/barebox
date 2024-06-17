/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#include <linux/linkage.h>

/**
 * We do not aim at handling exceptions but at least, we want the CPU to
 * stop when taking one.
 * Note that we can not expect to be able to issue a print since
 * it might be the cause of trap...
 */
#define exception_stub(__type) \
.section .exception.## __type, "ax", @progbits ;\
ENTRY(kvx_## __type ##_early_handler): ;\
	goto . ;\
	;; ;\
ENDPROC(kvx_ ## __type ## _early_handler)

exception_stub(debug)
exception_stub(trap)
exception_stub(interrupt)

/**
 * The only time this handle syscalls is when debug routines are not present
 * (which can happen when booted without JTAG). For instance the "magic
 * console" uses a syscall catched by the debug routine and if not handled
 * barebox will crash.
 */
.section .exception.syscall, "ax", @progbits
ENTRY(kvx_syscall_handler):
	rfe
	;;
ENDPROC(kvx_syscall_handler)
