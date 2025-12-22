// SPDX-License-Identifier: GPL-2.0-or-later

#define pr_fmt(fmt) "efi-loader: efibootmgr: " fmt

#include <boot.h>
#include <driver.h>

struct resolve_ctx {
	struct bootentry_provider *provider;
	struct bootentries *entries;
};

static int populate_esp_bootentries(struct cdev *cdev, void *_ctx)
{
	struct resolve_ctx *ctx = _ctx;

	pr_debug("processing %s\n", cdev->name);

	return ctx->provider->generate(ctx->entries, cdev->name);
}

static int efibootmgr_add_entry_from_bootvars(struct bootentries *entries,
					      const char *name)
{
	/* TODO: actually make use of the Boot# variables
	 * instead of only hardcoding order
	 */
	return 0;
}

static int efibootmgr_add_entry_from_fallback(struct bootentries *entries,
					      const char *name)
{
	struct resolve_ctx ctx;
	int nremovable, nbuiltin;

	ctx.provider = get_bootentry_provider("esp");
	if (!ctx.provider)
		return -ENOENT;

	ctx.entries = entries;

	nremovable = cdev_alias_resolve_for_each("storage.removable",
						 populate_esp_bootentries, &ctx);
	if (nremovable < 0)
		return nremovable;

	nbuiltin = cdev_alias_resolve_for_each("storage.builtin",
					       populate_esp_bootentries, &ctx);
	if (nbuiltin < 0)
		return nbuiltin;

	return nbuiltin + nremovable;
}

static int efibootmgr_add_entry(struct bootentries *entries, const char *name)
{
	int nentries;

	if (strcmp(name, "efibootmgr"))
		return 0;

	nentries = efibootmgr_add_entry_from_bootvars(entries, name);
	if (nentries)
		return nentries;

	return efibootmgr_add_entry_from_fallback(entries, name);
}

static struct bootentry_provider efibootmgr_entry_provider = {
	.name = "efibootmgr",
	.generate = efibootmgr_add_entry,
};

static int efibootmgr_init(void)
{
	return bootentry_register_provider(&efibootmgr_entry_provider);
}
device_initcall(efibootmgr_init);
