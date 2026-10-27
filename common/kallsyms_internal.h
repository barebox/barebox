/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/kernel/kallsyms_internal.h?id=8d3ae59288f1e7d58d76558a6ee96d533bc5019f */
#ifndef LINUX_KALLSYMS_INTERNAL_H_
#define LINUX_KALLSYMS_INTERNAL_H_

#include <linux/types.h>

extern const int kallsyms_offsets[];
extern const u8 kallsyms_names[];

extern const unsigned int kallsyms_num_syms;

extern const char kallsyms_token_table[];
extern const u16 kallsyms_token_index[];

extern const unsigned int kallsyms_markers[];
extern const u8 kallsyms_seqs_of_names[];

#endif // LINUX_KALLSYMS_INTERNAL_H_
