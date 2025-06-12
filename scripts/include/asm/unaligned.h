/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _ASM_ARM_UNALIGNED_H
#define _ASM_ARM_UNALIGNED_H

#include "../../../include/linux/unaligned/le_byteshift.h"
#include "../../../include/linux/unaligned/be_byteshift.h"
#include "../../../include/linux/unaligned/generic.h"

/*
 * Select endianness
 */
#ifdef CONFIG_CPU_BIG_ENDIAN
#define get_unaligned	__get_unaligned_be
#define put_unaligned	__put_unaligned_be
#else
#define get_unaligned	__get_unaligned_le
#define put_unaligned	__put_unaligned_le
#endif

#endif /* _ASM_ARM_UNALIGNED_H */
