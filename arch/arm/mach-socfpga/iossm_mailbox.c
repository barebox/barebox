// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022-2024 Intel Corporation <www.intel.com>
 *
 */

#define pr_fmt(fmt) "iossm_mailbox: " fmt

#include <common.h>
#include <io.h>
#include <linux/bitfield.h>
#include <linux/iopoll.h>
#include <linux/sizes.h>
#include "iossm_mailbox.h"
#include <mach/socfpga/generic.h>
#include <mach/socfpga/soc64-regs.h>
#include <mach/socfpga/soc64-system-manager.h>

#define ECC_INTSTATUS_SERR SOCFPGA_SYSMGR_ADDRESS + 0x9C
#define ECC_INISTATUS_DERR SOCFPGA_SYSMGR_ADDRESS + 0xA0
#define DDR_CSR_CLKGEN_LOCKED_IO96B0_MASK BIT(16)
#define DDR_CSR_CLKGEN_LOCKED_IO96B1_MASK BIT(17)

#define DDR_CSR_CLKGEN_LOCKED_IO96B_MASK(x)	(i == 0 ? DDR_CSR_CLKGEN_LOCKED_IO96B0_MASK : \
							DDR_CSR_CLKGEN_LOCKED_IO96B1_MASK)
#define MAX_RETRY_COUNT 3
#define NUM_CMD_RESPONSE_DATA 3

#define INTF_IP_TYPE_MASK	GENMASK(31, 29)
#define INTF_INSTANCE_ID_MASK	GENMASK(28, 24)

#define IOSSM_MAILBOX_HEADER_OFFSET			0x0
#define IOSSM_MAILBOX_SPEC_VERSION_MASK			GENMASK(2, 0)
#define IOSSM_MEM_INTF_INFO_0_OFFSET			0x200
#define IOSSM_MEM_TECHNOLOGY_INTF0_OFFSET               0x210
#define IOSSM_MEM_WIDTH_INFO_INTF0_OFFSET               0x230
#define IOSSM_MEM_TOTAL_CAPACITY_INTF0_OFFSET           0x234
#define IOSSM_MEM_INTF_INFO_1_OFFSET			0x280
#define IOSSM_MEM_TECHNOLOGY_INTF1_OFFSET               0x290
#define IOSSM_MEM_TOTAL_CAPACITY_INTF1_OFFSET           0x2B4
#define IOSSM_MEM_WIDTH_INFO_INTF1_OFFSET               0x2B0
#define IOSSM_ECC_ENABLE_INTF0_OFFSET			0x240
#define IOSSM_ECC_ENABLE_INTF1_OFFSET			0x2C0
#define IOSSM_MEM_INIT_STATUS_INTF0_OFFSET		0x260
#define IOSSM_MEM_INIT_STATUS_INTF1_OFFSET		0x2E0

/* supported DDR type list */
static const char *ddr_type_list[7] = {
		"DDR4", "DDR5", "DDR5_RDIMM", "LPDDR4", "LPDDR5", "QDRIV", "UNKNOWN"
};

static int is_ddr_csr_clkgen_locked(u32 clkgen_mask, u8 num_port)
{
	int ret;
	u32 tmp;

	ret = readl_poll_timeout(IOMEM(ECC_INTSTATUS_SERR),
				 tmp, tmp & clkgen_mask, 10 * USEC_PER_SEC);
	if (ret) {
		pr_debug("%s: ddr csr clkgena locked is timeout\n", __func__);
		return ret;
	}

	return 0;
}

/* Mailbox request function
 * This function will send the request to IOSSM mailbox and wait for response return
 *
 * @io96b_csr_addr: CSR address for the target IO96B
 * @ip_type:	    IP type for the specified memory interface
 * @instance_id:    IP instance ID for the specified memory interface
 * @usr_cmd_type:   User desire IOSSM mailbox command type
 * @usr_cmd_opcode: User desire IOSSM mailbox command opcode
 * @cmd_param_*:    Parameters (if applicable) for the requested IOSSM mailbox command
 * @resp:			Structure contain responses returned from the requested IOSSM
 *					mailbox command
 */
int io96b_mb_req(void __iomem *io96b_csr_addr, u32 ip_type, u32 instance_id,
		 u32 usr_cmd_type, u32 usr_cmd_opcode, u32 cmd_param_0,
		 u32 cmd_param_1, u32 cmd_param_2, u32 cmd_param_3,
		 u32 cmd_param_4, u32 cmd_param_5, u32 cmd_param_6,
		 struct io96b_mb_resp *resp)
{
	int ret;
	u32 cmd_req;

	/* Initialized zeros for responses*/
	memset(resp, 0x0, sizeof(*resp));

	/* Ensure CMD_REQ is cleared before write any command request */
	ret = readl_poll_timeout(io96b_csr_addr + IOSSM_CMD_REQ_OFFSET,
				 cmd_req, !(cmd_req & GENMASK(31, 0)),
				 10 * USEC_PER_SEC);
	if (ret) {
		pr_err("%s: CMD_REQ not ready\n", __func__);
		return -1;
	}

	/* Write CMD_PARAM_* */
	if (cmd_param_0)
		writel(cmd_param_0, io96b_csr_addr + IOSSM_CMD_PARAM_0_OFFSET);
	if (cmd_param_1)
		writel(cmd_param_1, io96b_csr_addr + IOSSM_CMD_PARAM_1_OFFSET);
	if (cmd_param_2)
		writel(cmd_param_2, io96b_csr_addr + IOSSM_CMD_PARAM_2_OFFSET);
	if (cmd_param_3)
		writel(cmd_param_3, io96b_csr_addr + IOSSM_CMD_PARAM_3_OFFSET);
	if (cmd_param_4)
		writel(cmd_param_4, io96b_csr_addr + IOSSM_CMD_PARAM_4_OFFSET);
	if (cmd_param_5)
		writel(cmd_param_5, io96b_csr_addr + IOSSM_CMD_PARAM_5_OFFSET);
	if (cmd_param_6)
		writel(cmd_param_6, io96b_csr_addr + IOSSM_CMD_PARAM_6_OFFSET);

	/* Write CMD_REQ (IP_TYPE, IP_INSTANCE_ID, CMD_TYPE and CMD_OPCODE) */
	cmd_req = (usr_cmd_opcode << 0) | (usr_cmd_type << 16) | (instance_id << 24) |
		(ip_type << 29);
	writel(cmd_req, io96b_csr_addr + IOSSM_CMD_REQ_OFFSET);
	pr_debug("%s: Write 0x%x to IOSSM_CMD_REQ_OFFSET 0x%p\n", __func__,
		 cmd_req, io96b_csr_addr + IOSSM_CMD_REQ_OFFSET);

	/* Read CMD_RESPONSE_READY in CMD_RESPONSE_STATUS*/
	ret = readl_poll_timeout(io96b_csr_addr + IOSSM_CMD_RESPONSE_STATUS_OFFSET,
				 resp->cmd_resp_status,
				 resp->cmd_resp_status & IOSSM_STATUS_COMMAND_RESPONSE_READY,
				 10 * USEC_PER_SEC);
	if (ret)
		pr_warn("%s: CMD_RESPONSE_STATUS ERROR: 0x%lx 0x%lx\n", __func__,
			FIELD_GET(GENMASK(4, 1), resp->cmd_resp_status),
			FIELD_GET(GENMASK(7, 5), resp->cmd_resp_status));
	pr_debug("%s: CMD_RESPONSE_STATUS 0x%p: 0x%x\n", __func__,
		 io96b_csr_addr + IOSSM_CMD_RESPONSE_STATUS_OFFSET,
		 resp->cmd_resp_status);

	/* read CMD_RESPONSE_DATA_* */
	resp->cmd_resp_data[0] = readl(io96b_csr_addr + IOSSM_CMD_RESPONSE_DATA_0_OFFSET);
	pr_debug("%s: IOSSM_CMD_RESPONSE_DATA_0_OFFSET 0x%p: 0x%x\n", __func__,
		 io96b_csr_addr + IOSSM_CMD_RESPONSE_DATA_0_OFFSET,
		 resp->cmd_resp_data[0]);
	resp->cmd_resp_data[1] = readl(io96b_csr_addr + IOSSM_CMD_RESPONSE_DATA_1_OFFSET);
	pr_debug("%s: IOSSM_CMD_RESPONSE_DATA_1_OFFSET 0x%p: 0x%x\n", __func__,
		 io96b_csr_addr + IOSSM_CMD_RESPONSE_DATA_1_OFFSET,
		 resp->cmd_resp_data[1]);
	resp->cmd_resp_data[2] = readl(io96b_csr_addr + IOSSM_CMD_RESPONSE_DATA_2_OFFSET);
	pr_debug("%s: IOSSM_CMD_RESPONSE_DATA_2_OFFSET 0x%p: 0x%x\n", __func__,
		 io96b_csr_addr + IOSSM_CMD_RESPONSE_DATA_2_OFFSET,
		 resp->cmd_resp_data[2]);

	/* write CMD_RESPONSE_READY = 0 */
	clrbits_le32(io96b_csr_addr + IOSSM_CMD_RESPONSE_STATUS_OFFSET,
		     IOSSM_STATUS_COMMAND_RESPONSE_READY);

	return 0;
}

static int io96b_mb_version(struct io96b_info *io96b_ctrl)
{
	void __iomem *io96b_csr_addr = io96b_ctrl->io96b[0].io96b_csr_addr;
	u32 mailbox_header;
	int version;

	mailbox_header = readl(io96b_csr_addr + IOSSM_MAILBOX_HEADER_OFFSET);
	version = FIELD_GET(IOSSM_MAILBOX_SPEC_VERSION_MASK, mailbox_header);

	return version;
}

static void io96b_mb_init_instance(struct io96b_info *io96b_ctrl, int instance)
{
	void __iomem *io96b_csr_addr = io96b_ctrl->io96b[instance].io96b_csr_addr;
	struct io96b_mb_ctrl *mb_ctrl = &io96b_ctrl->io96b[instance].mb_ctrl;
	struct io96b_mb_resp usr_resp;
	u32 mem_intf_info[2];
	int num_mem_interface = ARRAY_SIZE(mem_intf_info);
	int i;

	pr_debug("%s: Initialize IO96B_%d\n", __func__, instance);

	/* Get memory interface IP type and instance ID (IP identifier) */
	if (io96b_ctrl->version == 1) {
		mem_intf_info[0] = readl(io96b_csr_addr + IOSSM_MEM_INTF_INFO_0_OFFSET);
		mem_intf_info[1] = readl(io96b_csr_addr + IOSSM_MEM_INTF_INFO_1_OFFSET);
	} else {
		io96b_mb_req_no_param(io96b_csr_addr, 0, 0,
				      CMD_GET_SYS_INFO, GET_MEM_INTF_INFO,
				      &usr_resp);
		num_mem_interface =
			IOSSM_CMD_RESPONSE_DATA_SHORT(usr_resp.cmd_resp_status) & 0x3;
		for (i = 0; i < num_mem_interface; i++)
			mem_intf_info[i] = usr_resp.cmd_resp_data[i];
	}

	/* Retrieve memory interface IP type and instance ID (IP identifier) */
	for (i = 0; i < num_mem_interface; i++) {
		int interface = mb_ctrl->num_mem_interface;
		u8 ip_type;
		u8 instance_id;

		ip_type = FIELD_GET(INTF_IP_TYPE_MASK, mem_intf_info[i]);
		instance_id = FIELD_GET(INTF_INSTANCE_ID_MASK, mem_intf_info[i]);

		if (!ip_type)
			continue;

		mb_ctrl->num_mem_interface++;
		mb_ctrl->ip_type[interface] = ip_type;
		mb_ctrl->ip_instance_id[interface] = instance_id;

		pr_debug("%s: IO96B_%d interface %d: ip_type=0x%x instance_id=0x%x\n",
			 __func__, instance, interface, ip_type, instance_id);
	}
}

/*
 * Initial function to be called to set memory interface IP type and instance ID
 * IP type and instance ID need to be determined before sending mailbox command
 */
void io96b_mb_init(struct io96b_info *io96b_ctrl)
{
	int i;
	int version;

	version = io96b_mb_version(io96b_ctrl);
	switch (version) {
	case 0:
	case 1:
		pr_debug("IOSSM: mailbox version %d\n", version);
		break;
	default:
		pr_warn("IOSSM: unsupported mailbox version %d\n", version);
		break;
	}
	io96b_ctrl->version = version;

	pr_debug("%s: num_instance %d\n", __func__, io96b_ctrl->num_instance);
	for (i = 0; i < io96b_ctrl->num_instance; i++)
		io96b_mb_init_instance(io96b_ctrl, i);
}

static int io96b_cal_status(void __iomem *io96b_csr_addr)
{
	int ret;
	u32 cal_status;

	/* Ensure calibration completed */
	ret = readl_poll_timeout(io96b_csr_addr + IOSSM_STATUS_OFFSET,
				 cal_status,
				 !(cal_status & IOSSM_STATUS_CAL_BUSY),
				 10 * USEC_PER_SEC);
	if (ret) {
		pr_err("%s: SDRAM calibration IO96b instance 0x%p timeout\n",
		       __func__, io96b_csr_addr);
		hang();
	}

	if ((cal_status & IOSSM_STATUS_CAL_SUCCESS) &&
	    !(cal_status & IOSSM_STATUS_CAL_FAIL))
		return 0;
	else
		return -EPERM;
}

void io96b_init_mem_cal(struct io96b_info *io96b_ctrl)
{
	int count, i, ret;

	/* Initialize overall calibration status */
	io96b_ctrl->overall_cal_status = false;

	/* Check initial calibration status for the assigned IO96B*/
	count = 0;
	for (i = 0; i < io96b_ctrl->num_instance; i++) {
		if (io96b_ctrl->ckgen_lock) {
			ret = is_ddr_csr_clkgen_locked(DDR_CSR_CLKGEN_LOCKED_IO96B_MASK(i),
						       io96b_ctrl->num_port);
			if (ret) {
				pr_err("%s: ckgena_lock iossm IO96B_%d is not locked\n",
				       __func__, i);
				hang();
			}
		}
		ret = io96b_cal_status(io96b_ctrl->io96b[i].io96b_csr_addr);
		if (ret) {
			io96b_ctrl->io96b[i].cal_status = false;
			pr_err("%s: Initial DDR calibration IO96B_%d failed %d\n",
			       __func__, i, ret);
			hang();
		}
		io96b_ctrl->io96b[i].cal_status = true;
		pr_info("%s: Initial DDR calibration IO96B_%d succeed\n",
		       __func__, i);
		count++;
	}

	io96b_ctrl->overall_cal_status = true;
}

/*
 * Trying 3 times re-calibration if initial calibration failed
 */
int io96b_trig_mem_cal(struct io96b_info *io96b_ctrl)
{
	struct io96b_mb_resp usr_resp;
	struct io96b_mb_ctrl *mb_ctrl;
	bool recal_success;
	int i, j, k;
	u32 cal_stat_offset;
	u8 cal_stat;
	u8 trig_cal_stat;
	int count = 0;

	for (i = 0; i < io96b_ctrl->num_instance; i++) {
		mb_ctrl = &io96b_ctrl->io96b[i].mb_ctrl;

		if (io96b_ctrl->io96b[i].cal_status)
			continue;

		for (j = 0; j < mb_ctrl->num_mem_interface; j++) {
			recal_success = false;

			/* Re-calibration first memory interface with failed calibration */
			for (k = 0; k < MAX_RETRY_COUNT; k++) {
				/* Get the memory calibration status for memory interface */
				io96b_mb_req_no_param(io96b_ctrl->io96b[i].io96b_csr_addr,
					     0, 0, CMD_TRIG_MEM_CAL_OP,
					     GET_MEM_CAL_STATUS, &usr_resp);

				cal_stat_offset = usr_resp.cmd_resp_data[j];
				cal_stat = readl(io96b_ctrl->io96b[i].io96b_csr_addr +
						 cal_stat_offset);
				if (cal_stat == INTF_MEM_CAL_STATUS_SUCCESS) {
					recal_success = true;
					break;
				}
				io96b_mb_req_no_param(io96b_ctrl->io96b[i].io96b_csr_addr,
						      mb_ctrl->ip_type[j],
						      mb_ctrl->ip_instance_id[j],
						      CMD_TRIG_MEM_CAL_OP,
						      TRIG_MEM_CAL, &usr_resp);

				trig_cal_stat =
					IOSSM_CMD_RESPONSE_DATA_SHORT(usr_resp.cmd_resp_status) &
					BIT(0);
				pr_debug("%s: Memory calibration triggered status = %d\n",
					 __func__, trig_cal_stat);

				udelay(1);
			}

			if (!recal_success) {
				pr_err("%s: Error as SDRAM calibration failed\n", __func__);
				hang();
			}
		}

		io96b_ctrl->io96b[i].cal_status = true;
		io96b_ctrl->overall_cal_status = io96b_ctrl->io96b[i].cal_status;
		pr_info("%s: Initial DDR calibration IO96B_%d succeed\n", __func__, i);
		count++;
	}

	if (io96b_ctrl->overall_cal_status)
		pr_debug("%s: Overall SDRAM calibration success\n", __func__);

	return 0;
}

int io96b_get_mem_technology(struct io96b_info *io96b_ctrl)
{
	struct io96b_mb_resp usr_resp;
	struct io96b_mb_ctrl *mb_ctrl;
	int i, j;
	u32 mem_technology_intf;
	u8 ddr_type_ret;

	u32 mem_technology_intf_offset[] = {
		IOSSM_MEM_TECHNOLOGY_INTF0_OFFSET,
		IOSSM_MEM_TECHNOLOGY_INTF1_OFFSET
	};

	/* Initialize ddr type */
	io96b_ctrl->ddr_type = ddr_type_list[6];

	/* Get and ensure all memory interface(s) same DDR type */
	for (i = 0; i < io96b_ctrl->num_instance; i++) {
		mb_ctrl = &io96b_ctrl->io96b[i].mb_ctrl;
		for (j = 0; j < mb_ctrl->num_mem_interface; j++) {
			if (io96b_ctrl->version == 1) {
				mem_technology_intf = readl(io96b_ctrl->io96b[i].io96b_csr_addr +
							    mem_technology_intf_offset[j]);
			} else {
				io96b_mb_req_no_param(io96b_ctrl->io96b[i].io96b_csr_addr,
						      mb_ctrl->ip_type[j],
						      mb_ctrl->ip_instance_id[j],
						      CMD_GET_MEM_INFO, GET_MEM_TECHNOLOGY, &usr_resp);

				mem_technology_intf = IOSSM_CMD_RESPONSE_DATA_SHORT(usr_resp.cmd_resp_status);
			}

			ddr_type_ret = mem_technology_intf & GENMASK(2, 0);

			if (!strcmp(io96b_ctrl->ddr_type, "UNKNOWN"))
				io96b_ctrl->ddr_type = ddr_type_list[ddr_type_ret];

			if (ddr_type_list[ddr_type_ret] != io96b_ctrl->ddr_type) {
				pr_err("%s: Mismatch DDR type on IO96B_%d\n", __func__, i);
				return -ENOEXEC;
			}
		}
	}

	return 0;
}

int io96b_get_mem_width_info(struct io96b_info *io96b_ctrl)
{
	struct io96b_mb_resp usr_resp;
	struct io96b_mb_ctrl *mb_ctrl;
	int i, j;
	phys_size_t memory_size;
	u32 mem_width_info;
	phys_size_t total_memory_size = 0;

	u32 mem_total_capacity_intf_offset[] = {
		IOSSM_MEM_TOTAL_CAPACITY_INTF0_OFFSET,
		IOSSM_MEM_TOTAL_CAPACITY_INTF1_OFFSET
	};

	/* Get all memory interface(s) total memory size on all instance(s) */
	for (i = 0; i < io96b_ctrl->num_instance; i++) {
		mb_ctrl = &io96b_ctrl->io96b[i].mb_ctrl;
		memory_size = 0;

		for (j = 0; j < mb_ctrl->num_mem_interface; j++) {
			if (io96b_ctrl->version == 1) {
				mem_width_info = readl(io96b_ctrl->io96b[i].io96b_csr_addr +
						       mem_total_capacity_intf_offset[j]);
			} else {
				io96b_mb_req_no_param(io96b_ctrl->io96b[i].io96b_csr_addr,
						      mb_ctrl->ip_type[j],
						      mb_ctrl->ip_instance_id[j],
						      CMD_GET_MEM_INFO, GET_MEM_WIDTH_INFO, &usr_resp);
				mem_width_info = usr_resp.cmd_resp_data[1] & GENMASK(7, 0);
			}

			mb_ctrl->memory_size[j] = (phys_size_t)mem_width_info * (SZ_1G / SZ_8);

			memory_size += mb_ctrl->memory_size[j];
		}

		if (!memory_size) {
			pr_err("%s: Failed to get valid memory size\n", __func__);
			return -ENOEXEC;
		}

		io96b_ctrl->io96b[i].size = memory_size;

		total_memory_size = total_memory_size + memory_size;
	}

	if (!total_memory_size) {
		pr_err("%s: Failed to get valid memory size\n", __func__);
		return -ENOEXEC;
	}

	io96b_ctrl->overall_size = total_memory_size;

	return 0;
}

int io96b_ecc_enable_status(struct io96b_info *io96b_ctrl)
{
	struct io96b_mb_resp usr_resp;
	struct io96b_mb_ctrl *mb_ctrl;
	int i, j;
	u32 ecc_enable_intf;
	bool ecc_stat_set = false;
	bool ecc_stat;
	bool inline_ecc = false;

	u32 ecc_enable_intf_offset[] = {
		IOSSM_ECC_ENABLE_INTF0_OFFSET,
		IOSSM_ECC_ENABLE_INTF1_OFFSET
	};

	/* Initialize ECC status */
	io96b_ctrl->ecc_status = false;

	/* Get and ensure all memory interface(s) same ECC status */
	for (i = 0; i < io96b_ctrl->num_instance; i++) {
		mb_ctrl = &io96b_ctrl->io96b[i].mb_ctrl;
		for (j = 0; j < mb_ctrl->num_mem_interface; j++) {
			if (io96b_ctrl->version == 1) {
				ecc_enable_intf = readl(io96b_ctrl->io96b[i].io96b_csr_addr +
							ecc_enable_intf_offset[j]);
			} else {
				io96b_mb_req_no_param(io96b_ctrl->io96b[i].io96b_csr_addr,
						      mb_ctrl->ip_type[j],
						      mb_ctrl->ip_instance_id[j],
						      CMD_TRIG_CONTROLLER_OP, ECC_ENABLE_STATUS,
						      &usr_resp);
				ecc_enable_intf = IOSSM_CMD_RESPONSE_DATA_SHORT(usr_resp.cmd_resp_status);
			}
			ecc_stat = (ecc_enable_intf & GENMASK(1, 0)) == 0 ? false : true;
			inline_ecc = FIELD_GET(BIT(8), ecc_enable_intf);

			if (!ecc_stat_set) {
				io96b_ctrl->ecc_status = ecc_stat;

				if (io96b_ctrl->ecc_status)
					io96b_ctrl->inline_ecc = inline_ecc;

				ecc_stat_set = true;
			}

			if (ecc_stat != io96b_ctrl->ecc_status ||
			    (io96b_ctrl->ecc_status && inline_ecc != io96b_ctrl->inline_ecc)) {
				pr_err("%s: Mismatch DDR ECC status on IO96B_%d\n",
				       __func__, i);
				return -ENOEXEC;
			}
		}
	}

	pr_debug("%s: ECC enable status: %d\n", __func__, io96b_ctrl->ecc_status);

	return 0;
}

static int io96b_poll_bist_mem_init_status(struct io96b_info *io96b_ctrl,
					   int instance, int interface)
{
	void __iomem *io96b_csr_addr = io96b_ctrl->io96b[instance].io96b_csr_addr;
	struct io96b_mb_ctrl *mb_ctrl = &io96b_ctrl->io96b[instance].mb_ctrl;
	int timeout = 1 * USEC_PER_SEC;
	bool bist_success = false;
	int bist_error = 0;
	struct io96b_mb_resp usr_resp;
	u32 mem_init_status;

	u32 mem_init_status_offset[] = {
		IOSSM_MEM_INIT_STATUS_INTF0_OFFSET,
		IOSSM_MEM_INIT_STATUS_INTF1_OFFSET
	};

	/* Polling for the initiated memory initialization BIST status */
	while (!bist_success) {
		if (io96b_ctrl->version == 1) {
			mem_init_status = readl(io96b_csr_addr + mem_init_status_offset[interface]);
		} else {
			io96b_mb_req_no_param(io96b_csr_addr,
					      mb_ctrl->ip_type[interface],
					      mb_ctrl->ip_instance_id[interface],
					      CMD_TRIG_CONTROLLER_OP,
					      BIST_MEM_INIT_STATUS, &usr_resp);
			mem_init_status = IOSSM_CMD_RESPONSE_DATA_SHORT(usr_resp.cmd_resp_status);
		}

		bist_success = FIELD_GET(BIT(0), mem_init_status);
		bist_error = FIELD_GET(GENMASK(2, 1), mem_init_status);

		if (!bist_success && (timeout-- < 0)) {
			pr_err("%s: Timeout initialize memory on IO96B_%d (Error 0x%x)\n",
			       __func__, instance, bist_error);
			return -ETIMEDOUT;
		}

		__udelay(1);
	}

	return 0;
}

static int bist_mem_init_by_addr(struct io96b_info *io96b_ctrl,
				 int instance, int interface,
				 phys_addr_t base_addr, phys_size_t size)
{
	void __iomem *io96b_csr_addr = io96b_ctrl->io96b[instance].io96b_csr_addr;
	struct io96b_mb_ctrl *mb_ctrl = &io96b_ctrl->io96b[instance].mb_ctrl;
	struct io96b_mb_resp usr_resp;
	bool bist_start = false;
	int bist_error = 0;
	u32 mem_init_status;
	int ret = 0;
#define BIST_ADDR_SPACE		GENMASK(5, 0)
#define BIST_FULL_MEM		BIT(6)
	u32 cmd_param_0;

	if (io96b_ctrl->inline_ecc) {
		phys_size_t chunk_size;
		u32 mem_exp = 0;

		/* Check if size is a power of 2 */
		if (size == 0 || (size & (size - 1)) != 0)
			return -EINVAL;

		chunk_size = size;
		while (chunk_size >>= 1)
			mem_exp++;

		pr_debug("%s: Initializing memory: Addr=0x%llx, Size=2^%u\n",
			 __func__, base_addr, mem_exp);
		cmd_param_0 = FIELD_PREP(BIST_ADDR_SPACE, mem_exp);
	} else {
		pr_debug("%s: Start memory initialization BIST on full memory address",
			 __func__);
		cmd_param_0 = BIST_FULL_MEM;
	}

	ret = io96b_mb_req(io96b_csr_addr,
			   mb_ctrl->ip_type[interface],
			   mb_ctrl->ip_instance_id[interface],
			   CMD_TRIG_CONTROLLER_OP, BIST_MEM_INIT_START,
			   cmd_param_0,
			   FIELD_GET(GENMASK(31, 0), base_addr),
			   FIELD_GET(GENMASK(37, 32), base_addr),
			   0, 0, 0, 0, &usr_resp);
	if (ret)
		return ret;

	mem_init_status = IOSSM_CMD_RESPONSE_DATA_SHORT(usr_resp.cmd_resp_status);

	bist_start = FIELD_GET(BIT(0), mem_init_status);
	bist_error = FIELD_GET(GENMASK(2, 1), mem_init_status);

	if (!bist_start) {
		pr_err("%s: Failed to initialize memory on IO96B_%d (Error 0x%x)\n",
		       __func__, instance, bist_error);
		return -ENOEXEC;
	}

	return io96b_poll_bist_mem_init_status(io96b_ctrl, instance, interface);
}

int io96b_bist_mem_init_start(struct io96b_info *io96b_ctrl)
{
	int i, j;
	int ret = 0;

	/* Memory initialization BIST performed on all memory interface(s) */
	for (i = 0; i < io96b_ctrl->num_instance; i++) {
		for (j = 0; j < io96b_ctrl->io96b[i].mb_ctrl.num_mem_interface; j++) {
			ret = bist_mem_init_by_addr(io96b_ctrl, i, j, 0x0,
						    io96b_ctrl->io96b[i].mb_ctrl.memory_size[j]);
			if (ret) {
				pr_err("%s: Memory init failed at Instance %d, Interface %d\n",
				       __func__, i, j);
				return ret;
			}
		}

		pr_debug("%s: Memory initialized successfully on IO96B_%d\n", __func__, i);
	}

	return 0;
}
