// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2026 Tobias Waldekranz <tobias@waldekranz.com>

#include <block.h>
#include <command.h>
#include <device-mapper.h>
#include <driver.h>
#include <fs.h>
#include <stdio.h>
#include <xfuncs.h>

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/sprintf.h>

#include <lvm.h>

static int lvm_vg_by_devpath(const char *devpath, struct lvm_vg **vgp)
{
	struct cdev *cdev;

	cdev = cdev_by_name(devpath_to_name(devpath));
	if (!cdev)
		return -ENODEV;

	return lvm_vg_alloc_by_cdev(cdev, vgp);
}

static int lvm_vg_by_spec(const char *spec, struct lvm_vg **vgp)
{
	int err;

	err = lvm_vg_by_devpath(spec, vgp);
	switch (err) {
	case 0:
		return 0;
	case -ENODEV:
		err = lvm_vg_alloc_by_name(spec, vgp);
		if (err)
			printf("Found no volume group named \"%s\"\n", spec);

		return err;
	}

	printf("No volume group found on \"%s\"\n", spec);
	return err;
}

/* Total size of a VG, in sectors, i.e. the sum of the extents of all
 * its physical volumes.
 */
static blkcnt_t lvm_vg_size(struct lvm_vg *vg)
{
	blkcnt_t sectors = 0;
	size_t i;

	for (i = 0; i < vg->num_pvs; i++)
		sectors += vg->pvs[i]->pe_count * vg->pe_size;

	return sectors;
}

/* LVM UUIDs are conventionally displayed in 6-4-4-4-4-4-6 groups. */
#define LVM_UUID_STR_LEN (LVM_UUID_LEN + 6)

static const char *lvm_uuid_str(char *buf, const char *uuid)
{
	static const int grp[] = { 6, 4, 4, 4, 4, 4, 6 };
	char *out = buf;
	size_t g;

	for (g = 0; g < ARRAY_SIZE(grp); g++) {
		if (g)
			*out++ = '-';
		memcpy(out, uuid, grp[g]);
		out += grp[g];
		uuid += grp[g];
	}
	*out = '\0';
	return buf;
}

static void lvm_info_vg(struct lvm_vg *vg)
{
	char uuid[LVM_UUID_STR_LEN + 1];
	char esz[32], vsz[32];
	struct lvm_pv *pv;
	struct lvm_lv *lv;
	struct cdev *cdev;
	size_t i;

	strcpy(vsz, size_human_readable(lvm_vg_size(vg) << SECTOR_SHIFT));
	strcpy(esz, size_human_readable(vg->pe_size << SECTOR_SHIFT));

	printf("VG \"%s\":\n"
	       "  UUID:    %s\n"
	       "  SeqNum:  %llu\n"
	       "  Size:    %s (%s extents)\n"
	       "  #PV:     %zu\n"
	       "  #LV:     %zu\n",
	       vg->name, lvm_uuid_str(uuid, vg->uuid), vg->seqno, vsz, esz,
	       vg->num_pvs, vg->num_lvs);

	for (i = 0; i < vg->num_pvs; i++) {
		pv = vg->pvs[i];
		cdev = lvm_pv_cdev(pv);

		printf("  PV %s:\n"
		       "    UUID:  %s\n"
		       "    Size:  %s\n",
		       cdev ? cdev_name(cdev) : "[missing]",
		       lvm_uuid_str(uuid, pv->uuid),
		       size_human_readable((u64)pv->dev_size << SECTOR_SHIFT));
	}

	for (i = 0; i < vg->num_lvs; i++) {
		lv = vg->lvs[i];

		printf("  LV \"%s\":\n"
		       "    UUID:  %s\n"
		       "    Type:  %s\n"
		       "    Size:  %s\n",
		       lv->name, lvm_uuid_str(uuid, lv->uuid),
		       lv->type == LVM_LV_LINEAR ? "linear" : "unknown",
		       size_human_readable((u64)lv->size << SECTOR_SHIFT));
	}
}

static void lvm_info_all(void)
{
	struct lvm_vg_iter *iter;
	struct lvm_vg *vg;
	int i = 0;

	iter = lvm_vg_iter_new();
	while ((vg = lvm_vg_iter_next(iter))) {
		if (i++)
			putchar('\n');

		lvm_info_vg(vg);
		lvm_vg_free(vg);
	}

	lvm_vg_iter_free(iter);
}

static int lvm_info(int argc, char *argv[])
{
	struct lvm_vg *vg = NULL;

	if (argc > 1)
		return COMMAND_ERROR_USAGE;

	if (argc == 0) {
		lvm_info_all();
		return COMMAND_SUCCESS;
	}

	if (lvm_vg_by_spec(argv[0], &vg))
		return COMMAND_ERROR;

	lvm_info_vg(vg);
	lvm_vg_free(vg);
	return COMMAND_SUCCESS;
}

static int lvm_activate_lv(struct lvm_lv *lv, const char *name)
{
	char *defname, *table;
	struct dm_device *dm;

	table = lvm_lv_dm_ctable(lv);
	if (IS_ERR(table)) {
		printf("Cannot map %s/%s: %pe\n", lv->vg->name, lv->name, table);
		return PTR_ERR(table);
	}

	defname = name ? NULL : xasprintf("%s-%s", lv->vg->name, lv->name);

	dm = dm_create(name ? : defname, table);
	free(table);
	if (IS_ERR(dm))
		printf("Failed to create %s: %pe\n", name ? : defname, dm);
	else
		printf("Created %s\n", name ? : defname);

	free(defname);
	return IS_ERR(dm) ? PTR_ERR(dm) : 0;
}

static int lvm_activate(int argc, char *argv[])
{
	struct lvm_vg *vg = NULL;
	struct lvm_lv *lv;
	int err;

	if (argc < 2 || argc > 3)
		return COMMAND_ERROR_USAGE;

	if (lvm_vg_by_spec(argv[0], &vg))
		return COMMAND_ERROR;

	lv = lvm_vg_lv_by_name(vg, argv[1]);
	if (!lv) {
		printf("Logical volume \"%s/%s\" not found\n", vg->name, argv[1]);
		err = -ENOENT;
		goto out_free;
	}

	err = lvm_activate_lv(lv, (argc == 3) ? argv[2] : NULL);

out_free:
	lvm_vg_free(vg);
	return err ? COMMAND_ERROR : COMMAND_SUCCESS;
}

static int do_lvm(int argc, char *argv[])
{
	const char *cmd;

	switch (argc) {
	case 1:
		cmd = "info";
		argc--;
		argv++;
		break;
	default:
		cmd = argv[1];
		argc -= 2;
		argv += 2;
		break;
	}

	if (!strcmp(cmd, "info"))
		return lvm_info(argc, argv);
	else if (!strcmp(cmd, "activate"))
		return lvm_activate(argc, argv);

	printf("Unknown command: %s\n", cmd);
	return COMMAND_ERROR_USAGE;
}

BAREBOX_CMD_HELP_START(lvm)
BAREBOX_CMD_HELP_TEXT("lvm - inspect and activate LVM logical volumes")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Assembles the volume groups described by the LVM physical volumes")
BAREBOX_CMD_HELP_TEXT("found on the available block devices, and activates logical volumes")
BAREBOX_CMD_HELP_TEXT("as device mapper devices.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("A <vg> may be given either as a volume group name or as the path to")
BAREBOX_CMD_HELP_TEXT("a block device holding one of its physical volumes.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("commands:")
BAREBOX_CMD_HELP_OPT("info [<vg>]", "Show volume groups (all if <vg> is omitted)")
BAREBOX_CMD_HELP_OPT("activate <vg> <lv> [<name>]", "Create a dm device for an LV")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(lvm)
	.cmd = do_lvm,
	BAREBOX_CMD_DESC("inspect and activate LVM logical volumes")
	BAREBOX_CMD_OPTS("<command> [args...]")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
	BAREBOX_CMD_HELP(cmd_lvm_help)
BAREBOX_CMD_END
