// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <generated/compile.h>
#include <generated/utsrelease.h>
#include <linux/kasan-enabled.h>

const char version_string[] =
	"barebox " UTS_RELEASE " " UTS_VERSION;
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
	bool have_asan, have_ubsan;

	printf("\n\n");
	pr_info("%s\n", version_string);
	if (strlen(buildsystem_version_string) > 0)
		pr_info("Buildsystem version: %s\n", buildsystem_version_string);
	printf("\n\n");
	pr_info("Board: %s\n", barebox_get_model());

	have_asan = kasan_enabled();
	have_ubsan = IS_ENABLED(CONFIG_UBSAN);

	if (have_asan || have_ubsan)
		pr_info("Active sanitizers:%s%s%s\n",
			have_asan && IS_ENABLED(CONFIG_KASAN) ? " K" : " ",
			have_asan ? "ASAN" : "",
			have_ubsan ? " UBSAN" : "");
}
#endif
