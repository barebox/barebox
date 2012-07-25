#include <common.h>
#include <generated/compile.h>
#include <generated/utsrelease.h>

const char version_string[] =
	"barebox " UTS_RELEASE " " UTS_VERSION "\n";
EXPORT_SYMBOL(version_string);

void barebox_banner (void)
{
	printf("\n\n%s\n\n", version_string);
	printf("Board: " CONFIG_BOARDINFO "\n");
}

