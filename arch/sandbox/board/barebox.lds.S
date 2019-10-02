#include <asm-generic/barebox.lds.h>

SECTIONS
{
	. = ALIGN(64);
	.barebox_initcalls : { BAREBOX_INITCALLS }

	. = ALIGN(64);
	.barebox_exitcalls : { BAREBOX_EXITCALLS }

	. = ALIGN(64);
	.barebox_magicvar : { BAREBOX_MAGICVARS }

	. = ALIGN(64);
	__barebox_cmd : { BAREBOX_CMDS }

	. = ALIGN(64);
	__barebox_ratp_cmd : { BAREBOX_RATP_CMDS }
}

INSERT BEFORE .rodata;
