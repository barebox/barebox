#ifndef _ASM_RISCV_UNALIGNED_H
#define _ASM_RISCV_UNALIGNED_H

/*
 * FIXME: this file is copy-n-pasted from sandbox's unaligned.h
 */

#include <linux/unaligned/access_ok.h>
#include <linux/unaligned/generic.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define get_unaligned __get_unaligned_le
#define put_unaligned __put_unaligned_le
#else
#define get_unaligned __get_unaligned_be
#define put_unaligned __put_unaligned_be
#endif

#endif /* _ASM_RISCV_UNALIGNED_H */
