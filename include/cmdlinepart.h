#ifndef __CMD_LINE_PART_H
#define __CMD_LINE_PART_H

#define CMDLINEPART_ADD_DEVNAME (1 << 0)

int cmdlinepart_do_parse_one(char *devname, const char *partstr,
				 char **endp, loff_t *offset,
				 loff_t devsize, size_t *retsize,
				 unsigned int partition_flags);

#endif /* __CMD_LINE_PART_H */
