/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __PSTORE_INTERNAL_H__
#define __PSTORE_INTERNAL_H__

#include <linux/types.h>
#include <linux/time.h>
#include <linux/pstore.h>

#define PSTORE_NAMELEN  64

extern struct pstore_info *psinfo;

extern void	pstore_set_kmsg_bytes(int);
extern void	pstore_get_records(int);
extern int	pstore_mkfile(struct pstore_record *record);
extern int	pstore_is_mounted(void);

#endif
