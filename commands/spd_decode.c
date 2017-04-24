/*
 * This program is decoding and printing SPD contents
 * in human readable format
 * As an argument program, you must specify the file name.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ALTERA CORPORATION BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (C) 2015 Alexander Smirnov <alllecs@yandex.ru>
 */

#include <common.h>
#include <command.h>
#include <libfile.h>
#include <malloc.h>
#include <ddr_spd.h>

static int do_spd_decode(int argc, char *argv[])
{
	int ret;
	size_t size;
	void *data;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	ret = read_file_2(argv[1], &size, &data, 256);
	if (ret && ret != -EFBIG) {
		printf("unable to read %s: %s\n", argv[1], strerror(-ret));
		return COMMAND_ERROR;
	}

	printf("Decoding EEPROM: %s\n\n", argv[1]);
	ddr_spd_print(data);

	free(data);

	return 0;
}

BAREBOX_CMD_HELP_START(spd_decode)
BAREBOX_CMD_HELP_TEXT("Decode a SPD EEPROM and print contents in human readable form")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(spd_decode)
	.cmd	= do_spd_decode,
	BAREBOX_CMD_DESC("Decode SPD EEPROM")
	BAREBOX_CMD_OPTS("FILE")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_spd_decode_help)
BAREBOX_CMD_END
