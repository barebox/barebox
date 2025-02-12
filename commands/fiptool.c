/*
 * Copyright (c) 2016-2023, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Imported from TF-A v2.10.0
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/log2.h>
#include <linux/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <libfile.h>
#include <unistd.h>
#include <command.h>
#include <malloc.h>
#include <crypto/sha.h>
#include <linux/kstrtox.h>
#include <digest.h>
#include <stdio.h>

#include <fip.h>
#include <fiptool.h>

typedef struct cmd {
	char              *name;
	int              (*handler)(struct fip_state *fip, int, char **);
	void             (*usage)(int);
} cmd_t;


static int write_image_to_file(const struct fip_image *image, const char *filename)
{
	int fd;

	fd = creat(filename, 0777);
	if (fd < 0) {
		pr_err("creat %s: %m\n", filename);
		return -errno;
	}

	if (write_full(fd, image->buffer, image->toc_e.size) < 0) {
		pr_err("Failed to write %s: %m\n", filename);
		return -errno;
	}

	close(fd);
	return 0;
}


static int info_cmd(struct fip_state *fip, int argc, char *argv[])
{
	struct fip_image_desc *desc;
	fip_toc_header_t toc_header;
	int ret;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	argc--, argv++;

	ret = parse_fip(fip, argv[0], &toc_header);
	if (ret)
		return ret;

	pr_verbose("toc_header[name]: 0x%llX\n",
	    (unsigned long long)toc_header.name);
	pr_verbose("toc_header[serial_number]: 0x%llX\n",
	    (unsigned long long)toc_header.serial_number);
	pr_verbose("toc_header[flags]: 0x%llX\n",
	    (unsigned long long)toc_header.flags);

	fip_for_each_desc(fip, desc) {
		struct fip_image *image = desc->image;

		if (image == NULL)
			continue;
		printf("%s: offset=0x%llX, size=0x%llX, cmdline=\"-e %s\"",
		       desc->name,
		       (unsigned long long)image->toc_e.offset_address,
		       (unsigned long long)image->toc_e.size,
		       desc->cmdline_name);

		/*
		 * Omit this informative code portion for:
		 * Visual Studio missing SHA256.
		 * Statically linked builds.
		 */
		if (IS_ENABLED(CONFIG_HAVE_DIGEST_SHA256) && fip->verbose) {
			struct digest *sha256;
			unsigned char md[SHA256_DIGEST_SIZE];
			int err;

			sha256 = digest_alloc("sha256");
			if (!sha256)
				continue;

			err = digest_digest(sha256, image->buffer, image->toc_e.size, md);
			if (err)
				continue;

			digest_free(sha256);

			printf(", sha256=%*phN", (int)sizeof(md), md);
		}
		putchar('\n');
	}

	return 0;
}


static int uuid_cmd(struct fip_state *fip, int argc, char *argv[])
{
	for (toc_entry_t *t = toc_entries; t->cmdline_name != NULL; t++)
		printf("%pU\t%-16s\t%s\n", &t->uuid, t->cmdline_name, t->name);
	for (toc_entry_t *t = plat_def_toc_entries; t->cmdline_name != NULL; t++)
		printf("%pU\t%-16s\t%s\n", &t->uuid, t->cmdline_name, t->name);
	return 0;
}


static int parse_plat_toc_flags(const char *arg, unsigned long long *toc_flags)
{
	unsigned long long flags;
	char *endptr;

	errno = 0;
	flags = simple_strtoull(arg, &endptr, 16);
	if (*endptr != '\0' || flags > U16_MAX) {
		pr_err("Invalid platform ToC flags: %s\n", arg);
		return -1;
	}
	/* Platform ToC flags is a 16-bit field occupying bits [32-47]. */
	*toc_flags |= flags << 32;

	return 0;
}

static long get_image_align(char *arg)
{
	char *endptr;
	long align;

	errno = 0;
	align = simple_strtol(arg, &endptr, 0);
	if (*endptr != '\0' || align < 0 || !is_power_of_2(align)) {
		pr_err("Invalid alignment: %s\n", arg);
		return -1;
	}

	return align;
}

static void parse_blob_opt(char *arg, uuid_t *uuid, char *filename, size_t len)
{
	char *p, *val;

	while ((p = strsep(&arg, ",")) != NULL) {
		val = p + str_has_prefix(p, "uuid=");
		if (val != p) {
			uuid_parse(val, uuid);
			continue;
		}

		val = p + str_has_prefix(p, "file=");
		if (val != p) {
			snprintf(filename, len, "%s", val);
			continue;
		}
	}
}

static inline bool file_exists(const char *filename)
{
	struct stat st;

	return stat(filename, &st) == 0;
}

static __maybe_unused int create_cmd(struct fip_state *fip, int argc, char *argv[])
{
	unsigned long long toc_flags = 0;
	long align = 1;
	int opt;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	while ((opt = getopt(argc, argv, "e:p:a:b:")) > 0) {
		switch (opt) {
		case 'e': {
			struct fip_image_desc *desc;

			desc = lookup_image_desc_from_opt(fip, &optarg);
			if (!desc)
				return COMMAND_ERROR;
			set_image_desc_action(desc, DO_PACK, optarg);
			break;
		}
		case 'p':
			if (parse_plat_toc_flags(optarg, &toc_flags))
				return COMMAND_ERROR;
			break;
		case 'a':
			align = get_image_align(optarg);
			if (align < 0)
				return COMMAND_ERROR;
			break;
		case 'b': {
			char name[UUID_STRING_LEN + 1];
			char filename[PATH_MAX] = { 0 };
			uuid_t uuid = uuid_null;
			struct fip_image_desc *desc;

			parse_blob_opt(optarg, &uuid,
			    filename, sizeof(filename));

			if (memcmp(&uuid, &uuid_null, sizeof(uuid_t)) == 0 ||
			    filename[0] == '\0')
				return COMMAND_ERROR_USAGE;

			desc = lookup_image_desc_from_uuid(fip, &uuid);
			if (desc == NULL) {
				snprintf(name, sizeof(name), "%pU", &uuid);
				desc = new_image_desc(&uuid, name, "blob");
				add_image_desc(fip, desc);
			}
			set_image_desc_action(desc, DO_PACK, filename);
			break;
		}
		default:
			return COMMAND_ERROR_USAGE;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		return COMMAND_ERROR_USAGE;

	if (update_fip(fip))
		return COMMAND_ERROR;

	return pack_images(fip, argv[0], toc_flags, align);
}


static __maybe_unused int update_cmd(struct fip_state *fip, int argc, char *argv[])
{
	char outfile[PATH_MAX] = { 0 };
	fip_toc_header_t toc_header = { 0 };
	unsigned long long toc_flags = 0;
	long align = 1;
	int pflag = 0;
	int ret, opt;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	while ((opt = getopt(argc, argv, "e:p:b:a:o:")) > 0) {
		switch (opt) {
		case 'e': {
			struct fip_image_desc *desc;

			desc = lookup_image_desc_from_opt(fip, &optarg);
			if (!desc)
				return COMMAND_ERROR;
			set_image_desc_action(desc, DO_PACK, optarg);
			break;
		}
		case 'p':
			if (parse_plat_toc_flags(optarg, &toc_flags))
				return COMMAND_ERROR;
			pflag = 1;
			break;
		case 'b': {
			char name[UUID_STRING_LEN + 1];
			char filename[PATH_MAX] = { 0 };
			uuid_t uuid = uuid_null;
			struct fip_image_desc *desc;

			parse_blob_opt(optarg, &uuid,
			    filename, sizeof(filename));

			if (memcmp(&uuid, &uuid_null, sizeof(uuid_t)) == 0 ||
			    filename[0] == '\0')
				return COMMAND_ERROR_USAGE;

			desc = lookup_image_desc_from_uuid(fip, &uuid);
			if (desc == NULL) {
				snprintf(name, sizeof(name), "%pU", &uuid);
				desc = new_image_desc(&uuid, name, "blob");
				add_image_desc(fip, desc);
			}
			set_image_desc_action(desc, DO_PACK, filename);
			break;
		}
		case 'a':
			align = get_image_align(optarg);
			if (align < 0)
				return COMMAND_ERROR;
			break;
		case 'o':
			snprintf(outfile, sizeof(outfile), "%s", optarg);
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		return COMMAND_ERROR_USAGE;

	if (outfile[0] == '\0')
		snprintf(outfile, sizeof(outfile), "%s", argv[0]);

	if (file_exists(argv[0])) {
		ret = parse_fip(fip, argv[0], &toc_header);
		if (ret)
			return ret;
	}

	if (pflag)
		toc_header.flags &= ~(0xffffULL << 32);
	toc_flags = (toc_header.flags |= toc_flags);

	if (update_fip(fip))
		return COMMAND_ERROR;

	return pack_images(fip, outfile, toc_flags, align);
}

static int unpack_cmd(struct fip_state *fip, int argc, char *argv[])
{
	char outdir[PATH_MAX] = { 0 };
	struct fip_image_desc *desc;
	int fflag = 0;
	int unpack_all = 1;
	int ret, opt;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	while ((opt = getopt(argc, argv, "e:b:fo:")) > 0) {
		switch (opt) {
		case 'e': {
			struct fip_image_desc *desc;

			desc = lookup_image_desc_from_opt(fip, &optarg);
			if (!desc)
				return COMMAND_ERROR;
			set_image_desc_action(desc, DO_UNPACK, optarg);
			unpack_all = 0;
			break;
		}
		case 'b': {
			char name[UUID_STRING_LEN + 1];
			char filename[PATH_MAX] = { 0 };
			uuid_t uuid = uuid_null;
			struct fip_image_desc *desc;

			parse_blob_opt(optarg, &uuid,
			    filename, sizeof(filename));

			if (memcmp(&uuid, &uuid_null, sizeof(uuid_t)) == 0 ||
			    filename[0] == '\0')
				return COMMAND_ERROR_USAGE;

			desc = lookup_image_desc_from_uuid(fip, &uuid);
			if (desc == NULL) {
				snprintf(name, sizeof(name), "%pU", &uuid);
				desc = new_image_desc(&uuid, name, "blob");
				add_image_desc(fip, desc);
			}
			set_image_desc_action(desc, DO_UNPACK, filename);
			unpack_all = 0;
			break;
		}
		case 'f':
			fflag = 1;
			break;
		case 'o':
			snprintf(outdir, sizeof(outdir), "%s", optarg);
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		return COMMAND_ERROR_USAGE;

	ret = parse_fip(fip, argv[0], NULL);
	if (ret)
		return ret;

	if (outdir[0] != '\0')
		if (chdir(outdir) == -1) {
			pr_err("chdir %s: %m\n", outdir);
			return COMMAND_ERROR;
		}

	/* Unpack all specified images. */
	fip_for_each_desc(fip, desc) {
		char file[PATH_MAX];
		struct fip_image *image = desc->image;

		if (!unpack_all && desc->action != DO_UNPACK)
			continue;

		/* Build filename. */
		if (desc->action_arg == NULL)
			snprintf(file, sizeof(file), "%s.bin",
			    desc->cmdline_name);
		else
			snprintf(file, sizeof(file), "%s",
			    desc->action_arg);

		if (image == NULL) {
			if (!unpack_all)
				pr_warn("%s does not exist in %s\n", file, argv[0]);
			continue;
		}

		if (!file_exists(file) || fflag) {
			pr_verbose("Unpacking %s\n", file);
			ret = write_image_to_file(image, file);
			if (ret)
				return ret;
		} else {
			pr_warn("File %s already exists, use -f to overwrite it\n", file);
		}
	}

	return 0;
}

static __maybe_unused int remove_cmd(struct fip_state *fip, int argc, char *argv[])
{
	char outfile[PATH_MAX] = { 0 };
	fip_toc_header_t toc_header;
	struct fip_image_desc *desc;
	long align = 1;
	int ret, opt, fflag = 0;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	while ((opt = getopt(argc, argv, "e:a:b:fo:")) > 0) {
		switch (opt) {
		case 'e': {
			struct fip_image_desc *desc;

			desc = lookup_image_desc_from_opt(fip, &optarg);
			if (!desc)
				return COMMAND_ERROR;
			set_image_desc_action(desc, DO_REMOVE, NULL);
			break;
		}
		case 'a':
			align = get_image_align(optarg);
			if (align < 0)
				return COMMAND_ERROR;
			break;
		case 'b': {
			char name[UUID_STRING_LEN + 1], filename[PATH_MAX];
			uuid_t uuid = uuid_null;
			struct fip_image_desc *desc;

			parse_blob_opt(optarg, &uuid,
			    filename, sizeof(filename));

			if (memcmp(&uuid, &uuid_null, sizeof(uuid_t)) == 0)
				return COMMAND_ERROR_USAGE;

			desc = lookup_image_desc_from_uuid(fip, &uuid);
			if (desc == NULL) {
				snprintf(name, sizeof(name), "%pU", &uuid);
				desc = new_image_desc(&uuid, name, "blob");
				add_image_desc(fip, desc);
			}
			set_image_desc_action(desc, DO_REMOVE, NULL);
			break;
		}
		case 'f':
			fflag = 1;
			break;
		case 'o':
			snprintf(outfile, sizeof(outfile), "%s", optarg);
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		return COMMAND_ERROR_USAGE;

	if (outfile[0] != '\0' && file_exists(outfile) && !fflag)
		pr_err("File %s already exists, use -f to overwrite it\n",
		    outfile);

	if (outfile[0] == '\0')
		snprintf(outfile, sizeof(outfile), "%s", argv[0]);

	ret = parse_fip(fip, argv[0], &toc_header);
	if (ret)
		return ret;

	fip_for_each_desc(fip, desc) {
		if (desc->action != DO_REMOVE)
			continue;

		if (desc->image != NULL) {
			pr_verbose("Removing %s\n", desc->cmdline_name);
			free(desc->image);
			desc->image = NULL;
		} else {
			pr_warn("%s does not exist in %s\n", desc->cmdline_name, argv[0]);
		}
	}

	return pack_images(fip, outfile, toc_header.flags, align);
}

/* Available subcommands. */
static cmd_t cmds[] = {
	{ .name = "info",    .handler = info_cmd,    },
	{ .name = "uuid",    .handler = uuid_cmd,    },
	{ .name = "unpack",  .handler = unpack_cmd,  },
#ifdef CONFIG_CMD_FIPTOOL_WRITE
	{ .name = "create",  .handler = create_cmd,  },
	{ .name = "update",  .handler = update_cmd,  },
	{ .name = "remove",  .handler = remove_cmd,  },
#endif
};

static int do_fiptool(int argc, char *argv[])
{
	int i, opt, ret = 0;
	struct fip_state *fip = fip_new();

	/*
	 * Set POSIX mode so getopt stops at the first non-option
	 * which is the subcommand.
	 */
	while ((opt = getopt(argc, argv, "+v")) > 0) {
		switch (opt) {
		case 'v':
			fip->verbose = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		return COMMAND_ERROR_USAGE;

	fill_image_descs(fip);
	for (i = 0; i < ARRAY_SIZE(cmds); i++) {
		if (strcmp(cmds[i].name, argv[0]) == 0) {
			struct getopt_context gc;

			getopt_context_store(&gc);

			ret = cmds[i].handler(fip, argc, argv);

			getopt_context_restore(&gc);
			break;
		}
	}

	if (i == ARRAY_SIZE(cmds))
		return COMMAND_ERROR_USAGE;
	fip_free(fip);
	return ret;
}

BAREBOX_CMD_HELP_START(fiptool)
BAREBOX_CMD_HELP_TEXT("List information about the specified files or directories")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Global options:")
BAREBOX_CMD_HELP_OPT ("-v",  "verbose output")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Commands:")
BAREBOX_CMD_HELP_OPT("info",   "List images contained in FIP")
BAREBOX_CMD_HELP_OPT("uuid",   "List possible FIP image types and their UUIDs")
BAREBOX_CMD_HELP_OPT("unpack", "Unpack images from FIP")
#ifdef CONFIG_CMD_FIPTOOL_WRITE
BAREBOX_CMD_HELP_OPT("create", "Create a new FIP with the given images")
BAREBOX_CMD_HELP_OPT("update", "Update an existing FIP with the given images")
BAREBOX_CMD_HELP_OPT("remove", "Remove images from FIP")
#endif
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("fiptool unpack [OPTS] FIP_FILENAME")
BAREBOX_CMD_HELP_OPT("-a VALUE", "Each image is aligned to VALUE (default: 1)")
BAREBOX_CMD_HELP_OPT("-b uuid=UUID,file=FILE", "Unpack an image with the given UUID into FILE")
BAREBOX_CMD_HELP_OPT("-f", "force overwrite of the output FIP file if it already exists")
BAREBOX_CMD_HELP_OPT("-o", "Set an alternative output FIP file")
BAREBOX_CMD_HELP_OPT("-e TYPE[=FILE]", "unpack only TYPE entry")
#ifdef CONFIG_CMD_FIPTOOL_WRITE
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("fiptool create [OPTS] FIP_FILENAME")
BAREBOX_CMD_HELP_OPT("-a VALUE", "Each image is aligned to VALUE (default: 1)")
BAREBOX_CMD_HELP_OPT("-b uuid=UUID,file=FILE", "Add an image with the given UUID pointed to by FILE")
BAREBOX_CMD_HELP_OPT("-p", "16-bit platform specific flag field occupying bits 32-47 in 64-bit ToC header")
BAREBOX_CMD_HELP_OPT("-e TYPE[=FILE]", "add TYPE entry from FILE")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("fiptool update [OPTS] FIP_FILENAME")
BAREBOX_CMD_HELP_OPT("-a VALUE", "Each image is aligned to VALUE (default: 1)")
BAREBOX_CMD_HELP_OPT("-b uuid=UUID,file=FILE", "Add an image with the given UUID pointed to by FILE")
BAREBOX_CMD_HELP_OPT("-o", "Set an alternative output FIP file")
BAREBOX_CMD_HELP_OPT("-p", "16-bit platform specific flag field occupying bits 32-47 in 64-bit ToC header")
BAREBOX_CMD_HELP_OPT("-e TYPE[=FILE]", "update TYPE entry with FILE")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("fiptool remove [OPTS] FIP_FILENAME")
BAREBOX_CMD_HELP_OPT("-a VALUE", "Each image is aligned to VALUE (default: 1)")
BAREBOX_CMD_HELP_OPT("-b uuid=UUID", "Remove an image with the given UUID")
BAREBOX_CMD_HELP_OPT("-f", "force overwrit of the output FIP file if it already exists")
BAREBOX_CMD_HELP_OPT("-o", "Set an alternative output FIP file")
BAREBOX_CMD_HELP_OPT("-e TYPE", "remove TYPE entry")
#endif
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(fiptool)
	.cmd		= do_fiptool,
	BAREBOX_CMD_DESC("inspect and manipulate TF-A firmware image packages")
	BAREBOX_CMD_OPTS("[-v] COMMAND [ARGS...]")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_fiptool_help)
BAREBOX_CMD_END
