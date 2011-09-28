#include <common.h>
#include <generated/utsrelease.h>

const char version_string[] =
	"barebox " UTS_RELEASE " (" __DATE__ " - " __TIME__ ")";

void barebox_banner (void)
{
	printf("\n\n%s\n\n", version_string);
	printf("Board: " CONFIG_BOARDINFO "\n");
}

