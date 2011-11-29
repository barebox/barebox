#ifndef _ASM_NIOS_UNALIGNED_H
#define _ASM_NIOS_UNALIGNED_H

#include <linux/unaligned/le_byteshift.h>
#include <linux/unaligned/be_byteshift.h>
#include <linux/unaligned/generic.h>

/*
 * Select endianness
 */

#define get_unaligned	__get_unaligned_le
#define put_unaligned	__put_unaligned_le

#endif /* _ASM_NIOS_UNALIGNED_H */
