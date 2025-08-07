// SPDX-License-Identifier: GPL-2.0-only

#include <command.h>
#include <getopt.h>
#include <libfile.h>
#include <fs.h>
#include <xfuncs.h>
#include <linux/sizes.h>
#include <tee/avb.h>
#include <linux/ctype.h>
#include <environment.h>

static int do_avb_pvalue(int argc, char *argv[])
{
	size_t out = 0x13;
	void *buf = NULL;
	int ret = 0, rc = COMMAND_ERROR;
	int opt;
	const char *write_varname = NULL, *read_varname = NULL;
	void *val = NULL;
	size_t size;
	const char *filename = NULL;
	const char *varname = NULL;
	const int bufsize = SZ_8K;

	while ((opt = getopt(argc, argv, "r:w:f:")) > 0) {
		switch(opt) {
		case 'r':
			read_varname = optarg;
			break;
		case 'w':
			write_varname = optarg;
			break;
		case 'f':
			filename = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argc -= optind;
	argv += optind;

	if (!read_varname && !write_varname)
		return COMMAND_ERROR_USAGE;

	if (read_varname && write_varname)
		return COMMAND_ERROR_USAGE;

	if (write_varname) {
		if (argc >= 1)
			val = argv[0];

		if (filename) {
			ret = read_file_2(filename, &size, &buf, FILESIZE_MAX);
			if (ret) {
				printf("Cannot read %s: %pe\n", filename, ERR_PTR(ret));
				return COMMAND_ERROR;
			}
		} else {
			if (!val)
				return COMMAND_ERROR_USAGE;
			size = strlen(val) + 1;
			buf = xstrdup(val);

		}
		ret = avb_write_persistent_value(write_varname, size, buf);
		if (ret) {
			printf("Cannot write variable %s: %pe\n", write_varname, ERR_PTR(ret));
			goto err;
		}
	}

	if (read_varname) {
		if (argc >= 1)
			varname = argv[0];

		buf = xzalloc(bufsize);

		ret = avb_read_persistent_value(read_varname, SZ_8K, buf, &out);
		if (ret) {
			printf("Cannot read variable %s: %pe\n", read_varname, ERR_PTR(ret));
			rc = COMMAND_ERROR;
			goto err;
		}

		if (filename) {
			ret = write_file(filename, buf, out);
			if (ret) {
				printf("Cannot write %s: %pe\n", filename, ERR_PTR(ret));
				rc = COMMAND_ERROR;
				goto err;
			}
		} else if (varname) {
			unsigned char *str = buf;
			int i;

			str[bufsize - 1] = 0;
	
			for (i = 0; i < out; i++)
				if (!isprint(str[i]))
					break;

			str[i] = 0;

			setenv(varname, buf);
		} else {
			memory_display(buf, 0, out, 1, 0);
		}
	}

	rc = 0;

err:
	free(buf);

	return rc;
}

BAREBOX_CMD_HELP_START(avb_pvalue)
BAREBOX_CMD_HELP_TEXT("This command provides access to the AVB persistent variable store.")
BAREBOX_CMD_HELP_TEXT("usage:")
BAREBOX_CMD_HELP_TEXT("avb_pvalue [OPTION]... [VARNAME/VALUE]")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-r <varname>",  "read variable <varname>. read value into [VARNAME] if given")
BAREBOX_CMD_HELP_OPT ("-w <varname>",  "write variable <varname>. Set to [VALUE] if given")
BAREBOX_CMD_HELP_OPT ("-f <file>",  "read/write value from/to file")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(avb_pvalue)
	.cmd            = do_avb_pvalue,
	BAREBOX_CMD_DESC("AVB persistent variable store")
	BAREBOX_CMD_OPTS("[-rwf] [VARNAME/VALUE]")
	BAREBOX_CMD_HELP(cmd_avb_pvalue_help)
	BAREBOX_CMD_GROUP(CMD_GRP_SECURITY)
BAREBOX_CMD_END
