/*
 * digest.c - Calculate a md5/sha1/sha256 checksum of a memory area
 *
 * Copyright (c) 2011 Peter Korsgaard <jacmet@sunsite.dk>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <xfuncs.h>
#include <malloc.h>
#include <digest.h>

static int file_digest(struct digest *d, char *filename,
		       ulong start, ulong size)
{
	ulong len = 0;
	int fd, now, i, ret = 0;
	unsigned char *buf;

	d->init(d);

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror(filename);
		return fd;
	}

	if (start > 0) {
		ret = lseek(fd, start, SEEK_SET);
		if (ret == -1) {
			perror("lseek");
			goto out;
		}
	}

	buf = xmalloc(4096);

	while (size) {
		now = min((ulong)4096, size);
		now = read(fd, buf, now);
		if (now < 0) {
			ret = now;
			perror("read");
			goto out_free;
		}
		if (!now)
			break;

		if (ctrlc()) {
			ret = -EINTR;
			goto out_free;
		}

		d->update(d, buf, now);
		size -= now;
		len += now;
	}

	d->final(d, buf);

	for (i = 0; i < d->length; i++)
		printf("%02x", buf[i]);

	printf("  %s\t0x%08lx ... 0x%08lx\n", filename, start, start + len);

out_free:
	free(buf);
out:
	close(fd);

	return ret;
}

static int do_digest(char *algorithm, int argc, char *argv[])
{
	struct digest *d;
	int ret = 0;

	d = digest_get_by_name(algorithm);
	BUG_ON(!d);

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	argv++;
	while (*argv) {
		char *filename = "/dev/mem";
		ulong start = 0, size = ~0;

		/* arguments are either file, file+area or area */
		if (parse_area_spec(*argv, &start, &size)) {
			filename = *argv;
			if (argv[1] && !parse_area_spec(argv[1], &start, &size))
				argv++;
		}

		if (file_digest(d, filename, start, size) < 0)
			ret = 1;

		argv++;
	}

	return ret;
}

#ifdef CONFIG_CMD_MD5SUM

static int do_md5(struct command *cmdtp, int argc, char *argv[])
{
	return do_digest("md5", argc, argv);
}

BAREBOX_CMD_HELP_START(md5sum)
BAREBOX_CMD_HELP_USAGE("md5sum [[FILE] [AREA]]...\n")
BAREBOX_CMD_HELP_SHORT("Calculate a md5 checksum of a memory area.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(md5sum)
	.cmd		= do_md5,
	.usage		= "md5 checksum calculation",
	BAREBOX_CMD_HELP(cmd_md5sum_help)
BAREBOX_CMD_END

#endif /* CMD_CMD_MD5SUM */

#ifdef CONFIG_CMD_SHA1SUM

static int do_sha1(struct command *cmdtp, int argc, char *argv[])
{
	return do_digest("sha1", argc, argv);
}

BAREBOX_CMD_HELP_START(sha1sum)
BAREBOX_CMD_HELP_USAGE("sha1sum [[FILE] [AREA]]...\n")
BAREBOX_CMD_HELP_SHORT("Calculate a sha1 checksum of a memory area.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(sha1sum)
	.cmd		= do_sha1,
	.usage		= "sha1 checksum calculation",
	BAREBOX_CMD_HELP(cmd_sha1sum_help)
BAREBOX_CMD_END

#endif /* CMD_CMD_SHA1SUM */

#ifdef CONFIG_CMD_SHA256SUM

static int do_sha256(struct command *cmdtp, int argc, char *argv[])
{
	return do_digest("sha256", argc, argv);
}

BAREBOX_CMD_HELP_START(sha256sum)
BAREBOX_CMD_HELP_USAGE("sha256sum [[FILE] [AREA]]...\n")
BAREBOX_CMD_HELP_SHORT("Calculate a sha256 checksum of a memory area.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(sha256sum)
	.cmd		= do_sha256,
	.usage		= "sha256 checksum calculation",
	BAREBOX_CMD_HELP(cmd_sha256sum_help)
BAREBOX_CMD_END

#endif /* CMD_CMD_SHA256SUM */
