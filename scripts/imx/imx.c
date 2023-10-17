// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2016 Sascha Hauer, Pengutronix

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>
#include <linux/kernel.h>

#include "imx.h"

#define MAXARGS 32

/*
 * First word of bootloader image should be authenticated,
 * encrypt the rest.
 */
#define ENCRYPT_OFFSET	(HEADER_LEN + 0x10)

static int parse_line(char *line, char *argv[])
{
	int nargs = 0;

	while (nargs < MAXARGS) {

		/* skip any white space */
		while ((*line == ' ') || (*line == '\t'))
			++line;

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			return nargs;
		}

		argv[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && (*line != ' ') && (*line != '\t'))
			++line;

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			return nargs;
		}

		*line++ = '\0';		/* terminate current arg	 */
	}

	printf("** Too many args (max. %d) **\n", MAXARGS);

	return nargs;
}

struct command {
	const char *name;
	int (*parse)(struct config_data *data, int argc, char *argv[]);
};

static const char *check_cmds[] = {
	"until_all_bits_clear",	/* until ((*address & mask) == 0) { }; */
	"until_any_bit_clear",	/* until ((*address & mask) != mask) { }; */
	"until_all_bits_set",	/* until ((*address & mask) == mask) { }; */
	"until_any_bit_set",	/* until ((*address & mask) != 0) { }; */
};

static void do_cmd_check_usage(void)
{
	fprintf(stderr,
			"usage: check <width> <cmd> <addr> <mask>\n"
			"<width> access width in bytes [1|2|4]\n"
			"with <cmd> one of:\n"
			"until_all_bits_clear: while ((*addr & mask) == 0)\n"
			"until_all_bits_set:   while ((*addr & mask) == mask)\n"
			"until_any_bit_clear:  while ((*addr & mask) != mask)\n"
			"until_any_bit_set:    while ((*addr & mask) != 0)\n");
}

static int do_cmd_check(struct config_data *data, int argc, char *argv[])
{
	uint32_t addr, mask, cmd;
	int i, width;
	const char *scmd;

	if (argc < 5) {
		do_cmd_check_usage();
		return -EINVAL;
	}

	if (!data->check)
		return -ENOSYS;

	width = strtoul(argv[1], NULL, 0) >> 3;
	scmd = argv[2];
	addr = strtoul(argv[3], NULL, 0);
	mask = strtoul(argv[4], NULL, 0);

	switch (width) {
	case 1:
	case 2:
	case 4:
		break;
	default:
		fprintf(stderr, "illegal width %d\n", width);
		return -EINVAL;
	};

	for (i = 0; i < ARRAY_SIZE(check_cmds); i++) {
		if (!strcmp(scmd, check_cmds[i]))
			break;
	}

	if (i == ARRAY_SIZE(check_cmds)) {
		do_cmd_check_usage();
		return -EINVAL;
	}

	cmd = (TAG_CHECK << 24) | (i << 3) | width | ((sizeof(uint32_t) * 3) << 8);

	return data->check(data, cmd, addr, mask);
}

static int do_cmd_nop(struct config_data *data, int argc, char *argv[])
{
	if (!data->nop)
		return -ENOSYS;

	return data->nop(data);
}

static int write_mem(struct config_data *data, int argc, char *argv[],
		     int set_bits, int clear_bits)
{
	uint32_t addr, val, width;
	char *end;

	if (argc != 4) {
		fprintf(stderr, "usage: wm [8|16|32] <addr> <val>\n");
		return -EINVAL;
	}

	width = strtoul(argv[1], &end, 0);
	if (*end != '\0') {
		fprintf(stderr, "illegal width token \"%s\"\n", argv[1]);
		return -EINVAL;
	}

	addr = strtoul(argv[2], &end, 0);
	if (*end != '\0') {
		fprintf(stderr, "illegal address token \"%s\"\n", argv[2]);
		return -EINVAL;
	}

	val = strtoul(argv[3], &end, 0);
	if (*end != '\0') {
		fprintf(stderr, "illegal value token \"%s\"\n", argv[3]);
		return -EINVAL;
	}

	width >>= 3;

	switch (width) {
	case 1:
	case 2:
	case 4:
		break;
	default:
		fprintf(stderr, "illegal width %d\n", width);
		return -EINVAL;
	};

	return data->write_mem(data, addr, val, width, set_bits, clear_bits);
}

static int do_cmd_write_mem(struct config_data *data, int argc, char *argv[])
{
	return write_mem(data, argc, argv, 0, 0);
}

static int do_cmd_set_bits(struct config_data *data, int argc, char *argv[])
{
	return write_mem(data, argc, argv, 1, 0);
}

static int do_cmd_clear_bits(struct config_data *data, int argc, char *argv[])
{
	return write_mem(data, argc, argv, 0, 1);
}

static int do_loadaddr(struct config_data *data, int argc, char *argv[])
{
	if (argc < 2)
		return -EINVAL;

	data->image_load_addr = strtoul(argv[1], NULL, 0);

	return 0;
}

static int do_ivt_offset(struct config_data *data, int argc, char *argv[])
{
	if (argc < 2)
		return -EINVAL;

	data->image_ivt_offset = strtoul(argv[1], NULL, 0);

	return 0;
}

static int do_dcdofs_error(struct config_data *data, int argc, char *argv[])
{
	fprintf(stderr, "ERROR: misnomer dcdofs has been renamed to ivtofs. imxcfg must be adapted.\n");
	return -EINVAL;
}

struct soc_type {
	char *name;
	int header_version;
	int cpu_type;
	off_t header_gap;
	uint32_t first_opcode;
};

#define SZ_32K	(32 * 1024)

static struct soc_type socs[] = {
	{ .name = "imx25",  .header_version = 1, .cpu_type = IMX_CPU_IMX25,  .header_gap = 0,      .first_opcode = 0xea0003fe /* b 0x1000 */},
	{ .name = "imx35",  .header_version = 1, .cpu_type = IMX_CPU_IMX35,  .header_gap = 0,      .first_opcode = 0xea0003fe /* b 0x1000 */},
	{ .name = "imx50",  .header_version = 2, .cpu_type = IMX_CPU_IMX50,  .header_gap = 0,      .first_opcode = 0xea0003fe /* b 0x1000 */},
	{ .name = "imx51",  .header_version = 1, .cpu_type = IMX_CPU_IMX51,  .header_gap = 0,      .first_opcode = 0xea0003fe /* b 0x1000 */},
	{ .name = "imx53",  .header_version = 2, .cpu_type = IMX_CPU_IMX53,  .header_gap = 0,      .first_opcode = 0xea0003fe /* b 0x1000 */},
	{ .name = "imx6",   .header_version = 2, .cpu_type = IMX_CPU_IMX6,   .header_gap = 0,      .first_opcode = 0xea0003fe /* b 0x1000 */},
	{ .name = "imx7",   .header_version = 2, .cpu_type = IMX_CPU_IMX7,   .header_gap = 0,      .first_opcode = 0xea0003fe /* b 0x1000 */},
	{ .name = "imx8mm", .header_version = 2, .cpu_type = IMX_CPU_IMX8MM, .header_gap = SZ_32K, .first_opcode = 0x14000000 /* b 0x0000 (offset computed) */},
	{ .name = "imx8mn", .header_version = 2, .cpu_type = IMX_CPU_IMX8MN, .header_gap = SZ_32K, .first_opcode = 0x14000000 /* b 0x0000 (offset computed) */},
	{ .name = "imx8mp", .header_version = 2, .cpu_type = IMX_CPU_IMX8MP, .header_gap = SZ_32K, .first_opcode = 0x14000000 /* b 0x0000 (offset computed) */},
	{ .name = "imx8mq", .header_version = 2, .cpu_type = IMX_CPU_IMX8MQ, .header_gap = SZ_32K, .first_opcode = 0x14000000 /* b 0x0000 (offset computed) */},
	{ .name = "vf610",  .header_version = 2, .cpu_type = IMX_CPU_VF610,  .header_gap = 0,      .first_opcode = 0xea0003fe /* b 0x1000 */},
};

static int do_soc(struct config_data *data, int argc, char *argv[])
{
	char *soc;
	int i;

	if (argc < 2)
		return -EINVAL;

	soc = argv[1];

	for (i = 0; i < ARRAY_SIZE(socs); i++) {
		if (!strcmp(socs[i].name, soc)) {
			data->header_version = socs[i].header_version;
			data->cpu_type = socs[i].cpu_type;
			data->header_gap = socs[i].header_gap;
			data->first_opcode = socs[i].first_opcode;

			if (data->cpu_type == IMX_CPU_IMX35)
				data->load_size += HEADER_LEN;

			return 0;
		}
	}

	fprintf(stderr, "unknown SoC type \"%s\". Known SoCs are:\n", soc);
	for (i = 0; i < ARRAY_SIZE(socs); i++)
		fprintf(stderr, "%s ", socs[i].name);
	fprintf(stderr, "\n");

	return -EINVAL;
}

static int do_max_load_size(struct config_data *data, int argc, char *argv[])
{
	if (argc < 2)
		return -EINVAL;

	data->max_load_size = strtoul(argv[1], NULL, 0);

	return 0;
}

static int hab_add_str(struct config_data *data, const char *str)
{
	int len = strlen(str);

	if (data->csf_space < len)
		return -ENOMEM;

	strcat(data->csf, str);

	data->csf_space -= len;

	return 0;
}

static int do_hab(struct config_data *data, int argc, char *argv[])
{
	int i, ret;

	if (!data->csf) {
		data->csf_space = 0x10000;

		data->csf = calloc(data->csf_space + 1, 1);
		if (!data->csf)
			return -ENOMEM;
	}

	for (i = 1; i < argc; i++) {
		ret = hab_add_str(data, argv[i]);
		if (ret)
			return ret;

		ret = hab_add_str(data, " ");
		if (ret)
			return ret;
	}

	ret = hab_add_str(data, "\n");
	if (ret)
		return ret;

	return 0;
}

static void
imx8m_get_offset_size(struct config_data *data,
		      uint32_t *offset, uint32_t *signed_size)
{
	unsigned int hdrlen = HEADER_LEN;

	if (flexspi_image(data))
		hdrlen += FLEXSPI_HEADER_LEN;

	*signed_size = roundup(data->pbl_code_size + hdrlen, 0x1000);

	*offset += data->header_gap;
	if (data->signed_hdmi_firmware_file)
		*offset += PLUGIN_HDMI_SIZE;
}

static int do_hab_blocks(struct config_data *data, int argc, char *argv[])
{
	char *str;
	int ret;
	int i;
	uint32_t signed_size = data->load_size;
	uint32_t offset = data->image_ivt_offset;

	if (!data->csf)
		return -EINVAL;

	/*
	 * In case of encrypted image we reduce signed area to beginning
	 * of encrypted area.
	 */
	if (data->encrypt_image)
		signed_size = ENCRYPT_OFFSET;

	/*
	 * Ensure we only sign the PBL for i.MX8MQ
	 */
	if (data->pbl_code_size && cpu_is_mx8m(data))
		imx8m_get_offset_size(data, &offset, &signed_size);

	if (signed_size > 0) {
		ret = asprintf(&str, "Blocks = 0x%08x 0x%08x 0x%08x \"%s\"",
			data->image_load_addr + data->image_ivt_offset, offset,
			signed_size - data->image_ivt_offset, data->outfile);
	} else {
		fprintf(stderr, "Invalid signed size area 0x%08x\n",
			signed_size);
		return -EINVAL;
	}

	if (ret < 0)
		return -ENOMEM;

	ret = hab_add_str(data, str);
	free(str);
	if (ret)
		return ret;

	for (i = 1; i < argc; i++) {
		uint32_t addr;
		uint32_t off;
		uint32_t size;
		char *b;
		char *e;

		b = argv[i];
		if (*b == '"') // remove leading qoute
			b++;
		if (!*b || *b == '"')
			continue; // skip if empty

		off = strtoul(b, &e, 0);
		if (*e != '+') {
			fprintf(stderr, "failed to find '+' in '%s'\n", b);
			fprintf(stderr, "format off+size@addr expected, but given: %s\n", argv[i]);
			return -EINVAL;
		}

		b = e + 1;
		size = strtoul(b, &e, 0);
		if (*e != '@') {
			fprintf(stderr, "failed to find '@' in '%s'\n", b);
			fprintf(stderr, "format off+size@addr expected, but given: %s\n", argv[i]);
			return -EINVAL;
		}

		b = e + 1;
		addr = strtoul(b, &e, 0);
		if (*e && *e != '"') { // ignore trailing qoute
			fprintf(stderr, "unexpected char at the end: '%c'\n", *e);
			fprintf(stderr, "format off+size@addr expected, but given: %s\n", argv[i]);
			return -EINVAL;
		}

		ret = asprintf(&str, ", 0x%08x 0x%08x 0x%08x \"%s\"", addr, off, size, data->outfile);
		if (ret < 0)
			return -ENOMEM;

		ret = hab_add_str(data, str);
		free(str);
		if (ret)
			return ret;
	}

	return hab_add_str(data, "\n");
}

static int do_hab_encrypt(struct config_data *data, int argc, char *argv[])
{
	if (!data->encrypt_image)
		return 0;

	return do_hab(data, argc, argv);
}

static int do_hab_encrypt_key(struct config_data *data, int argc, char *argv[])
{
	char *str;
	char *dekfile;
	int ret;

	if (!data->csf)
		return -EINVAL;

	if (!data->encrypt_image)
		return 0;

	ret = asprintf(&dekfile, "%s.dek", data->outfile);
	if (ret < 0)
		return -ENOMEM;

	ret = asprintf(&str, "Key = \"%s\"\n", dekfile);
	if (ret < 0)
		return -ENOMEM;

	ret = hab_add_str(data, str);
	if (ret)
		return ret;

	return 0;
}

static int do_hab_encrypt_key_length(struct config_data *data, int argc,
				     char *argv[])
{
	unsigned int dek_bits;
	char *str;
	int ret;

	if (!data->csf)
		return -EINVAL;

	if (!data->encrypt_image)
		return 0;

	if (argc < 2)
		return -EINVAL;

	dek_bits = strtoul(argv[1], NULL, 0);

	if (dek_bits != 128 && dek_bits != 192 && dek_bits != 256) {
		fprintf(stderr, "wrong dek size (%u)\n", dek_bits);
		return -EINVAL;
	}

	data->dek_size = dek_bits / 8;

	ret = asprintf(&str, "Key Length = %u\n", dek_bits);
	if (ret < 0)
		return -ENOMEM;

	ret = hab_add_str(data, str);
	if (ret)
		return ret;

	return 0;
}

static int do_hab_encrypt_blob_address(struct config_data *data, int argc,
				       char *argv[])
{
	char *str;
	int ret;

	if (!data->csf)
		return -EINVAL;

	if (!data->encrypt_image)
		return 0;

	ret = asprintf(&str,
		       "Blob address = 0x%08zx\n",
		       data->image_load_addr + data->load_size + CSF_LEN -
				(DEK_BLOB_OVERHEAD + data->dek_size));
	if (ret < 0)
		return -ENOMEM;

	ret = hab_add_str(data, str);
	if (ret)
		return ret;

	return 0;
}

static int do_hab_encrypt_blocks(struct config_data *data, int argc,
				 char *argv[])
{
	char *str;
	int ret;

	if (!data->csf)
		return -EINVAL;

	if (!data->encrypt_image)
		return 0;

	ret = asprintf(&str, "Blocks = 0x%08x 0x%x %d \"%s\"\n",
		       data->image_load_addr + ENCRYPT_OFFSET, ENCRYPT_OFFSET,
		       data->load_size - ENCRYPT_OFFSET, data->outfile);
	if (ret < 0)
		return -ENOMEM;

	ret = hab_add_str(data, str);
	if (ret)
		return ret;

	return 0;
}

static int do_super_root_key(struct config_data *data, int argc, char *argv[])
{
	int len;
	char *srkfile;

	if (argc != 2) {
		fprintf(stderr, "usage: super_root_key <keyfile>\n");
		return -EINVAL;
	}

	if (data->header_version != 1) {
		fprintf(stderr, "Warning: The super_root_key command is meaningless "
			"on non HABv3 based SoCs\n");
		return 0;
	}

	srkfile = argv[1];

	if (*srkfile == '"')
		srkfile++;

	data->srkfile = strdup(srkfile);
	if (!data->srkfile)
		return -ENOMEM;

	len = strlen(data->srkfile);
	if (data->srkfile[len - 1] == '"')
		data->srkfile[len - 1] = 0;

	return 0;
}

static int
do_signed_hdmi_firmware(struct config_data *data, int argc, char *argv[])
{
	const char *file;
	int len;


	if (argc != 2) {
		fprintf(stderr, "usage: signed_hdmi_firmware <file>\n");
		return -EINVAL;
	}

	if (data->cpu_type != IMX_CPU_IMX8MQ) {
		fprintf(stderr,
			"Warning: The signed_hdmi_firmware command is "
			"only supported i.MX8MQ SoCs\n");
		return 0;
	}

	file = argv[1];

	if (*file == '"')
		file++;

	data->signed_hdmi_firmware_file = strdup(file);
	if (!data->signed_hdmi_firmware_file)
		return -ENOMEM;

	len = strlen(data->signed_hdmi_firmware_file);
	if (data->signed_hdmi_firmware_file[len - 1] == '"')
		data->signed_hdmi_firmware_file[len - 1] = 0;

	return 0;
}

static int do_flexspi_ivtofs(struct config_data *data, int argc, char *argv[])
{
	if (argc < 2)
		return -EINVAL;

	if (data->csf) {
		fprintf(stderr, "#include <mach/imx/flexspi-imx8m*-cfg.h> must be placed in front "
				"of #include <mach/imx/habv4-imx8-gencsf.h>\n");
		return -EINVAL;
	}

	data->image_flexspi_ivt_offset = strtoul(argv[1], NULL, 0);

	return 0;
}

static int do_flexspi_fcfbofs(struct config_data *data, int argc, char *argv[])
{
	if (argc < 2)
		return -EINVAL;

	if (data->csf) {
		fprintf(stderr, "#include <mach/imx/flexspi-imx8m*-cfg.h> must be placed in front "
				"of #include <mach/imx/habv4-imx8-gencsf.h>\n");
		return -EINVAL;
	}

	data->image_flexspi_fcfb_offset = strtoul(argv[1], NULL, 0);

	return 0;
}

struct command cmds[] = {
	{
		.name = "wm",
		.parse = do_cmd_write_mem,
	}, {
		.name = "set_bits",
		.parse = do_cmd_set_bits,
	}, {
		.name = "clear_bits",
		.parse = do_cmd_clear_bits,
	}, {
		.name = "check",
		.parse = do_cmd_check,
	}, {
		.name = "nop",
		.parse = do_cmd_nop,
	}, {
		.name = "loadaddr",
		.parse = do_loadaddr,
	}, {
		.name = "ivtofs",
		.parse = do_ivt_offset,
	}, {
		.name = "dcdofs",
		.parse = do_dcdofs_error,
	}, {
		.name = "soc",
		.parse = do_soc,
	},  {
		.name = "max_load_size",
		.parse = do_max_load_size,
	}, {
		.name = "hab",
		.parse = do_hab,
	}, {
		.name = "hab_blocks",
		.parse = do_hab_blocks,
	}, {
		.name = "hab_encrypt",
		.parse = do_hab_encrypt,
	}, {
		.name = "hab_encrypt_key",
		.parse = do_hab_encrypt_key,
	}, {
		.name = "hab_encrypt_key_length",
		.parse = do_hab_encrypt_key_length,
	}, {
		.name = "hab_encrypt_blob_address",
		.parse = do_hab_encrypt_blob_address,
	}, {
		.name = "hab_encrypt_blocks",
		.parse = do_hab_encrypt_blocks,
	}, {
		.name = "super_root_key",
		.parse = do_super_root_key,
	}, {
		.name = "signed_hdmi_firmware",
		.parse = do_signed_hdmi_firmware,
	}, {
		.name = "flexspi_fcfbofs",
		.parse = do_flexspi_fcfbofs,
	}, {
		.name = "flexspi_ivtofs",
		.parse = do_flexspi_ivtofs,
	},
};

static char *readcmd(struct config_data *data, FILE *f)
{
	static char *buf;
	char *str;
	ssize_t ret;
	int inquotes = 0;

	if (!buf) {
		buf = malloc(4096);
		if (!buf)
			return NULL;
	}

	str = buf;
	*str = 0;

	while (1) {
		ret = fread(str, 1, 1, f);
		if (!ret)
			return strlen(buf) ? buf : NULL;
		if (*str == '"') {
			inquotes = !inquotes;
		} else if ((*str == '\n' || *str == ';') && !inquotes) {
			*str = 0;
			return buf;
		}

		str++;
	}
}

int parse_config(struct config_data *data, const char *_filename)
{
	FILE *f;
	int lineno = 0;
	char *line = NULL, *tmp;
	char *argv[MAXARGS];
	int nargs, i, ret = 0;
	char *filename;

	filename = strdup(_filename);

	f = fopen(filename, "r");
	if (!f) {
		fprintf(stderr, "Error: %s - Can't open DCD file\n", filename);
		exit(1);
	}

	while (1) {
		line = readcmd(data, f);
		if (!line)
			break;

		lineno++;

		tmp = strchr(line, '#');
		if (tmp) {
			char *endptr;
			long linenum = strtol(tmp + 1, &endptr, 10);
			if (strncmp(endptr, " \"", 2) == 0 && endptr[2]) {
				free(filename);
				lineno = linenum - 1;
				filename = strdup(endptr + 2);
				filename[strlen(filename) - 1] = '\0';
			}
			*tmp = '\0';
		}

		nargs = parse_line(line, argv);
		if (!nargs)
			continue;

		ret = -ENOENT;

		for (i = 0; i < ARRAY_SIZE(cmds); i++) {
			if (!strcmp(cmds[i].name, argv[0])) {
				ret = cmds[i].parse(data, nargs, argv);
				if (ret) {
					fprintf(stderr, "%s:%d: %s\n",
						filename, lineno, strerror(-ret));
					goto cleanup;
				}
				break;
			}
		}

		if (ret == -ENOENT) {
			fprintf(stderr, "no such command: %s\n", argv[0]);
			goto cleanup;
		}
	}

cleanup:
	fclose(f);
	free(filename);
	return ret;
}
