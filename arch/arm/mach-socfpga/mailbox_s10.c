// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2017-2023 Intel Corporation <www.intel.com>
 *
 */

#define pr_fmt(fmt) "mailbox_s10: " fmt

#include <common.h>
#include <io.h>
#include <mach/socfpga/mailbox_s10.h>
#include <mach/socfpga/soc64-regs.h>
#include <mach/socfpga/soc64-system-manager.h>

#define MBOX_READL(reg)			\
	 readl(SOCFPGA_MAILBOX_ADDRESS + (reg))

#define MBOX_WRITEL(data, reg)		\
	writel(data, SOCFPGA_MAILBOX_ADDRESS + (reg))

#define MBOX_READ_RESP_BUF(rout)	\
	MBOX_READL(MBOX_RESP_BUF + ((rout) * sizeof(u32)))

#define MBOX_WRITE_CMD_BUF(data, cin)	\
	MBOX_WRITEL(data, MBOX_CMD_BUF + ((cin) * sizeof(u32)))

static int mbox_polling_resp(u32 rout)
{
	u32 rin;
	unsigned long i = 2000;

	while (i) {
		rin = MBOX_READL(MBOX_RIN);
		if (rout != rin)
			return 0;

		udelay(1000);
		i--;
	}

	return -ETIMEDOUT;
}

static int mbox_is_cmdbuf_full(u32 cin)
{
	return (((cin + 1) % MBOX_CMD_BUFFER_SIZE) == MBOX_READL(MBOX_COUT));
}

static int mbox_write_cmd_buffer(u32 *cin, u32 data)
{
	int timeout = 1000;
	int is_cmdbuf_overflow = 0;

	while (timeout) {
		if (mbox_is_cmdbuf_full(*cin)) {
			if (is_cmdbuf_overflow == 0) {
				/* Trigger SDM doorbell */
				MBOX_WRITEL(1, MBOX_DOORBELL_TO_SDM);
				is_cmdbuf_overflow = 1;
			}
			udelay(1000);
		} else {
			/* write header to circular buffer */
			MBOX_WRITE_CMD_BUF(data, (*cin)++);
			*cin %= MBOX_CMD_BUFFER_SIZE;
			MBOX_WRITEL(*cin, MBOX_CIN);
			if (is_cmdbuf_overflow)
				is_cmdbuf_overflow = 0;
			break;
		}
		timeout--;
	}

	if (!timeout)
		return -ETIMEDOUT;

	return 0;
}

/* Check for available slot and write to circular buffer.
 * It also update command valid offset (cin) register.
 */
static int mbox_fill_cmd_circular_buff(u32 header, u32 len,
				       u32 *arg)
{
	int i, ret;
	u32 cin = MBOX_READL(MBOX_CIN) % MBOX_CMD_BUFFER_SIZE;

	ret = mbox_write_cmd_buffer(&cin, header);
	if (ret)
		return ret;

	/* write arguments */
	for (i = 0; i < len; i++) {
		ret = mbox_write_cmd_buffer(&cin, arg[i]);
		if (ret)
			return ret;
	}

	/* Always trigger the SDM doorbell at the end to ensure SDM able to read
	 * the remaining data.
	 */
	MBOX_WRITEL(1, MBOX_DOORBELL_TO_SDM);

	return 0;
}

/* Check the command and fill it into circular buffer */
static int mbox_prepare_cmd_only(u8 id, u32 cmd,
				 u8 is_indirect, u32 len,
				 u32 *arg)
{
	u32 header;
	int ret;

	if (cmd > MBOX_MAX_CMD_INDEX)
		return -EINVAL;

	header = MBOX_CMD_HEADER(MBOX_CLIENT_ID_BAREBOX, id, len,
				 (is_indirect) ? 1 : 0, cmd);

	ret = mbox_fill_cmd_circular_buff(header, len, arg);

	return ret;
}

/* Support one command and up to 31 words argument length only */
static int mbox_send_cmd_common(u8 id, u32 cmd, u8 is_indirect,
				u32 len, u32 *arg, u8 urgent,
				u32 *resp_buf_len,
				u32 *resp_buf)
{
	u32 rin;
	u32 resp;
	u32 rout;
	u32 status;
	u32 resp_len;
	u32 buf_len;
	int ret;

	if (urgent) {
		/* Read status because it is toggled */
		status = MBOX_READL(MBOX_STATUS) & MBOX_STATUS_UA_MSK;
		/* Write urgent command to urgent register */
		MBOX_WRITEL(cmd, MBOX_URG);
		/* write doorbell */
		MBOX_WRITEL(1, MBOX_DOORBELL_TO_SDM);
	} else {
		ret = mbox_prepare_cmd_only(id, cmd, is_indirect, len, arg);
		if (ret)
			return ret;
	}

	while (1) {
		ret = 1000;

		/* Wait for doorbell from SDM */
		do {
			if (MBOX_READL(MBOX_DOORBELL_FROM_SDM))
				break;
			udelay(1000);
		} while (--ret);

		if (!ret)
			return -ETIMEDOUT;

		/* clear interrupt */
		MBOX_WRITEL(0, MBOX_DOORBELL_FROM_SDM);

		if (urgent) {
			u32 new_status = MBOX_READL(MBOX_STATUS);

			/* Urgent ACK is toggled */
			if ((new_status & MBOX_STATUS_UA_MSK) ^ status)
				return 0;

			return -ECOMM;
		}

		/* read current response offset */
		rout = MBOX_READL(MBOX_ROUT);

		/* read response valid offset */
		rin = MBOX_READL(MBOX_RIN);

		if (rout != rin) {
			/* Response received */
			resp = MBOX_READ_RESP_BUF(rout);
			rout++;
			/* wrapping around when it reach the buffer size */
			rout %= MBOX_RESP_BUFFER_SIZE;
			/* update next ROUT */
			MBOX_WRITEL(rout, MBOX_ROUT);

			/* check client ID and ID */
			if ((MBOX_RESP_CLIENT_GET(resp) ==
			     MBOX_CLIENT_ID_BAREBOX) &&
			    (MBOX_RESP_ID_GET(resp) == id)) {
				int resp_err = MBOX_RESP_ERR_GET(resp);

				if (resp_buf_len) {
					buf_len = *resp_buf_len;
					*resp_buf_len = 0;
				} else {
					buf_len = 0;
				}

				resp_len = MBOX_RESP_LEN_GET(resp);
				while (resp_len) {
					ret = mbox_polling_resp(rout);
					if (ret)
						return ret;
					/* we need to process response buffer
					 * even caller doesn't need it
					 */
					resp = MBOX_READ_RESP_BUF(rout);
					rout++;
					resp_len--;
					rout %= MBOX_RESP_BUFFER_SIZE;
					MBOX_WRITEL(rout, MBOX_ROUT);
					if (buf_len) {
						/* copy response to buffer */
						resp_buf[*resp_buf_len] = resp;
						(*resp_buf_len)++;
						buf_len--;
					}
				}
				return resp_err;
			}
		}
	}

	return -EIO;
}

static int mbox_send_cmd_common_retry(u8 id, u32 cmd,
				      u8 is_indirect,
				      u32 len, u32 *arg,
				      u8 urgent,
				      u32 *resp_buf_len,
				      u32 *resp_buf)
{
	int ret;
	int i;

	for (i = 0; i < 3; i++) {
		ret = mbox_send_cmd_common(id, cmd, is_indirect, len, arg,
					   urgent, resp_buf_len, resp_buf);
		if (ret == MBOX_RESP_TIMEOUT || ret == MBOX_RESP_DEVICE_BUSY)
			udelay(2000); /* wait for 2ms before resend */
		else
			break;
	}

	return ret;
}

static int mbox_send_cmd(u8 id, u32 cmd, u8 is_indirect, u32 len, u32 *arg,
			 u8 urgent, u32 *resp_buf_len, u32 *resp_buf)
{
	return mbox_send_cmd_common_retry(id, cmd, is_indirect, len, arg,
					  urgent, resp_buf_len, resp_buf);
}

int socfpga_mailbox_s10_init(void)
{
	int ret;

	/* enable mailbox interrupts */
	MBOX_WRITEL(MBOX_ALL_INTRS, MBOX_FLAGS);

	/* Ensure urgent request is cleared */
	MBOX_WRITEL(0, MBOX_URG);

	/* Ensure the Doorbell Interrupt is cleared */
	MBOX_WRITEL(0, MBOX_DOORBELL_FROM_SDM);

	ret = mbox_send_cmd(MBOX_ID_BAREBOX, MBOX_RESTART, MBOX_CMD_DIRECT, 0,
			    NULL, 1, 0, NULL);
	if (ret)
		return ret;

	pr_debug("%s: success...\n", __func__);

	/* Renable mailbox interrupts after MBOX_RESTART */
	MBOX_WRITEL(MBOX_ALL_INTRS, MBOX_FLAGS);

	return 0;
}

int socfpga_mailbox_s10_qspi_close(void)
{
	return mbox_send_cmd(MBOX_ID_BAREBOX, MBOX_QSPI_CLOSE, MBOX_CMD_DIRECT,
			     0, NULL, 0, 0, NULL);
}

int socfpga_mailbox_s10_qspi_open(void)
{
	int ret;
	u32 resp_buf[1];
	u32 resp_buf_len;
	u32 reg;
	u32 clk_khz;

	ret = mbox_send_cmd(MBOX_ID_BAREBOX, MBOX_QSPI_OPEN, MBOX_CMD_DIRECT,
			    0, NULL, 0, 0, NULL);
	if (ret) {
		/* retry again by closing and reopen the QSPI again */
		ret = socfpga_mailbox_s10_qspi_close();
		if (ret)
			return ret;

		ret = mbox_send_cmd(MBOX_ID_BAREBOX, MBOX_QSPI_OPEN,
				    MBOX_CMD_DIRECT, 0, NULL, 0, 0, NULL);
		if (ret)
			return ret;
	}

	/* HPS will directly control the QSPI controller, no longer mailbox */
	resp_buf_len = 1;
	ret = mbox_send_cmd(MBOX_ID_BAREBOX, MBOX_QSPI_DIRECT, MBOX_CMD_DIRECT,
			    0, NULL, 0, (u32 *)&resp_buf_len,
			    (u32 *)&resp_buf);
	if (ret)
		goto error;

	/* Get the QSPI clock from SDM response and save for later use */
	clk_khz = resp_buf[0];
	if (clk_khz < 1000)
		return -EINVAL;

	clk_khz /= 1000;
	pr_info("QSPI: reference clock at %d kHZ\n", clk_khz);

	reg = (readl(SOCFPGA_SYSMGR_ADDRESS + SYSMGR_SOC64_BOOT_SCRATCH_COLD0)) &
			~(SYSMGR_SCRATCH_REG_0_QSPI_REFCLK_MASK);

	writel((clk_khz & SYSMGR_SCRATCH_REG_0_QSPI_REFCLK_MASK) | reg,
	       SOCFPGA_SYSMGR_ADDRESS + SYSMGR_SOC64_BOOT_SCRATCH_COLD0);

	return 0;

error:
	socfpga_mailbox_s10_qspi_close();

	return ret;
}
