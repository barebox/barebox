#ifndef __COMPLETE_
#define __COMPLETE_

#include <list.h>

int complete(char *instr, char **outstr);
void complete_reset(void);

struct complete_handle {
	struct list_head list;
	char str[0];
};

#endif /* __COMPLETE_ */

