// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: © 2023 Marc Reilly <marc@cpdesign.com.au>

/* pwm - pwm commands */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <pwm.h>

#define HZ_TO_NANOSECONDS(x) (1000000000UL/(x))
#define HZ_FROM_NANOSECONDS(x) (1000000000UL/(x))

static bool is_equal_state(struct pwm_state *state1, struct pwm_state *state2)
{
	return (state1->period_ns == state2->period_ns)
		&& (state1->duty_ns == state2->duty_ns)
		&& (state1->polarity == state2->polarity)
		&& (state1->p_enable == state2->p_enable);
}

static int do_pwm_cmd(int argc, char *argv[])
{
	struct pwm_device *pwm = NULL;
	struct pwm_state state, orig_state;
	int error = 0;
	char *devname = NULL;
	int duty = -1, period = -1;
	int freq = -1, width = -1;
	bool invert_polarity = false, stop = false;
	bool use_default_width = false;
	bool verbose = false;
	int opt;

	while ((opt = getopt(argc, argv, "d:D:P:f:w:F:isv")) > 0) {
		switch (opt) {
		case 'd':
			devname = optarg;
			break;
		case 'D':
			duty = simple_strtol(optarg, NULL, 0);
			break;
		case 'P':
			period = simple_strtol(optarg, NULL, 0);
			break;
		case 'F':
			/* convenience option for changing frequency without
			 * having to specify duty width */
			use_default_width = true;
			/* fallthrough */
		case 'f':
			freq = simple_strtol(optarg, NULL, 0);
			break;
		case 'w':
			width = simple_strtol(optarg, NULL, 0);
			break;
		case 'i':
			invert_polarity = true;
			break;
		case 's':
			stop = true;
			break;
		case 'v':
			verbose = true;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (!devname) {
		printf(" need to specify a device\n");
		return COMMAND_ERROR;
	}

	if ((freq == 0) || (period == 0)) {
		printf(" period or freqency needs to be non-zero\n");
		return COMMAND_ERROR;
	}

	if (freq >= 0 && period >= 0) {
		printf(" specify period or frequency, not both\n");
		return COMMAND_ERROR;
	}

	if (duty >= 0 && width >= 0) {
		printf(" specify duty or width, not both\n");
		return COMMAND_ERROR;
	}

	if (width > 100) {
		printf(" width (%% duty cycle) can't be more than 100%%\n");
		return COMMAND_ERROR;
	}

	pwm = pwm_request(devname);
	if (!pwm) {
		printf(" pwm device %s not found\n", devname);
		return -ENODEV;
	}

	pwm_get_state(pwm, &state);

	/* argc will be at least 3 with a valid devname */
	if (verbose || (argc <= 3)) {
		printf("pwm params for '%s':\n", devname);
		printf("  period   : %u (ns)\n", state.period_ns);
		printf("  duty     : %u (ns)\n", state.duty_ns);
		printf("  enabled  : %d\n", state.p_enable);
		printf("  polarity : %s\n", state.polarity == 0 ? "Normal" : "Inverted");
		if (state.period_ns)
			printf("  freq     : %lu (Hz)\n", HZ_FROM_NANOSECONDS(state.period_ns));
		else
			printf("  freq     : -\n");

		pwm_free(pwm);
		return 0;
	}

	if ((state.period_ns == 0) && (freq < 0) && (period < 0)) {
		printf(" need to know some timing info: freq or period\n");
		pwm_free(pwm);
		return COMMAND_ERROR;
	}

	orig_state = state;

	if (invert_polarity)
		state.polarity = invert_polarity;

	/* period */
	if (freq > 0) {
		state.p_enable = true;
		state.period_ns = HZ_TO_NANOSECONDS(freq);
		if (use_default_width && (width < 0)) {
			width = 50;
		}
	} else if (period > 0) {
		state.p_enable = true;
		state.period_ns = period;
	}

	/* duty */
	if (width >= 0) {
		pwm_set_relative_duty_cycle(&state, width, 100);
	} else if (duty >= 0) {
		state.duty_ns = duty;
	}

	if (state.duty_ns > state.period_ns) {
		printf(" duty_ns must not be greater than period_ns\n");
	}

	/* only set the state if its changed */
	if (!is_equal_state(&orig_state, & state))
		error = pwm_apply_state(pwm, &state);

	if (error < 0)
		printf(" error while applying state: %d\n", error);

	/* stop handled as an additional step on purpose, allows turning off
	 * output (eg if duty => 0) and stopping in one command
	 */
	if (stop > 0) {
		state.p_enable = false;
		error = pwm_apply_state(pwm, &state);
		if (error < 0)
			printf(" error while stopping: %d\n", error);
	}

	pwm_free(pwm);

	return error;
}

BAREBOX_CMD_HELP_START(pwm)
BAREBOX_CMD_HELP_TEXT("Sets pwm device parameters, or shows current value.")
BAREBOX_CMD_HELP_TEXT(" Specify the pwm device by device name.")
BAREBOX_CMD_HELP_TEXT(" If no other parameters are given, or if args has '-v',")
BAREBOX_CMD_HELP_TEXT("  then show the current values only.")
BAREBOX_CMD_HELP_TEXT(" Timings can either be specified via period + duty (on) duration,")
BAREBOX_CMD_HELP_TEXT("  or via frequency. Duty can be given either as a percentage or time.")
BAREBOX_CMD_HELP_TEXT(" If a parameter is not specified, the current value will be used")
BAREBOX_CMD_HELP_TEXT("  where possible.")
BAREBOX_CMD_HELP_TEXT(" To set an output to inactive state, set the duty to 0.")
BAREBOX_CMD_HELP_TEXT("  (although note this will not by itself stop the pwm running.)")
BAREBOX_CMD_HELP_TEXT(" Stopping the pwm device does not necessarily set the output to inactive,")
BAREBOX_CMD_HELP_TEXT("  but stop is handled last, so can be done in addition to other changes.")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-d <name>", "device name (eg 'pwm0')")
BAREBOX_CMD_HELP_OPT("-D <duty_ns>", "duty cycle (ns)")
BAREBOX_CMD_HELP_OPT("-P <period_ns>", "period (ns)")
BAREBOX_CMD_HELP_OPT("-f <freq_hz>", "frequency (Hz)")
BAREBOX_CMD_HELP_OPT("-w <duty_%>", "duty cycle (%) - the on 'width' of each cycle")
BAREBOX_CMD_HELP_OPT("-i\t", "line inverted polarity")
BAREBOX_CMD_HELP_OPT("-s\t", "stop (disable) the pwm device")
BAREBOX_CMD_HELP_OPT("-v\t", "print current values")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(pwm)
	.cmd		= do_pwm_cmd,
	BAREBOX_CMD_DESC("pwm")
	BAREBOX_CMD_OPTS("[-dDPfwisv]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_pwm_help)
BAREBOX_CMD_END
