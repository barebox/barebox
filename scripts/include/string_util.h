#ifndef _TOOLS_STRING_UTIL_H_
#define _TOOLS_STRING_UTIL_H_

#include <linux/types.h>
#include <stddef.h>

// SPDX-SnippetBegin
// SPDX-Snippet-Comment: Origin-URL: https://git.pengutronix.de/cgit/barebox/tree/lib/string.c?id=dfcf686f94a5a5387660f2afab79a714baab828a

/**
 * strsep_unescaped - Split a string into tokens, while ignoring escaped delimiters
 * @s: The string to be searched
 * @ct: The delimiter characters to search for
 * @delim: optional pointer to store found delimiter into
 *
 * strsep_unescaped() behaves like strsep unless it meets an escaped delimiter.
 * In that case, it shifts the string back in memory to overwrite the escape's
 * backslash then continues the search until an unescaped delimiter is found.
 *
 * On end of string, this function returns NULL. As long as a non-NULL
 * value is returned and @delim is not NULL, the found delimiter will
 * be stored into *@delim.
 */
static char *strsep_unescaped(char **s, const char *ct, char *delim)
{
	char *sbegin = *s, *hay;
	const char *needle;
	size_t shift = 0;

	if (sbegin == NULL)
		return NULL;

	for (hay = sbegin; *hay != '\0'; ++hay) {
		*hay = hay[shift];

		if (*hay == '\\') {
			*hay = hay[++shift];
			if (*hay != '\\')
				continue;
		}

		for (needle = ct; *needle != '\0'; ++needle) {
			if (*hay == *needle)
				goto match;
		}
	}

	*s = NULL;
	if (delim)
		*delim = '\0';
	return sbegin;

match:
	if (delim)
		*delim = *hay;
	*hay = '\0';
	*s = &hay[shift + 1];

	return sbegin;
}

// SPDX-SnippetEnd


#endif /* _TOOLS_STRING_UTIL_H_ */
