#ifndef __COMPLETE_
#define __COMPLETE_

#include <linux/list.h>

int complete(char *instr, char **outstr);
void complete_reset(void);

#endif /* __COMPLETE_ */

