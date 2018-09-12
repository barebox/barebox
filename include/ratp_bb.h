#ifndef __RATP_BB_H
#define __RATP_BB_H

#include <linux/stringify.h>

#define BB_RATP_TYPE_COMMAND		1
#define BB_RATP_TYPE_COMMAND_RETURN	2
#define BB_RATP_TYPE_CONSOLEMSG		3
#define BB_RATP_TYPE_PING		4
#define BB_RATP_TYPE_PONG		5
#define BB_RATP_TYPE_GETENV		6
#define BB_RATP_TYPE_GETENV_RETURN	7
#define BB_RATP_TYPE_FS			8
#define BB_RATP_TYPE_FS_RETURN		9
#define BB_RATP_TYPE_MD			10
#define BB_RATP_TYPE_MD_RETURN		11
#define BB_RATP_TYPE_MW			12
#define BB_RATP_TYPE_MW_RETURN		13
#define BB_RATP_TYPE_RESET		14
#define BB_RATP_TYPE_I2C_READ		15
#define BB_RATP_TYPE_I2C_READ_RETURN	16
#define BB_RATP_TYPE_I2C_WRITE		17
#define BB_RATP_TYPE_I2C_WRITE_RETURN	18
#define BB_RATP_TYPE_GPIO_GET_VALUE		19
#define BB_RATP_TYPE_GPIO_GET_VALUE_RETURN	20
#define BB_RATP_TYPE_GPIO_SET_VALUE		21
#define BB_RATP_TYPE_GPIO_SET_VALUE_RETURN	22
#define BB_RATP_TYPE_GPIO_SET_DIRECTION		23
#define BB_RATP_TYPE_GPIO_SET_DIRECTION_RETURN	24

struct ratp_bb {
	uint16_t type;
	uint16_t flags;
	uint8_t data[];
};

struct ratp_bb_pkt {
	unsigned int len;
	uint8_t data[];
};

int  barebox_ratp(struct console_device *cdev);
void barebox_ratp_command_run(void);
int  barebox_ratp_fs_call(struct ratp_bb_pkt *tx, struct ratp_bb_pkt **rx);
int  barebox_ratp_fs_mount(const char *path);

/*
 * RATP commands definition
 */

struct ratp_command {
	struct list_head  list;
	uint16_t          request_id;
	uint16_t          response_id;
	int		(*cmd)(const struct ratp_bb *req,
			       int req_len,
			       struct ratp_bb **rsp,
			       int *rsp_len);
}
#ifdef __x86_64__
/* This is required because the linker will put symbols on a 64 bit alignment */
__attribute__((aligned(64)))
#endif
;

#define BAREBOX_RATP_CMD_START(_name)							\
extern const struct ratp_command __barebox_ratp_cmd_##_name;				\
const struct ratp_command __barebox_ratp_cmd_##_name					\
	__attribute__ ((unused,section (".barebox_ratp_cmd_" __stringify(_name)))) = {

#define BAREBOX_RATP_CMD_END								\
};

int register_ratp_command(struct ratp_command *cmd);

#endif /* __RATP_BB_H */
