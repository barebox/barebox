#include <asm-generic/barebox.lds.h>

SECTIONS
{
	. = ALIGN(64);
	.barebox_rodata : {
		RO_DATA_SECTION
	}
}

INSERT BEFORE .rodata;
