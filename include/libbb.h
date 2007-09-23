
#ifndef __LIBBB_H
#define __LIBBB_H
char *concat_path_file(const char *path, const char *filename);
int execable_file(const char *name);
char *find_execable(const char *filename);
char* last_char_is(const char *s, int c);

#endif /* __LIBBB_H */
