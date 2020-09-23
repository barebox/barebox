#include <common.h>
#include <generated/compile.h>
#include <generated/utsrelease.h>

const char version_string[] =
	"barebox " UTS_RELEASE " " UTS_VERSION "\n";
EXPORT_SYMBOL(version_string);

const char release_string[] =
	"barebox-" UTS_RELEASE;
EXPORT_SYMBOL(release_string);

const char buildsystem_version_string[] =
	BUILDSYSTEM_VERSION;
EXPORT_SYMBOL(buildsystem_version_string);

#ifdef CONFIG_BANNER
void barebox_banner (void)
{
	printf("\n\n");
	pr_info("%s", version_string);
	if (strlen(buildsystem_version_string) > 0)
		pr_info("Buildsystem version: %s", buildsystem_version_string);
	printf("\n\n");
	pr_info("Board: %s\n", barebox_get_model());
}
#endif
