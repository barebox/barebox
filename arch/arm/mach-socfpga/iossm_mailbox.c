// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022-2024 Intel Corporation <www.intel.com>
 *
 */

#define pr_fmt(fmt) "iossm_mailbox: " fmt

#include <common.h>
#include <io.h>
#include <linux/bitfield.h>
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

/* supported DDR type list */
static const char *ddr_type_list[7] = {
		"DDR4", "DDR5", "DDR5_RDIMM", "LPDDR4", "LPDDR5", "QDRIV", "UNKNOWN"
};

static int wait_for_timeout(const void __iomem *reg, u32 mask, bool set)
{
	int timeout = 1000000;
	int val;

	while (timeout > 0) {
		val = readl(IOMEM(reg));
		if (!set)
			val = ~val;

		if ((val & mask) == mask)
			return 0;
		__udelay(10);
		timeout--;
	}

	return -ETIMEDOUT;
}

static int is_ddr_csr_clkgen_locked(u32 clkgen_mask, u8 num_port)
{
	int ret;

	ret = wait_for_timeout(IOMEM(ECC_INTSTATUS_SERR), clkgen_mask, true);
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
int io96b_mb_req(phys_addr_t io96b_csr_addr, u32 ip_type, u32 instance_id,
		 u32 usr_cmd_type, u32 usr_cmd_opcode, u32 cmd_param_0,
		 u32 cmd_param_1, u32 cmd_param_2, u32 cmd_param_3,
		 u32 cmd_param_4, u32 cmd_param_5, u32 cmd_param_6,
		 struct io96b_mb_resp *resp)
{
	int ret;
	u32 cmd_req, cmd_resp;

	/* Initialized zeros for responses*/
	memset(resp, 0x0, sizeof(*resp));

	/* Ensure CMD_REQ is cleared before write any command request */
	ret = wait_for_timeout((IOMEM(io96b_csr_addr) + IOSSM_CMD_REQ_OFFSET), GENMASK(31, 0), false);
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
	pr_debug("%s: Write 0x%x to IOSSM_CMD_REQ_OFFSET 0x%llx\n", __func__,
		 cmd_req, io96b_csr_addr + IOSSM_CMD_REQ_OFFSET);

	/* Read CMD_RESPONSE_READY in CMD_RESPONSE_STATUS*/
	ret = wait_for_timeout((IOMEM(io96b_csr_addr) + IOSSM_CMD_RESPONSE_STATUS_OFFSET),
			       IOSSM_STATUS_COMMAND_RESPONSE_READY, true);
	if (ret) {
		pr_err("%s: CMD_RESPONSE ERROR:\n", __func__);
		cmd_resp = readl(io96b_csr_addr + IOSSM_CMD_RESPONSE_STATUS_OFFSET);
		pr_err("%s: STATUS_GENERAL_ERROR: 0x%x\n", __func__, (cmd_resp >> 1) & 0xF);
		pr_err("%s: STATUS_CMD_RESPONSE_ERROR: 0x%x\n", __func__, (cmd_resp >> 5) & 0x7);
	}

	/* read CMD_RESPONSE_STATUS*/
	resp->cmd_resp_status = readl(io96b_csr_addr + IOSSM_CMD_RESPONSE_STATUS_OFFSET);
	pr_debug("%s: CMD_RESPONSE_STATUS 0x%llx: 0x%x\n", __func__, io96b_csr_addr +
		IOSSM_CMD_RESPONSE_STATUS_OFFSET, resp->cmd_resp_status);

	/* read CMD_RESPONSE_DATA_* */
	resp->cmd_resp_data[0] = readl(io96b_csr_addr + IOSSM_CMD_RESPONSE_DATA_0_OFFSET);
	pr_debug("%s: IOSSM_CMD_RESPONSE_DATA_0_OFFSET 0x%llx: 0x%x\n",
		 __func__, io96b_csr_addr + IOSSM_CMD_RESPONSE_DATA_0_OFFSET,
		 resp->cmd_resp_data[0]);
	resp->cmd_resp_data[1] = readl(io96b_csr_addr + IOSSM_CMD_RESPONSE_DATA_1_OFFSET);
	pr_debug("%s: IOSSM_CMD_RESPONSE_DATA_1_OFFSET 0x%llx: 0x%x\n",
		 __func__, io96b_csr_addr + IOSSM_CMD_RESPONSE_DATA_1_OFFSET,
		 resp->cmd_resp_data[1]);
	resp->cmd_resp_data[2] = readl(io96b_csr_addr + IOSSM_CMD_RESPONSE_DATA_2_OFFSET);
	pr_debug("%s: IOSSM_CMD_RESPONSE_DATA_2_OFFSET 0x%llx: 0x%x\n",
		 __func__, io96b_csr_addr + IOSSM_CMD_RESPONSE_DATA_2_OFFSET,
		 resp->cmd_resp_data[2]);

	resp->cmd_resp_status = readl(io96b_csr_addr + IOSSM_CMD_RESPONSE_STATUS_OFFSET);
	pr_debug("%s: CMD_RESPONSE_STATUS 0x%llx: 0x%x\n", __func__,
		 io96b_csr_addr + IOSSM_CMD_RESPONSE_STATUS_OFFSET,
		 resp->cmd_resp_status);

	/* write CMD_RESPONSE_READY = 0 */
	clrbits_le32((u32 *)(uintptr_t)(io96b_csr_addr + IOSSM_CMD_RESPONSE_STATUS_OFFSET),
		     IOSSM_STATUS_COMMAND_RESPONSE_READY);

	resp->cmd_resp_status = readl(io96b_csr_addr + IOSSM_CMD_RESPONSE_STATUS_OFFSET);
	pr_debug("%s: CMD_RESPONSE_READY 0x%llx: 0x%x\n", __func__, io96b_csr_addr +
		 IOSSM_CMD_RESPONSE_STATUS_OFFSET, resp->cmd_resp_status);

	return 0;
}

static int io96b_mb_version(struct io96b_info *io96b_ctrl)
{
	phys_addr_t io96b_csr_addr = io96b_ctrl->io96b[0].io96b_csr_addr;
	u32 mailbox_header;
	int version;

	mailbox_header = readl(io96b_csr_addr + IOSSM_MAILBOX_HEADER_OFFSET);
	version = FIELD_GET(IOSSM_MAILBOX_SPEC_VERSION_MASK, mailbox_header);

	return version;
}

/*
 * Initial function to be called to set memory interface IP type and instance ID
 * IP type and instance ID need to be determined before sending mailbox command
 */
void io96b_mb_init(struct io96b_info *io96b_ctrl)
{
	struct io96b_mb_resp usr_resp;
	struct io96b_mb_ctrl *mb_ctrl;
	u8 ip_type_ret, instance_id_ret;
	int i, j, k;
	int version;

	version = io96b_mb_version(io96b_ctrl);
	switch (version) {
	case 0:
		pr_debug("IOSSM: mailbox version %d\n", version);
		break;
	default:
		pr_warn("IOSSM: unsupported mailbox version %d\n", version);
		break;
	}
	io96b_ctrl->version = version;

	pr_debug("%s: num_instance %d\n", __func__, io96b_ctrl->num_instance);
	for (i = 0; i < io96b_ctrl->num_instance; i++) {
		int num_mem_interface = 0;
		u32 mem_intf_info[2];

		mb_ctrl = &io96b_ctrl->io96b[i].mb_ctrl;
		pr_debug("%s: get memory interface IO96B %d\n", __func__, i);
		/* Get memory interface IP type and instance ID (IP identifier) */
		io96b_mb_req_no_param(io96b_ctrl->io96b[i].io96b_csr_addr, 0, 0,
				      CMD_GET_SYS_INFO, GET_MEM_INTF_INFO, &usr_resp);
		num_mem_interface =
			IOSSM_CMD_RESPONSE_DATA_SHORT(usr_resp.cmd_resp_status) & 0x3;
		for (j = 0; j < num_mem_interface; j++)
			mem_intf_info[j] = usr_resp.cmd_resp_data[j];

		/* Retrieve memory interface IP type and instance ID (IP identifier) */
		j = 0;
		for (k = 0; k < num_mem_interface; k++) {
			ip_type_ret = FIELD_GET(INTF_IP_TYPE_MASK, mem_intf_info[k]);
			instance_id_ret = FIELD_GET(INTF_INSTANCE_ID_MASK, mem_intf_info[k]);

			if (ip_type_ret) {
				mb_ctrl->num_mem_interface++;
				mb_ctrl->ip_type[j] = ip_type_ret;
				mb_ctrl->ip_instance_id[j] = instance_id_ret;
				pr_debug("%s: IO96B %d mem_interface %d: ip_type_ret: 0x%x\n",
					 __func__, i, j, ip_type_ret);
				pr_debug("%s: IO96B %d mem_interface %d: instance_id_ret: 0x%x\n",
					 __func__, i, j, instance_id_ret);
				j++;
			}
		}
	}
}

int io96b_cal_status(phys_addr_t addr)
{
	int ret;
	u32 cal_success, cal_fail;
	phys_addr_t status_addr = addr + IOSSM_STATUS_OFFSET;
	/* Ensure calibration completed */
	ret = wait_for_timeout(IOMEM(status_addr), IOSSM_STATUS_CAL_BUSY, false);
	if (ret) {
		pr_err("%s: SDRAM calibration IO96b instance 0x%llx timeout\n",
		       __func__, status_addr);
		hang();
	}

	/* Calibration status */
	cal_success = readl(status_addr) & IOSSM_STATUS_CAL_SUCCESS;
	cal_fail = readl(status_addr) & IOSSM_STATUS_CAL_FAIL;

	if (cal_success && !cal_fail)
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

	/* Initialize ddr type */
	io96b_ctrl->ddr_type = ddr_type_list[6];

	/* Get and ensure all memory interface(s) same DDR type */
	for (i = 0; i < io96b_ctrl->num_instance; i++) {
		mb_ctrl = &io96b_ctrl->io96b[i].mb_ctrl;
		for (j = 0; j < mb_ctrl->num_mem_interface; j++) {
			io96b_mb_req_no_param(io96b_ctrl->io96b[i].io96b_csr_addr,
					      mb_ctrl->ip_type[j],
					      mb_ctrl->ip_instance_id[j],
					      CMD_GET_MEM_INFO, GET_MEM_TECHNOLOGY, &usr_resp);

			mem_technology_intf = IOSSM_CMD_RESPONSE_DATA_SHORT(usr_resp.cmd_resp_status);

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

	/* Get all memory interface(s) total memory size on all instance(s) */
	for (i = 0; i < io96b_ctrl->num_instance; i++) {
		mb_ctrl = &io96b_ctrl->io96b[i].mb_ctrl;
		memory_size = 0;

		for (j = 0; j < mb_ctrl->num_mem_interface; j++) {
			io96b_mb_req_no_param(io96b_ctrl->io96b[i].io96b_csr_addr,
					      mb_ctrl->ip_type[j],
					      mb_ctrl->ip_instance_id[j],
					      CMD_GET_MEM_INFO, GET_MEM_WIDTH_INFO, &usr_resp);
			mem_width_info = usr_resp.cmd_resp_data[1] & GENMASK(7, 0);

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

	/* Initialize ECC status */
	io96b_ctrl->ecc_status = false;

	/* Get and ensure all memory interface(s) same ECC status */
	for (i = 0; i < io96b_ctrl->num_instance; i++) {
		mb_ctrl = &io96b_ctrl->io96b[i].mb_ctrl;
		for (j = 0; j < mb_ctrl->num_mem_interface; j++) {
			io96b_mb_req_no_param(io96b_ctrl->io96b[i].io96b_csr_addr,
					      mb_ctrl->ip_type[j],
					      mb_ctrl->ip_instance_id[j],
					      CMD_TRIG_CONTROLLER_OP, ECC_ENABLE_STATUS,
					      &usr_resp);
			ecc_enable_intf = IOSSM_CMD_RESPONSE_DATA_SHORT(usr_resp.cmd_resp_status);

			ecc_stat = (ecc_enable_intf & GENMASK(1, 0)) == 0 ? false : true;

			if (!ecc_stat_set) {
				io96b_ctrl->ecc_status = ecc_stat;
				ecc_stat_set = true;
			}

			if (ecc_stat != io96b_ctrl->ecc_status) {
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
	phys_addr_t io96b_csr_addr = io96b_ctrl->io96b[instance].io96b_csr_addr;
	struct io96b_mb_ctrl *mb_ctrl = &io96b_ctrl->io96b[instance].mb_ctrl;
	int timeout = 1 * USEC_PER_SEC;
	bool bist_success = false;
	int bist_error = 0;
	struct io96b_mb_resp usr_resp;
	u32 mem_init_status;

	/* Polling for the initiated memory initialization BIST status */
	while (!bist_success) {
		io96b_mb_req_no_param(io96b_csr_addr,
				      mb_ctrl->ip_type[interface],
				      mb_ctrl->ip_instance_id[interface],
				      CMD_TRIG_CONTROLLER_OP,
				      BIST_MEM_INIT_STATUS, &usr_resp);
		mem_init_status = IOSSM_CMD_RESPONSE_DATA_SHORT(usr_resp.cmd_resp_status);

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

int io96b_bist_mem_init_start(struct io96b_info *io96b_ctrl)
{
	struct io96b_mb_resp usr_resp;
	int i, j;
	bool bist_start;
	u32 mem_init_status_intf;
	int ret = 0;

	/* Full memory initialization BIST performed on all memory interface(s) */
	for (i = 0; i < io96b_ctrl->num_instance; i++) {
		for (j = 0; j < io96b_ctrl->io96b[i].mb_ctrl.num_mem_interface; j++) {
			bist_start = false;

			/* Start memory initialization BIST on full memory address */
			io96b_mb_req(io96b_ctrl->io96b[i].io96b_csr_addr,
				     io96b_ctrl->io96b[i].mb_ctrl.ip_type[j],
				     io96b_ctrl->io96b[i].mb_ctrl.ip_instance_id[j],
				     CMD_TRIG_CONTROLLER_OP, BIST_MEM_INIT_START,
				     0x40, 0, 0, 0, 0, 0, 0, &usr_resp);
			mem_init_status_intf = IOSSM_CMD_RESPONSE_DATA_SHORT(usr_resp.cmd_resp_status);

			bist_start = mem_init_status_intf & BIT(0);

			if (!bist_start) {
				pr_err("%s: Failed to initialized memory on IO96B_%d\n",
				       __func__, i);
				pr_err("%s: BIST_MEM_INIT_START Error code 0x%x\n",
				       __func__, (mem_init_status_intf & GENMASK(2, 1)) > 0x1);
				return -ENOEXEC;
			}

			ret = io96b_poll_bist_mem_init_status(io96b_ctrl, i, j);
			if (ret)
				return ret;
		}

		pr_debug("%s: Memory initialized successfully on IO96B_%d\n", __func__, i);
	}
	return 0;
}
