#ifndef __PBL_HANDOFF_DATA_H
#define __PBL_HANDOFF_DATA_H

#include <linux/list.h>

struct handoff_data {
	struct list_head entries;
};

#define HANDOFF_DATA_BAREBOX(n)		(0x28061971 + (n))
#define HANDOFF_DATA_INTERNAL_DT	HANDOFF_DATA_BAREBOX(0)
#define HANDOFF_DATA_INTERNAL_DT_Z	HANDOFF_DATA_BAREBOX(1)
#define HANDOFF_DATA_EXTERNAL_DT	HANDOFF_DATA_BAREBOX(2)
#define HANDOFF_DATA_BOARDDATA		HANDOFF_DATA_BAREBOX(3)

#define HANDOFF_DATA_BOARD(n)		(0x951726fb + (n))

struct handoff_data_entry {
	struct list_head list;
	void *data;
	size_t size;
	unsigned int cookie;
#define HANDOFF_DATA_FLAG_NO_COPY	BIT(0)
	unsigned int flags;
};

#define handoff_data_add_flags(_cookie, _data, _size, _flags)	\
	do {							\
		static struct handoff_data_entry hde;		\
		hde.cookie = _cookie;				\
		hde.data = _data;				\
		hde.size = _size;				\
		hde.flags = _flags;				\
								\
		handoff_data_add_entry(&hde);			\
	} while (0);

#define handoff_data_add(_cookie, _data, _size)			\
	handoff_data_add_flags((_cookie), (_data), (_size), 0)

void handoff_data_add_entry(struct handoff_data_entry *entry);
void handoff_data_move(void *dest);
void handoff_data_set(struct handoff_data *handoff);
void *handoff_data_get_entry(unsigned int cookie, size_t *size);
int handoff_data_show(void);

size_t __handoff_data_size(const struct handoff_data *hd);
static inline size_t handoff_data_size(void)
{
	return __handoff_data_size(NULL);
}

#endif /* __PBL_HANDOFF_DATA_H */
