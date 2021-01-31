// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2021 Ahmad Fatoum

#include <common.h>
#include <command.h>
#include <sound.h>
#include <getopt.h>

static int do_beep(int argc, char *argv[])
{
	int ret, i, opt;
	u32 tempo, total_us = 0;
	bool wait = false;

	while((opt = getopt(argc, argv, "wc")) > 0) {
		switch(opt) {
		case 'w':
			wait = true;
			break;
		case 'c':
			return beep_cancel();
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0 || argc % 2 != 1)
		return COMMAND_ERROR_USAGE;

	ret = kstrtou32(argv[0], 0, &tempo);
	if (ret || tempo == 0)
		return COMMAND_ERROR_USAGE;

	tempo = 60 * USEC_PER_SEC / tempo;

	if (argc == 1) {
		ret = beep(BELL_DEFAULT_FREQUENCY, tempo);
		if (ret)
			return ret;

		total_us += tempo;
		goto out;
	}

	for (i = 1; i < argc; i += 2) {
		u32 pitch = 0, duration;
		u16 val;

		ret = kstrtou16(argv[i], 0, &val);
		if (ret)
			return COMMAND_ERROR_USAGE;

		if (val)
			pitch = clamp_t(unsigned, val, 20, 20000);

		ret = kstrtou16(argv[i+1], 0, &val);
		if (ret)
			return COMMAND_ERROR_USAGE;

		duration = val * tempo;

		ret = beep(pitch, duration);
		if (ret)
			return ret;

		total_us += duration;
	}

out:
	if (wait)
		beep_wait(total_us);

	return 0;
}

/* https://www.gnu.org/software/grub/manual/grub/html_node/play.html */
BAREBOX_CMD_HELP_START(beep)
BAREBOX_CMD_HELP_TEXT("Tempo is an unsigned 32bit number. It's followed by pairs of unsigned")
BAREBOX_CMD_HELP_TEXT("16bit numbers for pitch and duration.")
BAREBOX_CMD_HELP_TEXT("The tempo is the base for all note durations. 60 gives a 1-second base,")
BAREBOX_CMD_HELP_TEXT("120 gives a half-second base, etc. Pitches are Hz.")
BAREBOX_CMD_HELP_TEXT("Set pitch to 0 to produce a rest.")
BAREBOX_CMD_HELP_TEXT("When only tempo is given, a beep of duration 1 at bell frequency results.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-c",  "cancel pending beeps")
BAREBOX_CMD_HELP_OPT ("-w",  "wait until beep is over")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(beep)
	.cmd = do_beep,
	BAREBOX_CMD_DESC("play a GRUB beep tune")
	BAREBOX_CMD_OPTS("tempo [pitch1 duration1 [pitch2 duration2] ...]")
	BAREBOX_CMD_HELP(cmd_beep_help)
	BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
BAREBOX_CMD_END
