#include <asm-generic/barebox.lds.h>

SECTIONS
{
	. = ALIGN(64);
	__barebox_initcalls_start = .;
	__barebox_initcalls : { INITCALLS }
	__barebox_initcalls_end = .;

	. = ALIGN(64);
	__barebox_exitcalls_start = .;
	__barebox_exitcalls : { EXITCALLS }
	__barebox_exitcalls_end = .;

	. = ALIGN(64);
	__barebox_magicvar_start = .;
	.barebox_magicvar : { BAREBOX_MAGICVARS }
	__barebox_magicvar_end = .;

	. = ALIGN(64);
	__barebox_cmd_start = .;
	__barebox_cmd : { BAREBOX_CMDS }
	__barebox_cmd_end = .;

	. = ALIGN(64);
	__barebox_ratp_cmd_start = .;
	__barebox_ratp_cmd : { BAREBOX_RATP_CMDS }
	__barebox_ratp_cmd_end = .;
}

INSERT BEFORE .rodata;
