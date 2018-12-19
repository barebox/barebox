
#ifndef __LIBBB_H
#define __LIBBB_H

#include <linux/stat.h>

#define DOT_OR_DOTDOT(s) ((s)[0] == '.' && (!(s)[1] || ((s)[1] == '.' && !(s)[2])))

char *concat_path_file(const char *path, const char *filename);
char *concat_subpath_file(const char *path, const char *f);
int execable_file(const char *name);
char *find_execable(const char *filename);
char* last_char_is(const char *s, int c);

enum {
	ACTION_RECURSE     = (1 << 0),
	ACTION_FOLLOWLINKS    = (1 << 1),
	ACTION_DEPTHFIRST  = (1 << 2),
	/*ACTION_REVERSE   = (1 << 3), - unused */
	ACTION_SORT        = (1 << 4),
};

int recursive_action(const char *fileName, unsigned flags,
	int (*fileAction) (const char *fileName, struct stat* statbuf, void* userData, int depth),
	int (*dirAction) (const char *fileName, struct stat* statbuf, void* userData, int depth),
	void* userData, const unsigned depth);

char * safe_strncpy(char *dst, const char *src, size_t size);

int process_escape_sequence(const char *source, char *dest, int destlen);

char *simple_itoa(unsigned int i);

#endif /* __LIBBB_H */
