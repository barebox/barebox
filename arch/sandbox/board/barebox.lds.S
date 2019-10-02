#include <asm-generic/barebox.lds.h>

SECTIONS
{
	. = ALIGN(64);
	.barebox_initcalls : { BAREBOX_INITCALLS }
	.barebox_exitcalls : { BAREBOX_EXITCALLS }
	.barebox_magicvar : { BAREBOX_MAGICVARS }
	__barebox_cmd : { BAREBOX_CMDS }
	__barebox_ratp_cmd : { BAREBOX_RATP_CMDS }
}

INSERT BEFORE .rodata;
