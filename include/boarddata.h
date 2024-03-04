/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _BAREBOX_BOARDDATA_H_
#define _BAREBOX_BOARDDATA_H_

#include <linux/types.h>

struct barebox_boarddata {
#define BAREBOX_BOARDDATA_MAGIC		0xabe742c3
	u32 magic;
#define BAREBOX_MACH_TYPE_EFI		0xef1bbef1
	u32 machine; /* machine number to pass to barebox. This may or may
		      * not be a ARM machine number registered on arm.linux.org.uk.
		      * It must only be unique across barebox. Please use a number
		      * that do not potientially clashes with registered machines,
		      * i.e. use a number > 0x10000.
		      */
#ifdef CONFIG_EFI_STUB
	void *image;
	void *sys_table;
#endif
};

/*
 * Create a boarddata struct at given address. Suitable to be passed
 * as boarddata to barebox_$ARCH_entry(). The boarddata can be retrieved
 * later with barebox_get_boarddata().
 */
static inline struct barebox_boarddata *boarddata_create(void *adr, u32 machine)
{
	struct barebox_boarddata *bd = adr;

	bd->magic = BAREBOX_BOARDDATA_MAGIC;
	bd->machine = machine;

	return bd;
}

const struct barebox_boarddata *barebox_get_boarddata(void);

#endif	/* _BAREBOX_BOARDDATA_H_ */
