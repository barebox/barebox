#ifndef __PARSEOPT_H__
#define __PARSEOPT_H__
void parseopt_llu_suffix(const char *options, const char *opt,
			 unsigned long long *val);

void parseopt_b(const char *options, const char *opt, bool *val);
void parseopt_hu(const char *options, const char *opt, unsigned short *val);
void parseopt_u16(const char *options, const char *opt, uint16_t *val);
void parseopt_str(const char *options, const char *opt, char **val);

#endif /* __PARSEOPT_H__ */
