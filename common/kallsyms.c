#include <common.h>
#include <init.h>
#include <kallsyms.h>

/* These will be re-linked against their real values during the second link stage */
extern const unsigned long kallsyms_addresses[] __attribute__((weak));
extern const unsigned long kallsyms_num_syms __attribute__((weak));
extern const u8 kallsyms_names[] __attribute__((weak));

extern const u8 kallsyms_token_table[] __attribute__((weak));
extern const u16 kallsyms_token_index[] __attribute__((weak));

extern const unsigned long kallsyms_markers[] __attribute__((weak));

/* expand a compressed symbol data into the resulting uncompressed string,
   given the offset to where the symbol is in the compressed stream */
static unsigned int kallsyms_expand_symbol(unsigned int off, char *result)
{
	int len, skipped_first = 0;
	const u8 *tptr, *data;

	/* get the compressed symbol length from the first symbol byte */
	data = &kallsyms_names[off];
	len = *data;
	data++;

	/* update the offset to return the offset for the next symbol on
	 * the compressed stream */
	off += len + 1;

	/* for every byte on the compressed symbol data, copy the table
	   entry for that byte */
	while(len) {
		tptr = &kallsyms_token_table[ kallsyms_token_index[*data] ];
		data++;
		len--;

		while (*tptr) {
			if(skipped_first) {
				*result = *tptr;
				result++;
			} else
				skipped_first = 1;
			tptr++;
		}
	}

	*result = '\0';

	/* return to offset to the next symbol */
	return off;
}

/* Lookup the address for this symbol. Returns 0 if not found. */
unsigned long kallsyms_lookup_name(const char *name)
{
	char namebuf[KSYM_NAME_LEN];
	unsigned long i;
	unsigned int off;

	for (i = 0, off = 0; i < kallsyms_num_syms; i++) {
		off = kallsyms_expand_symbol(off, namebuf);

		if (strcmp(namebuf, name) == 0)
			return kallsyms_addresses[i];
	}
//	return module_kallsyms_lookup_name(name);
	return 0;
}
