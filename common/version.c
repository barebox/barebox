#include <common.h>
#include <generated/compile.h>
#include <generated/utsrelease.h>

const char version_string[] =
	"barebox " UTS_RELEASE " " UTS_VERSION "\n";
EXPORT_SYMBOL(version_string);

void barebox_banner (void)
{
	pr_info("\n\n%s\n\n", version_string);
	pr_info("Board: %s\n", barebox_get_model());
}
