#include <common.h>
#include <reloc.h>
#include <generated/utsrelease.h>

const char version_string[] =
	"barebox " UTS_RELEASE " (" __DATE__ " - " __TIME__ ")";

void barebox_banner (void)
{
	printf (RELOC("\n\n%s\n\n"), RELOC_VAR(version_string));
	printf(RELOC("Board: " CONFIG_BOARDINFO "\n"));
}

