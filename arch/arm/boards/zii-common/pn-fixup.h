/*
 * Copyright (C) 2019 Zodiac Inflight Innovation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ZII_PN_FIXUP__
#define __ZII_PN_FIXUP__

struct zii_pn_fixup {
	const char *pn;
	void (*callback) (const struct zii_pn_fixup *fixup);
};

char *zii_read_part_number(const char *, size_t);
/**
 * __zii_process_fixups - Process array of ZII part number based fixups
 *
 * @__fixups: Array of part number base fixups
 * @__cell_name: Name of the NVMEM cell containing the part number
 * @__cell_size: Size of the NVMEM cell containing the part number
 *
 * NOTE: Keeping this code as a marcro allows us to avoid restricting
 * the type of __fixups to an array of struct zii_pn_fixup. This is
 * really convenient becuase it allows us to do things like
 *
 * struct zii_foo_fixup {
 * 	struct zii_pn_fixup parent;
 *	type1 custom_field_1
 *	type2 custom_field_2
 *      ...
 * };
 *
 * ...
 *
 * const struct zii_foo_fixup foo_fixups[] = {
 * 	{ fixup1 },
 * 	{ fixup2 },
 * 	{ fixup3 },
 * };
 *
 * ...
 *
 * __zii_process_fixups(foo_fixups, "blah", BLAH_LENGTH);
 *
 * which allows us to have the most compact definition of array of
 * fixups
 */
#define __zii_process_fixups(__fixups, __cell_name, __cell_size)	\
	do {								\
		char *__pn = zii_read_part_number(__cell_name,		\
						  __cell_size);		\
		const struct zii_pn_fixup *__fixup;			\
		unsigned int __i;					\
		bool __match_found = false;				\
									\
		if (WARN_ON(IS_ERR(__pn)))				\
			break;						\
									\
		for (__i = 0; __i < ARRAY_SIZE(__fixups); __i++) {	\
			__fixup =					\
			  (const struct zii_pn_fixup *) &__fixups[__i]; \
									\
			if (strstr(__pn, __fixup->pn)) {		\
				pr_debug("%s->%pS\n", __func__,		\
					 __fixup->callback);		\
				__match_found = true;			\
				__fixup->callback(__fixup);		\
			}						\
		}							\
		if (!__match_found)					\
			pr_err("No config fixups found for P/N %s!\n", __pn); \
		free(__pn);						\
	} while (0)

#define DDS_PART_NUMBER_SIZE	15
#define LRU_PART_NUMBER_SIZE	15

#define zii_process_dds_fixups(_fixups)		\
	__zii_process_fixups(_fixups, "dds-part-number", DDS_PART_NUMBER_SIZE)

#define zii_process_lru_fixups(_fixups)		\
	__zii_process_fixups(_fixups, "lru-part-number", LRU_PART_NUMBER_SIZE)

#endif	/* __ZII_PN_FIXUP__ */
