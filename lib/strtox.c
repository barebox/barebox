#include <common.h>
#include <linux/ctype.h>

unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}
		if (!base) {
			base = 8;
		}
	}
	if (!base) {
		base = 10;
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}
EXPORT_SYMBOL(simple_strtoul);

long simple_strtol(const char *cp,char **endp,unsigned int base)
{
	if(*cp=='-')
		return -simple_strtoul(cp+1,endp,base);
	return simple_strtoul(cp,endp,base);
}
EXPORT_SYMBOL(simple_strtol);

unsigned long long simple_strtoull (const char *cp, char **endp, unsigned int base)
{
	unsigned long long result = 0, value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit (cp[1])) {
			base = 16;
			cp++;
		}
		if (!base) {
			base = 8;
		}
	}
	if (!base) {
		base = 10;
	}
	while (isxdigit (*cp) && (value = isdigit (*cp)
				? *cp - '0'
				: (islower (*cp) ? toupper (*cp) : *cp) - 'A' + 10) < base) {
		result = result * base + value;
		cp++;
	}
	if (endp)
		*endp = (char *) cp;
	return result;
}
EXPORT_SYMBOL(simple_strtoull);

