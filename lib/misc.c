
#include <common.h>
#include <command.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <errno.h>
#include <fs.h>
#include <net.h>

int cmd_get_data_size(char* arg, int default_size)
{
	/* Check for a size specification .b, .w or .l.
	 */
	int len = strlen(arg);
	if (len > 2 && arg[len-2] == '.') {
		switch(arg[len-1]) {
		case 'b':
			return 1;
		case 'w':
			return 2;
		case 'l':
			return 4;
		case 's':
			return -2;
		default:
			return -1;
		}
	}
	return default_size;
}


unsigned long strtoul_suffix(const char *str, char **endp, int base)
{
        unsigned long val;
        char *end;

        val = simple_strtoul(str, &end, base);

        switch (*end) {
        case 'G':
                val *= 1024;
        case 'M':
		val *= 1024;
        case 'k':
        case 'K':
                val *= 1024;
                end++;
        }

        if (endp)
                *endp = (char *)end;

        return val;
}

int parse_area_spec(const char *str, ulong *start, ulong *size)
{
	char *endp;

	if (*str == '+') {
		/* no beginning given but size so start is 0 */
		*start = 0;
		*size = strtoul_suffix(str + 1, &endp, 0);
		return 0;
	}

	if (*str == '-') {
		/* no beginning given but end, so start is 0 */
		*start = 0;
		*size = strtoul_suffix(str + 1, &endp, 0) + 1;
		return 0;
	}

	*start = strtoul_suffix(str, &endp, 0);

	str = endp;

	if (!*str) {
		/* beginning given, but no size, assume maximum size */
		*size = ~0;
		return 0;
	}

	if (*str == '-') {
                /* beginning and end given */
		*size = strtoul_suffix(str + 1, NULL, 0) + 1;
		return 0;
	}

	if (*str == '+') {
                /* beginning and size given */
		*size = strtoul_suffix(str + 1, NULL, 0);
		return 0;
	}

	return -1;
}

int spec_str_to_info(const char *str, struct memarea_info *info)
{
	char *endp;
	int ret;

	info->device = device_from_spec_str(str, &endp);
	if (!info->device) {
		printf("unknown device: %s\n", deviceid_from_spec_str(str, NULL));
		return -ENODEV;
	}

	ret = parse_area_spec(endp, &info->start, &info->size);
	if (ret)
		return ret;

	if (info->size == ~0)
		info->size = info->device->size;

	return 0;
}

