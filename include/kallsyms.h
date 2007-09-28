#ifndef __KALLSYMS_H
#define __KALLSYMS_H

#define KSYM_NAME_LEN 128
unsigned long kallsyms_lookup_name(const char *name);

#endif /* __KALLSYMS_H */
