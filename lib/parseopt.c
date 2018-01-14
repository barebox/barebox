#include <common.h>

#include "parseopt.h"

void parseopt_b(const char *options, const char *opt, bool *val)
{
	const char *start;
	size_t optlen = strlen(opt);

again:
	start = strstr(options, opt);

	if (!start) {
		*val = false;
		return;
	}

	if (start > options && start[-1] != ',') {
		options = start;
		goto again;
	}

	if (start[optlen] != ',' && start[optlen] != '\0') {
		options = start;
		goto again;
	}

	*val = true;
}

void parseopt_hu(const char *options, const char *opt, unsigned short *val)
{
	const char *start;
	size_t optlen = strlen(opt);
	ulong v;
	char *endp;

again:
	start = strstr(options, opt);

	if (!start)
		return;

	if (start > options && start[-1] != ',') {
		options = start;
		goto again;
	}

	if (start[optlen] != '=') {
		options = start;
		goto again;
	}

	v = simple_strtoul(start + optlen + 1, &endp, 0);
	if (v > USHRT_MAX)
		return;

	if (*endp == ',' || *endp == '\0')
		*val = v;
}

void parseopt_u16(const char *options, const char *opt, uint16_t *val)
{
	const char *start;
	size_t optlen = strlen(opt);
	ulong v;
	char *endp;

again:
	start = strstr(options, opt);

	if (!start)
		return;

	if (start > options && start[-1] != ',') {
		options = start;
		goto again;
	}

	if (start[optlen] != '=') {
		options = start;
		goto again;
	}

	v = simple_strtoul(start + optlen + 1, &endp, 0);
	if (v > U16_MAX)
		return;

	if (*endp == ',' || *endp == '\0')
		*val = v;
}

void parseopt_str(const char *options, const char *opt, char **val)
{
	const char *start;
	size_t optlen = strlen(opt);
	char *endp;
	char *parsed;

again:
	start = strstr(options, opt);

	if (!start)
		return;

	if (start > options && start[-1] != ',') {
		options = start;
		goto again;
	}

	if (start[optlen] != '=') {
		options = start;
		goto again;
	}

	parsed = (char *)start + optlen + 1;
	endp = parsed;
	while (*endp != '\0' && *endp != ',') {

		endp++;
	}

	*val = xstrndup(parsed, endp - parsed);
}
