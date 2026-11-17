/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __PBL_LAYOUT_H
#define __PBL_LAYOUT_H

/* p_type of barebox proper's segment holding barebox_layout (PT_LOOS based) */
#define PT_BAREBOX_LAYOUT	0x6bb00001

#ifndef __ASSEMBLY__

#include <linux/types.h>

struct elf_image;

/*
 * The memory barebox proper runs in and the areas it uses outside of its
 * image. The PBL fills this in after loading barebox proper and before
 * mapping its read-only segments, so barebox proper finds it initialized
 * and can't modify it. Areas with start == end are not present.
 */
struct barebox_layout {
	ulong membase, memsize;
	struct {
		ulong start, end;
	} ramoops, guard_page, stack, scratch;
};

extern const struct barebox_layout barebox_layout;

struct barebox_layout *pbl_barebox_layout(struct elf_image *elf,
					  ulong membase, ulong memsize);

#endif

#endif
