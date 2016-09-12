#include <common.h>
#include <command.h>
#include <getopt.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <rtc.h>
#include <sntp.h>
#include <linux/rtc.h>
#include <string.h>
#include <environment.h>

static char *strchrnul(const char *s, int c)
{
	while (*s != '\0' && *s != c)
		s++;

	return (char *)s;
}

static int sscanf_two_digits(char *s, int *res)
{
	char buf[3];
	unsigned long t;

	if (!isdigit(s[0]) || !isdigit(s[1])) {
		return -EINVAL;
	}

	buf[0] = s[0];
	buf[1] = s[1];
	buf[2] = '\0';

	t = simple_strtoul(buf, NULL, 10);
	*res = t;

	return 0;
}

static int parse_datestr(char *date_str, struct rtc_time *ptm)
{
	char end = '\0';
	int len = strchrnul(date_str, '.') - date_str;
	int year;

	/* ccyymmddHHMM[.SS] */
	if (len != 12) {
		return -EINVAL;
	}

	if (sscanf_two_digits(date_str, &year) ||
		sscanf_two_digits(&date_str[2], &ptm->tm_year)) {
		return -EINVAL;
	}

	ptm->tm_year = year * 100 + ptm->tm_year;

	/* Adjust years */
	ptm->tm_year -= 1900;

	if (sscanf_two_digits(&date_str[4], &ptm->tm_mon) ||
		sscanf_two_digits(&date_str[6], &ptm->tm_mday) ||
		sscanf_two_digits(&date_str[8], &ptm->tm_hour) ||
		sscanf_two_digits(&date_str[10], &ptm->tm_min)) {
		return -EINVAL;
	}

	/* Adjust month from 1-12 to 0-11 */
	ptm->tm_mon -= 1;

	end = date_str[12];

	if (end == '.') {
		/* xxx.SS */
		if (!sscanf_two_digits(&date_str[13], &ptm->tm_sec)) {
			end = '\0';
		}
		/* else end != NUL and we error out */
	}

	if (end != '\0') {
		return -EINVAL;
	}

	return 0;
}

static int do_hwclock(int argc, char *argv[])
{
	struct rtc_device *r;
	struct rtc_time tm;
	struct rtc_time stm;
	char rtc_name[16] = "rtc0";
	char *env_name = NULL;
	int opt;
	int set = 0;
	int ret;
	int ntp_to_hw = 0;
	char *ntpserver = NULL;

	while ((opt = getopt(argc, argv, "f:s:e:n:")) > 0) {

		switch (opt) {
		case 'f':
			strncpy(rtc_name, optarg, 16);
			break;
		case 's':
			memset(&stm, 0, sizeof(stm));

			ret = parse_datestr(optarg, &stm);
			if (ret)
				return ret;

			ret = rtc_valid_tm(&stm);
			if (ret)
				return ret;

			set = 1;
			break;
		case 'e':
			env_name = optarg;
			break;
		case 'n':
			ntp_to_hw = 1;
			ntpserver = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	r = rtc_lookup(rtc_name);
	if (IS_ERR(r))
		return PTR_ERR(r);

	if (set) {
		return rtc_set_time(r, &stm);
	}

	if (ntp_to_hw) {
		s64 now;

		if (!IS_ENABLED(CONFIG_NET_SNTP)) {
			printf("SNTP support is disabled\n");
			return 1;
		}

		now = sntp(ntpserver);
		if (now < 0)
			return now;

		rtc_time_to_tm(now, &stm);
		printf("%s\n", time_str(&stm));
		return rtc_set_time(r, &stm);
	}

	ret = rtc_read_time(r, &tm);
	if (ret < 0)
		return ret;

	if (env_name) {
		unsigned long time;
		char t[12];

		rtc_tm_to_time(&tm, &time);
		snprintf(t, 12, "%lu", time);
		setenv(env_name, t);
	} else {
		printf("%s\n", time_str(&tm));
	}

	return 0;
}

BAREBOX_CMD_HELP_START(hwclock)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-f NAME\t\t\t", "RTC device name (default rtc0)")
BAREBOX_CMD_HELP_OPT ("-e VARNAME\t\t", "store RTC readout into variable VARNAME")
BAREBOX_CMD_HELP_OPT ("-n NTPSERVER\t", "set RTC from NTP server")
BAREBOX_CMD_HELP_OPT ("-s ccyymmddHHMM[.SS]\t", "set time")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(hwclock)
	.cmd		= do_hwclock,
	BAREBOX_CMD_DESC("query or set the hardware clock (RTC)")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_hwclock_help)
BAREBOX_CMD_END
