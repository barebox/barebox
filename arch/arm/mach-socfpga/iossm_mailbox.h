/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022-2023 Intel Corporation <www.intel.com>
 */

#ifndef IOSSM_MAILBOX_H_
#define IOSSM_MAILBOX_H_

#define IOSSM_STATUS_CAL_SUCCESS		BIT(0)
#define IOSSM_STATUS_CAL_FAIL			BIT(1)
#define IOSSM_STATUS_CAL_BUSY			BIT(2)
#define IOSSM_STATUS_COMMAND_RESPONSE_READY	BIT(0)
#define IOSSM_CMD_RESPONSE_STATUS_OFFSET	0x45C
#define IOSSM_CMD_RESPONSE_DATA_0_OFFSET	0x458
#define IOSSM_CMD_RESPONSE_DATA_1_OFFSET	0x454
#define IOSSM_CMD_RESPONSE_DATA_2_OFFSET	0x450
#define IOSSM_CMD_REQ_OFFSET			0x43C
#define IOSSM_CMD_PARAM_0_OFFSET		0x438
#define IOSSM_CMD_PARAM_1_OFFSET		0x434
#define IOSSM_CMD_PARAM_2_OFFSET		0x430
#define IOSSM_CMD_PARAM_3_OFFSET		0x42C
#define IOSSM_CMD_PARAM_4_OFFSET		0x428
#define IOSSM_CMD_PARAM_5_OFFSET		0x424
#define IOSSM_CMD_PARAM_6_OFFSET		0x420
#define IOSSM_STATUS_OFFSET			0x400
#define IOSSM_CMD_RESPONSE_DATA_SHORT_MASK	GENMASK(31, 16)
#define IOSSM_CMD_RESPONSE_DATA_SHORT(data)	(((data) & IOSSM_CMD_RESPONSE_DATA_SHORT_MASK) >> 16)
#define MAX_IO96B_SUPPORTED			2

/* supported mailbox command type */
enum iossm_mailbox_cmd_type  {
	CMD_NOP,
	CMD_GET_SYS_INFO,
	CMD_GET_MEM_INFO,
	CMD_GET_MEM_CAL_INFO,
	CMD_TRIG_CONTROLLER_OP,
	CMD_TRIG_MEM_CAL_OP
};

/* supported mailbox command opcode */
enum iossm_mailbox_cmd_opcode  {
	GET_MEM_INTF_INFO	     = 0x0001,
	GET_MEM_TECHNOLOGY,
	GET_MEMCLK_FREQ_KHZ,
	GET_MEM_WIDTH_INFO,
	ECC_ENABLE_SET		     = 0x0101,
	ECC_ENABLE_STATUS,
	ECC_INTERRUPT_STATUS,
	ECC_INTERRUPT_ACK,
	ECC_INTERRUPT_MASK,
	ECC_WRITEBACK_ENABLE,
	ECC_SCRUB_IN_PROGRESS_STATUS = 0x0201,
	ECC_SCRUB_MODE_0_START,
	ECC_SCRUB_MODE_1_START,
	BIST_STANDARD_MODE_START     = 0x0301,
	BIST_RESULTS_STATUS,
	BIST_MEM_INIT_START,
	BIST_MEM_INIT_STATUS,
	BIST_SET_DATA_PATTERN_UPPER,
	BIST_SET_DATA_PATTERN_LOWER,
	TRIG_MEM_CAL		     = 0x000a,
	GET_MEM_CAL_STATUS
};

/* response data of cmd opcode GET_MEM_CAL_STATUS */
#define INTF_UNUSED			0x0
#define INTF_MEM_CAL_STATUS_SUCCESS	0x1
#define INTF_MEM_CAL_STATUS_FAIL	0x2
#define INTF_MEM_CAL_STATUS_ONGOING	0x4

/*
 * IOSSM mailbox required information
 *
 * @num_mem_interface:	Number of memory interfaces instantiated
 * @ip_type:		IP type implemented on the IO96B
 * @ip_instance_id:	IP identifier for every IP instance implemented on the IO96B
 */
struct io96b_mb_ctrl {
	u32 num_mem_interface;
	u32 ip_type[2];
	u32 ip_instance_id[2];
};

/*
 * IOSSM mailbox response outputs
 *
 * @cmd_resp_status: Command Interface status
 * @cmd_resp_data_*: More spaces for command response
 */
struct io96b_mb_resp {
	u32 cmd_resp_status;
	u32 cmd_resp_data[3];
};

/*
 * IO96B instance specific information
 *
 * @size:		Memory size
 * @io96b_csr_addr:	IO96B instance CSR address
 * @cal_status:		IO96B instance calibration status
 * @mb_ctrl:		IOSSM mailbox required information
 */
struct io96b_instance {
	u16 size;
	phys_addr_t io96b_csr_addr;
	bool cal_status;
	struct io96b_mb_ctrl mb_ctrl;
};

/*
 * Overall IO96B instance(s) information
 *
 * @num_instance:	Number of instance(s) assigned to HPS
 * @overall_cal_status: Overall calibration status for all IO96B instance(s)
 * @ddr_type:		DDR memory type
 * @ecc_status:		ECC enable status (false = disabled, true = enabled)
 * @overall_size:	Total DDR memory size
 * @io96b[]:		IO96B instance specific information
 * @ckgen_lock:		IO96B GEN PLL lock (false = not locked, true = locked)
 * @num_port:		Number of IO96B port.
 */
struct io96b_info {
	u8			 num_instance;
	bool			 overall_cal_status;
	const char		*ddr_type;
	bool			 ecc_status;
	u16			 overall_size;
	struct io96b_instance	 io96b[MAX_IO96B_SUPPORTED];
	bool			 ckgen_lock;
	u8			 num_port;
};

int io96b_mb_req(phys_addr_t io96b_csr_addr, u32 ip_type, u32 instance_id,
		 u32 usr_cmd_type, u32 usr_cmd_opcode, u32 cmd_param_0,
		 u32 cmd_param_1, u32 cmd_param_2, u32 cmd_param_3,
		 u32 cmd_param_4, u32 cmd_param_5, u32 cmd_param_6,
		 struct io96b_mb_resp *resp);

static inline int io96b_mb_req_no_param(phys_addr_t io96b_csr_addr, u32 ip_type,
					u32 instance_id, u32 usr_cmd_type,
					u32 usr_cmd_opcode, struct io96b_mb_resp *resp)
{
	return io96b_mb_req(io96b_csr_addr, ip_type, instance_id, usr_cmd_type,
			    usr_cmd_opcode, 0, 0, 0, 0, 0, 0, 0, resp);
}

/* Supported IOSSM mailbox function */
void io96b_mb_init(struct io96b_info *io96b_ctrl);
int io96b_cal_status(phys_addr_t addr);
void io96b_init_mem_cal(struct io96b_info *io96b_ctrl);
int io96b_trig_mem_cal(struct io96b_info *io96b_ctrl);
int io96b_get_mem_technology(struct io96b_info *io96b_ctrl);
int io96b_get_mem_width_info(struct io96b_info *io96b_ctrl);
int io96b_ecc_enable_status(struct io96b_info *io96b_ctrl);
int io96b_bist_mem_init_start(struct io96b_info *io96b_ctrl);

#endif // IOSSM_MAILBOX_H_
