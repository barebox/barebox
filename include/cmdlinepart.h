#ifndef __CMD_LINE_PART_H
#define __CMD_LINE_PART_H

#define CMDLINEPART_ADD_DEVNAME (1 << 0)

int cmdlinepart_do_parse_one(const char *devname, const char *partstr,
				 const char **endp, loff_t *offset,
				 loff_t devsize, loff_t *retsize,
				 unsigned int partition_flags);

int cmdlinepart_do_parse(const char *devname, const char *parts, loff_t devsize,
		unsigned partition_flags);

#endif /* __CMD_LINE_PART_H */
