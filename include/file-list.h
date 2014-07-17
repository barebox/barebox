#ifndef __FILE_LIST
#define __FILE_LIST

#define FILE_LIST_FLAG_SAFE	(1 << 0)
#define FILE_LIST_FLAG_READBACK	(1 << 1)
#define FILE_LIST_FLAG_CREATE	(1 << 2)

struct file_list_entry {
	char *name;
	char *filename;
	unsigned long flags;
	struct list_head list;
};

struct file_list {
	struct list_head list;
	int num_entries;
};

struct file_list *file_list_parse(const char *str);
void file_list_free(struct file_list *);

#define file_list_for_each_entry(files, entry) \
	list_for_each_entry(entry, &files->list, list)

#endif /* __FILE_LIST */
