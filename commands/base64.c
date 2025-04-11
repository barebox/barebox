// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <xfuncs.h>
#include <malloc.h>
#include <libfile.h>
#include <getopt.h>
#include <base64.h>

static int do_base64(int argc, char *argv[])
{
	char *input, *output = NULL;
	bool url = false, decode = false;
	int opt, ret;
	size_t bufsz, outbufsz;
	void *inbuf = NULL, *outbuf = NULL;

	while ((opt = getopt(argc, argv, "duo:")) > 0) {
		switch(opt) {
		case 'd':
			decode = true;
			break;
		case 'u':
			url = true;
			break;
		case 'o':
			output = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1 || !output)
		return COMMAND_ERROR_USAGE;

	input = argv[0];

	inbuf = read_file(input, &bufsz);
	if (!inbuf) {
		ret = -errno;
		goto out;
	}

	outbuf = malloc(bufsz);
	if (!outbuf) {
		ret = -errno;
		goto out;
	}

	if (!decode) {
		printf("encoding currently not supported\n");
		ret = -ENOSYS;
		goto out;
	}

	if (url)
		outbufsz = decode_base64url(outbuf, bufsz, inbuf);
	else
		outbufsz = decode_base64(outbuf, bufsz, inbuf);

	ret = write_file(output, outbuf, outbufsz);
out:
	free(inbuf);
	free(outbuf);

	return ret;
}

BAREBOX_CMD_HELP_START(base64)
BAREBOX_CMD_HELP_TEXT("decode normal and URL base64 data")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-d",  "decode to base64 instead of encoding")
BAREBOX_CMD_HELP_OPT ("-o OUT",  "output file name")
BAREBOX_CMD_HELP_OPT ("-u",  "Use base64 URL character set")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(base64)
	.cmd		= do_base64,
	BAREBOX_CMD_DESC("base64 decode data")
	BAREBOX_CMD_OPTS("[-u] -d -o OUT IN")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_base64_help)
BAREBOX_CMD_END
