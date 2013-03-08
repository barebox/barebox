#include <common.h>
#include <generated/compile.h>
#include <generated/utsrelease.h>
#include <of.h>

const char version_string[] =
	"barebox " UTS_RELEASE " " UTS_VERSION "\n";
EXPORT_SYMBOL(version_string);

void barebox_banner (void)
{
	const char *board;

	board = of_get_model();

	if (!board)
		board = CONFIG_BOARDINFO;

	pr_info("\n\n%s\n\n", version_string);
	pr_info("Board: %s\n", board);
}
