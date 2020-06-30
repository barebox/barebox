/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#ifndef _ASM_KVX_ELF_H
#define _ASM_KVX_ELF_H

/*
 * ELF register definitions..
 */
#include <linux/types.h>

#define EM_KVX		256

#define ELF_ARCH	EM_KVX
#define ELF_CLASS	ELFCLASS32
#define ELF_DATA	ELFDATA2MSB

#endif	/* _ASM_KVX_ELF_H */

