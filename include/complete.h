#ifndef __COMPLETE_
#define __COMPLETE_

#include <linux/list.h>
#include <malloc.h>
#include <stringlist.h>

#define COMPLETE_END		0
#define COMPLETE_CONTINUE	1

int complete(char *instr, char **outstr);
void complete_reset(void);

int command_complete(struct string_list *sl, char *instr);
int device_complete(struct string_list *sl, char *instr);
int empty_complete(struct string_list *sl, char *instr);
int eth_complete(struct string_list *sl, char *instr);
int command_var_complete(struct string_list *sl, char *instr);
int devfs_partition_complete(struct string_list *sl, char *instr);

#endif /* __COMPLETE_ */

