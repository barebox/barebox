// SPDX-License-Identifier: GPL-2.0-only

#include <lib/elf.h>
#include <pbl/layout.h>
#include <printf.h>

#ifdef __PBL__
/* find the layout in a barebox proper loaded with elf_load_inplace() */
struct barebox_layout *pbl_barebox_layout(struct elf_image *elf,
					  ulong membase, ulong memsize)
{
	struct barebox_layout *layout;
	u64 size;

	/* ld.lld may place orphan sections behind it in the same segment */
	layout = elf_find_segment(elf, PT_BAREBOX_LAYOUT, &size);
	if (!layout || size < sizeof(*layout))
		panic("No barebox layout segment found\n");

	layout->membase = membase;
	layout->memsize = memsize;

	return layout;
}
#else
/* filled in by the PBL, so the compiler must not see it read here */
const struct barebox_layout barebox_layout __section(.barebox_layout);
#endif
