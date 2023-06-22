// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2017, STMicroelectronics - All Rights Reserved
 * Author(s): Patrice Chotard, <patrice.chotard@st.com> for STMicroelectronics.
 */

#include <common.h>
#include <dma.h>
#include <init.h>
#include <linux/amba/bus.h>
#include <linux/iopoll.h>
#include <linux/log2.h>
#include <linux/reset.h>
#include <mci.h>

#define DRIVER_NAME "stm32_sdmmc"

/* SDMMC REGISTERS OFFSET */
#define SDMMC_POWER		0x00	/* SDMMC power control             */
#define SDMMC_CLKCR		0x04	/* SDMMC clock control             */
#define SDMMC_ARG		0x08	/* SDMMC argument                  */
#define SDMMC_CMD		0x0C	/* SDMMC command                   */
#define SDMMC_RESP1		0x14	/* SDMMC response 1                */
#define SDMMC_RESP2		0x18	/* SDMMC response 2                */
#define SDMMC_RESP3		0x1C	/* SDMMC response 3                */
#define SDMMC_RESP4		0x20	/* SDMMC response 4                */
#define SDMMC_DTIMER		0x24	/* SDMMC data timer                */
#define SDMMC_DLEN		0x28	/* SDMMC data length               */
#define SDMMC_DCTRL		0x2C	/* SDMMC data control              */
#define SDMMC_DCOUNT		0x30	/* SDMMC data counter              */
#define SDMMC_STA		0x34	/* SDMMC status                    */
#define SDMMC_ICR		0x38	/* SDMMC interrupt clear           */
#define SDMMC_MASK		0x3C	/* SDMMC mask                      */
#define SDMMC_IDMACTRL		0x50	/* SDMMC DMA control               */
#define SDMMC_IDMABASE0		0x58	/* SDMMC DMA buffer 0 base address */

/* SDMMC_POWER register */
#define SDMMC_POWER_PWRCTRL_MASK	GENMASK(1, 0)
#define SDMMC_POWER_PWRCTRL_OFF		0
#define SDMMC_POWER_PWRCTRL_CYCLE	2
#define SDMMC_POWER_PWRCTRL_ON		3
#define SDMMC_POWER_VSWITCH		BIT(2)
#define SDMMC_POWER_VSWITCHEN		BIT(3)
#define SDMMC_POWER_DIRPOL		BIT(4)

/* SDMMC_CLKCR register */
#define SDMMC_CLKCR_CLKDIV		GENMASK(9, 0)
#define SDMMC_CLKCR_CLKDIV_MAX		SDMMC_CLKCR_CLKDIV
#define SDMMC_CLKCR_PWRSAV		BIT(12)
#define SDMMC_CLKCR_WIDBUS_4		BIT(14)
#define SDMMC_CLKCR_WIDBUS_8		BIT(15)
#define SDMMC_CLKCR_NEGEDGE		BIT(16)
#define SDMMC_CLKCR_HWFC_EN		BIT(17)
#define SDMMC_CLKCR_DDR			BIT(18)
#define SDMMC_CLKCR_BUSSPEED		BIT(19)
#define SDMMC_CLKCR_SELCLKRX_MASK	GENMASK(21, 20)
#define SDMMC_CLKCR_SELCLKRX_CK		0
#define SDMMC_CLKCR_SELCLKRX_CKIN	BIT(20)
#define SDMMC_CLKCR_SELCLKRX_FBCK	BIT(21)

/* SDMMC_CMD register */
#define SDMMC_CMD_CMDINDEX		GENMASK(5, 0)
#define SDMMC_CMD_CMDTRANS		BIT(6)
#define SDMMC_CMD_CMDSTOP		BIT(7)
#define SDMMC_CMD_WAITRESP		GENMASK(9, 8)
#define SDMMC_CMD_WAITRESP_0		BIT(8)
#define SDMMC_CMD_WAITRESP_1		BIT(9)
#define SDMMC_CMD_WAITINT		BIT(10)
#define SDMMC_CMD_WAITPEND		BIT(11)
#define SDMMC_CMD_CPSMEN		BIT(12)
#define SDMMC_CMD_DTHOLD		BIT(13)
#define SDMMC_CMD_BOOTMODE		BIT(14)
#define SDMMC_CMD_BOOTEN		BIT(15)
#define SDMMC_CMD_CMDSUSPEND		BIT(16)

/* SDMMC_DCTRL register */
#define SDMMC_DCTRL_DTEN		BIT(0)
#define SDMMC_DCTRL_DTDIR		BIT(1)
#define SDMMC_DCTRL_DTMODE		GENMASK(3, 2)
#define SDMMC_DCTRL_DBLOCKSIZE		GENMASK(7, 4)
#define SDMMC_DCTRL_DBLOCKSIZE_SHIFT	4
#define SDMMC_DCTRL_RWSTART		BIT(8)
#define SDMMC_DCTRL_RWSTOP		BIT(9)
#define SDMMC_DCTRL_RWMOD		BIT(10)
#define SDMMC_DCTRL_SDMMCEN		BIT(11)
#define SDMMC_DCTRL_BOOTACKEN		BIT(12)
#define SDMMC_DCTRL_FIFORST		BIT(13)

/* SDMMC_STA register */
#define SDMMC_STA_CCRCFAIL		BIT(0)
#define SDMMC_STA_DCRCFAIL		BIT(1)
#define SDMMC_STA_CTIMEOUT		BIT(2)
#define SDMMC_STA_DTIMEOUT		BIT(3)
#define SDMMC_STA_TXUNDERR		BIT(4)
#define SDMMC_STA_RXOVERR		BIT(5)
#define SDMMC_STA_CMDREND		BIT(6)
#define SDMMC_STA_CMDSENT		BIT(7)
#define SDMMC_STA_DATAEND		BIT(8)
#define SDMMC_STA_DHOLD			BIT(9)
#define SDMMC_STA_DBCKEND		BIT(10)
#define SDMMC_STA_DABORT		BIT(11)
#define SDMMC_STA_DPSMACT		BIT(12)
#define SDMMC_STA_CPSMACT		BIT(13)
#define SDMMC_STA_TXFIFOHE		BIT(14)
#define SDMMC_STA_RXFIFOHF		BIT(15)
#define SDMMC_STA_TXFIFOF		BIT(16)
#define SDMMC_STA_RXFIFOF		BIT(17)
#define SDMMC_STA_TXFIFOE		BIT(18)
#define SDMMC_STA_RXFIFOE		BIT(19)
#define SDMMC_STA_BUSYD0		BIT(20)
#define SDMMC_STA_BUSYD0END		BIT(21)
#define SDMMC_STA_SDMMCIT		BIT(22)
#define SDMMC_STA_ACKFAIL		BIT(23)
#define SDMMC_STA_ACKTIMEOUT		BIT(24)
#define SDMMC_STA_VSWEND		BIT(25)
#define SDMMC_STA_CKSTOP		BIT(26)
#define SDMMC_STA_IDMATE		BIT(27)
#define SDMMC_STA_IDMABTC		BIT(28)

/* SDMMC_ICR register */
#define SDMMC_ICR_CCRCFAILC		BIT(0)
#define SDMMC_ICR_DCRCFAILC		BIT(1)
#define SDMMC_ICR_CTIMEOUTC		BIT(2)
#define SDMMC_ICR_DTIMEOUTC		BIT(3)
#define SDMMC_ICR_TXUNDERRC		BIT(4)
#define SDMMC_ICR_RXOVERRC		BIT(5)
#define SDMMC_ICR_CMDRENDC		BIT(6)
#define SDMMC_ICR_CMDSENTC		BIT(7)
#define SDMMC_ICR_DATAENDC		BIT(8)
#define SDMMC_ICR_DHOLDC		BIT(9)
#define SDMMC_ICR_DBCKENDC		BIT(10)
#define SDMMC_ICR_DABORTC		BIT(11)
#define SDMMC_ICR_BUSYD0ENDC		BIT(21)
#define SDMMC_ICR_SDMMCITC		BIT(22)
#define SDMMC_ICR_ACKFAILC		BIT(23)
#define SDMMC_ICR_ACKTIMEOUTC		BIT(24)
#define SDMMC_ICR_VSWENDC		BIT(25)
#define SDMMC_ICR_CKSTOPC		BIT(26)
#define SDMMC_ICR_IDMATEC		BIT(27)
#define SDMMC_ICR_IDMABTCC		BIT(28)
#define SDMMC_ICR_STATIC_FLAGS		((GENMASK(28, 21)) | (GENMASK(11, 0)))

/* SDMMC_MASK register */
#define SDMMC_MASK_CCRCFAILIE		BIT(0)
#define SDMMC_MASK_DCRCFAILIE		BIT(1)
#define SDMMC_MASK_CTIMEOUTIE		BIT(2)
#define SDMMC_MASK_DTIMEOUTIE		BIT(3)
#define SDMMC_MASK_TXUNDERRIE		BIT(4)
#define SDMMC_MASK_RXOVERRIE		BIT(5)
#define SDMMC_MASK_CMDRENDIE		BIT(6)
#define SDMMC_MASK_CMDSENTIE		BIT(7)
#define SDMMC_MASK_DATAENDIE		BIT(8)
#define SDMMC_MASK_DHOLDIE		BIT(9)
#define SDMMC_MASK_DBCKENDIE		BIT(10)
#define SDMMC_MASK_DABORTIE		BIT(11)
#define SDMMC_MASK_TXFIFOHEIE		BIT(14)
#define SDMMC_MASK_RXFIFOHFIE		BIT(15)
#define SDMMC_MASK_RXFIFOFIE		BIT(17)
#define SDMMC_MASK_TXFIFOEIE		BIT(18)
#define SDMMC_MASK_BUSYD0ENDIE		BIT(21)
#define SDMMC_MASK_SDMMCITIE		BIT(22)
#define SDMMC_MASK_ACKFAILIE		BIT(23)
#define SDMMC_MASK_ACKTIMEOUTIE		BIT(24)
#define SDMMC_MASK_VSWENDIE		BIT(25)
#define SDMMC_MASK_CKSTOPIE		BIT(26)
#define SDMMC_MASK_IDMABTCIE		BIT(28)

/* SDMMC_IDMACTRL register */
#define SDMMC_IDMACTRL_IDMAEN		BIT(0)

#define SDMMC_CMD_TIMEOUT		0xFFFFFFFF
#define SDMMC_BUSYD0END_TIMEOUT_US	2000000

#define IS_RISING_EDGE(reg) ((reg) & SDMMC_CLKCR_NEGEDGE ? 0 : 1)

struct stm32_sdmmc2_priv {
	void __iomem *base;
	struct mci_host mci;
	struct device	*dev;
	struct clk *clk;
	struct reset_control *reset_ctl;
	u32 clk_reg_msk;
	u32 pwr_reg_msk;
};

#define to_mci_host(mci)	container_of(mci, struct stm32_sdmmc2_priv, mci)

static int stm32_sdmmc2_reset(struct mci_host *mci, struct device *mci_dev)
{
	struct stm32_sdmmc2_priv *priv = to_mci_host(mci);

	/*
	 * Reset the SDMMC with the RCC.SDMMCxRST register bit.
	 * This will reset the SDMMC to the reset state and the CPSM and DPSM
	 * to the Idle state. SDMMC is disabled, Signals Hiz.
	 */
	reset_control_assert(priv->reset_ctl);
	udelay(2);
	reset_control_deassert(priv->reset_ctl);

	/*
	 * Set the SDMMC in power-cycle state.
	 * This will make that the SDMMC_D[7:0],
	 * SDMMC_CMD and SDMMC_CK are driven low, to prevent the card from being
	 * supplied through the signal lines.
	 */
	writel(SDMMC_POWER_PWRCTRL_CYCLE | priv->pwr_reg_msk,
	       priv->base + SDMMC_POWER);

	return 0;
}

static void stm32_sdmmc2_pwrcycle(struct stm32_sdmmc2_priv *priv)
{
	if ((readl(priv->base + SDMMC_POWER) & SDMMC_POWER_PWRCTRL_MASK) ==
	    SDMMC_POWER_PWRCTRL_CYCLE)
		return;

	stm32_sdmmc2_reset(&priv->mci, priv->dev);
}

/*
 * set the SDMMC state Power-on: the card is clocked
 * manage the SDMMC state control:
 * Reset => Power-Cycle => Power-Off => Power
 *    PWRCTRL=10     PWCTRL=00    PWCTRL=11
 */
static void stm32_sdmmc2_pwron(struct stm32_sdmmc2_priv *priv)
{
	u32 pwrctrl =
		readl(priv->base + SDMMC_POWER) &  SDMMC_POWER_PWRCTRL_MASK;

	if (pwrctrl == SDMMC_POWER_PWRCTRL_ON)
		return;

	/* warning: same PWRCTRL value after reset and for power-off state
	 * it is the reset state here = the only managed by the driver
	 */
	if (pwrctrl == SDMMC_POWER_PWRCTRL_OFF) {
		writel(SDMMC_POWER_PWRCTRL_CYCLE | priv->pwr_reg_msk,
		       priv->base + SDMMC_POWER);
	}

	/*
	 * the remaining case is SDMMC_POWER_PWRCTRL_CYCLE
	 * switch to Power-Off state: SDMCC disable, signals drive 1
	 */
	writel(SDMMC_POWER_PWRCTRL_OFF | priv->pwr_reg_msk,
	       priv->base + SDMMC_POWER);

	/* After the 1ms delay set the SDMMC to power-on */
	mdelay(1);
	writel(SDMMC_POWER_PWRCTRL_ON | priv->pwr_reg_msk,
	       priv->base + SDMMC_POWER);

	/* during the first 74 SDMMC_CK cycles the SDMMC is still disabled. */
	udelay(DIV_ROUND_UP(74 * USEC_PER_SEC, priv->mci.clock));
}

static void stm32_sdmmc2_start_data(struct stm32_sdmmc2_priv *priv,
				    struct mci_data *data, u32 data_length)
{
	unsigned int num_bytes = data->blocks * data->blocksize;
	u32 data_ctrl, idmabase0;

	/* Configure the SDMMC DPSM (Data Path State Machine) */
	data_ctrl = (__ilog2_u32(data->blocksize) <<
		     SDMMC_DCTRL_DBLOCKSIZE_SHIFT) &
		    SDMMC_DCTRL_DBLOCKSIZE;

	if (data->flags & MMC_DATA_READ) {
		data_ctrl |= SDMMC_DCTRL_DTDIR;
		idmabase0 = (u32)data->dest;
	} else {
		idmabase0 = (u32)data->src;
	}

	/* Set the SDMMC DataLength value */
	writel(data_length, priv->base + SDMMC_DLEN);

	/* Write to SDMMC DCTRL */
	writel(data_ctrl, priv->base + SDMMC_DCTRL);

	if (data->flags & MMC_DATA_WRITE)
		dma_sync_single_for_device(priv->dev, (unsigned long)idmabase0,
					   num_bytes, DMA_TO_DEVICE);
	else
		dma_sync_single_for_device(priv->dev, (unsigned long)idmabase0,
					   num_bytes, DMA_FROM_DEVICE);

	/* Enable internal DMA */
	writel(idmabase0, priv->base + SDMMC_IDMABASE0);
	writel(SDMMC_IDMACTRL_IDMAEN, priv->base + SDMMC_IDMACTRL);
}

static void stm32_sdmmc2_start_cmd(struct stm32_sdmmc2_priv *priv,
				   struct mci_cmd *cmd, u32 cmd_param,
				   u32 data_length)
{
	u32 timeout = 0;

	if (readl(priv->base + SDMMC_CMD) & SDMMC_CMD_CPSMEN)
		writel(0, priv->base + SDMMC_CMD);

	cmd_param |= cmd->cmdidx | SDMMC_CMD_CPSMEN;
	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136)
			cmd_param |= SDMMC_CMD_WAITRESP;
		else if (cmd->resp_type & MMC_RSP_CRC)
			cmd_param |= SDMMC_CMD_WAITRESP_0;
		else
			cmd_param |= SDMMC_CMD_WAITRESP_1;
	}

	/*
	 * SDMMC_DTIME must be set in two case:
	 * - on data transfert.
	 * - on busy request.
	 * If not done or too short, the dtimeout flag occurs and DPSM stays
	 * enabled/busy and waits for abort (stop transmission cmd).
	 * Next data command is not possible whereas DPSM is activated.
	 */
	if (data_length) {
		timeout = SDMMC_CMD_TIMEOUT;
	} else {
		writel(0, priv->base + SDMMC_DCTRL);

		if (cmd->resp_type & MMC_RSP_BUSY)
			timeout = SDMMC_CMD_TIMEOUT;
	}

	/* Set the SDMMC Data TimeOut value */
	writel(timeout, priv->base + SDMMC_DTIMER);

	/* Clear flags */
	writel(SDMMC_ICR_STATIC_FLAGS, priv->base + SDMMC_ICR);

	/* Set SDMMC argument value */
	writel(cmd->cmdarg, priv->base + SDMMC_ARG);

	/* Set SDMMC command parameters */
	writel(cmd_param, priv->base + SDMMC_CMD);
}

static int stm32_sdmmc2_end_cmd(struct stm32_sdmmc2_priv *priv,
				struct mci_cmd *cmd)
{
	u32 mask = SDMMC_STA_CTIMEOUT;
	u32 status;
	int ret;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		mask |= SDMMC_STA_CMDREND;
		if (cmd->resp_type & MMC_RSP_CRC)
			mask |= SDMMC_STA_CCRCFAIL;
	} else {
		mask |= SDMMC_STA_CMDSENT;
	}

	/* Polling status register */
	ret = readl_poll_timeout(priv->base + SDMMC_STA, status, status & mask,
				 SDMMC_BUSYD0END_TIMEOUT_US);
	if (ret < 0) {
		dev_err(priv->dev, "timeout reading SDMMC_STA register\n");
		return ret;
	}

	/* Check status */
	if (status & SDMMC_STA_CTIMEOUT) {
		dev_dbg(priv->dev, "%s: error SDMMC_STA_CTIMEOUT (0x%x) for cmd %d\n",
			__func__, status, cmd->cmdidx);
		return -ETIMEDOUT;
	}

	if (status & SDMMC_STA_CCRCFAIL && cmd->resp_type & MMC_RSP_CRC) {
		dev_err(priv->dev, "%s: error SDMMC_STA_CCRCFAIL (0x%x) for cmd %d\n",
			__func__, status, cmd->cmdidx);
		return -EILSEQ;
	}

	if (status & SDMMC_STA_CMDREND && cmd->resp_type & MMC_RSP_PRESENT) {
		cmd->response[0] = readl(priv->base + SDMMC_RESP1);
		if (cmd->resp_type & MMC_RSP_136) {
			cmd->response[1] = readl(priv->base + SDMMC_RESP2);
			cmd->response[2] = readl(priv->base + SDMMC_RESP3);
			cmd->response[3] = readl(priv->base + SDMMC_RESP4);
		}

		/* Wait for BUSYD0END flag if busy status is detected */
		if (cmd->resp_type & MMC_RSP_BUSY &&
		    status & SDMMC_STA_BUSYD0) {
			mask = SDMMC_STA_DTIMEOUT | SDMMC_STA_BUSYD0END;

			/* Polling status register */
			ret = readl_poll_timeout(priv->base + SDMMC_STA,
						 status, status & mask,
						 SDMMC_BUSYD0END_TIMEOUT_US);

			if (ret < 0) {
				dev_err(priv->dev, "%s: timeout reading SDMMC_STA\n",
					__func__);
				return ret;
			}

			if (status & SDMMC_STA_DTIMEOUT) {
				dev_err(priv->dev, "%s: error SDMMC_STA_DTIMEOUT (0x%x)\n",
					__func__, status);
				return -ETIMEDOUT;
			}
		}
	}

	return 0;
}

static int stm32_sdmmc2_end_data(struct stm32_sdmmc2_priv *priv,
				 struct mci_cmd *cmd,
				 struct mci_data *data)
{
	u32 mask = SDMMC_STA_DCRCFAIL | SDMMC_STA_DTIMEOUT |
		   SDMMC_STA_IDMATE | SDMMC_STA_DATAEND;
	unsigned int num_bytes = data->blocks * data->blocksize;
	u32 status;
	int ret;

	if (data->flags & MMC_DATA_READ)
		mask |= SDMMC_STA_RXOVERR;
	else
		mask |= SDMMC_STA_TXUNDERR;

	ret = readl_poll_timeout(priv->base + SDMMC_STA, status, status & mask,
				 SDMMC_BUSYD0END_TIMEOUT_US);
	if (ret < 0) {
		dev_err(priv->dev, "Time out on waiting for SDMMC_STA. cmd %d\n",
			cmd->cmdidx);
		return ret;
	}

	if (data->flags & MMC_DATA_WRITE)
		dma_sync_single_for_cpu(priv->dev, (unsigned long)data->src,
					num_bytes, DMA_TO_DEVICE);
	else
		dma_sync_single_for_cpu(priv->dev, (unsigned long)data->dest,
					num_bytes, DMA_FROM_DEVICE);

	if (status & SDMMC_STA_DCRCFAIL) {
		dev_err(priv->dev, "error SDMMC_STA_DCRCFAIL (0x%x) for cmd %d\n",
			status, cmd->cmdidx);
		return -EILSEQ;
	}

	if (status & SDMMC_STA_DTIMEOUT) {
		dev_err(priv->dev, "error SDMMC_STA_DTIMEOUT (0x%x) for cmd %d\n",
			status, cmd->cmdidx);
		return -ETIMEDOUT;
	}

	if (status & SDMMC_STA_TXUNDERR) {
		dev_err(priv->dev, "error SDMMC_STA_TXUNDERR (0x%x) for cmd %d\n",
			status, cmd->cmdidx);
		return -EIO;
	}

	if (status & SDMMC_STA_RXOVERR) {
		dev_err(priv->dev, "error SDMMC_STA_RXOVERR (0x%x) for cmd %d\n",
			status, cmd->cmdidx);
		return -EIO;
	}

	if (status & SDMMC_STA_IDMATE) {
		dev_err(priv->dev, "%s: error SDMMC_STA_IDMATE (0x%x) for cmd %d\n",
			__func__, status, cmd->cmdidx);
		return -EIO;
	}

	return 0;
}

static int stm32_sdmmc2_send_cmd(struct mci_host *mci, struct mci_cmd *cmd,
				 struct mci_data *data)
{
	struct stm32_sdmmc2_priv *priv = to_mci_host(mci);
	u32 cmdat = data ? SDMMC_CMD_CMDTRANS : 0;
	u32 data_length = 0;
	int ret;

	if (data) {
		data_length = data->blocks * data->blocksize;
		stm32_sdmmc2_start_data(priv, data, data_length);
	}

	stm32_sdmmc2_start_cmd(priv, cmd, cmdat, data_length);

	dev_dbg(priv->dev, "%s: send cmd %d data: 0x%x @ 0x%x\n", __func__,
		cmd->cmdidx, data ? data_length : 0, (unsigned int)data);

	ret = stm32_sdmmc2_end_cmd(priv, cmd);

	if (data && !ret)
		ret = stm32_sdmmc2_end_data(priv, cmd, data);

	/* Clear flags */
	writel(SDMMC_ICR_STATIC_FLAGS, priv->base + SDMMC_ICR);
	if (data)
		writel(0x0, priv->base + SDMMC_IDMACTRL);

	/*
	 * To stop Data Path State Machine, a stop_transmission command
	 * shall be send on cmd or data errors.
	 */
	if (ret && cmd->cmdidx != MMC_CMD_STOP_TRANSMISSION) {
		struct mci_cmd stop_cmd;

		stop_cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		stop_cmd.cmdarg = 0;
		stop_cmd.resp_type = MMC_RSP_R1b;

		dev_dbg(priv->dev, "%s: send STOP command to abort dpsm treatments\n",
			__func__);

		data_length = 0;

		stm32_sdmmc2_start_cmd(priv, &stop_cmd,
				       SDMMC_CMD_CMDSTOP, data_length);
		ret = stm32_sdmmc2_end_cmd(priv, &stop_cmd);

		writel(SDMMC_ICR_STATIC_FLAGS, priv->base + SDMMC_ICR);
	}

	dev_dbg(priv->dev, "%s: end for CMD %d, ret = %d\n", __func__,
		cmd->cmdidx, ret);

	return ret;
}

static void stm32_sdmmc2_set_ios(struct mci_host *mci, struct mci_ios *ios)
{
	struct stm32_sdmmc2_priv *priv = to_mci_host(mci);
	u32 desired = mci->clock;
	u32 sys_clock = clk_get_rate(priv->clk);
	u32 clk = 0;

	dev_dbg(priv->dev, "%s: bus_width = %d, clock = %d\n", __func__,
		mci->bus_width, mci->clock);

	if (mci->clock)
		stm32_sdmmc2_pwron(priv);
	else
		stm32_sdmmc2_pwrcycle(priv);

	/*
	 * clk_div = 0 => command and data generated on SDMMCCLK falling edge
	 * clk_div > 0 and NEGEDGE = 0 => command and data generated on
	 * SDMMCCLK rising edge
	 * clk_div > 0 and NEGEDGE = 1 => command and data generated on
	 * SDMMCCLK falling edge
	 */
	if (desired && (sys_clock > desired ||
			IS_RISING_EDGE(priv->clk_reg_msk))) {
		clk = DIV_ROUND_UP(sys_clock, 2 * desired);
		if (clk > SDMMC_CLKCR_CLKDIV_MAX)
			clk = SDMMC_CLKCR_CLKDIV_MAX;
	}

	if (mci->bus_width == MMC_BUS_WIDTH_4)
		clk |= SDMMC_CLKCR_WIDBUS_4;
	if (mci->bus_width == MMC_BUS_WIDTH_8)
		clk |= SDMMC_CLKCR_WIDBUS_8;

	writel(clk | priv->clk_reg_msk | SDMMC_CLKCR_HWFC_EN,
	       priv->base + SDMMC_CLKCR);
}

static int stm32_sdmmc2_probe(struct amba_device *adev,
			      const struct amba_id *id)
{
	struct device *dev = &adev->dev;
	struct device_node *np = dev->of_node;
	struct stm32_sdmmc2_priv *priv;
	struct mci_host *mci;
	int ret;

	priv = xzalloc(sizeof(*priv));

	priv->base = amba_get_mem_region(adev);
	priv->dev = dev;

	mci = &priv->mci;
	mci->send_cmd = stm32_sdmmc2_send_cmd;
	mci->set_ios = stm32_sdmmc2_set_ios;
	mci->init = stm32_sdmmc2_reset;
	mci->hw_dev = dev;

	priv->clk = clk_get(dev, NULL);
	if (IS_ERR(priv->clk)) {
		ret = PTR_ERR(priv->clk);
		goto priv_free;
	}

	ret = clk_enable(priv->clk);
	if (ret)
		goto priv_free;

	if (of_get_property(np, "st,neg-edge", NULL))
		priv->clk_reg_msk |= SDMMC_CLKCR_NEGEDGE;
	if (of_get_property(np, "st,sig-dir", NULL))
		priv->pwr_reg_msk |= SDMMC_POWER_DIRPOL;
	if (of_get_property(np, "st,use-ckin", NULL))
		priv->clk_reg_msk |= SDMMC_CLKCR_SELCLKRX_CKIN;

	priv->reset_ctl = reset_control_get(dev, NULL);
	if (IS_ERR(priv->reset_ctl))
		return PTR_ERR(priv->reset_ctl);

	mci->f_min = 400000;
	/* f_max is taken from kernel v5.3 variant_stm32_sdmmc */
	mci->f_max = 208000000;
	mci->voltages = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;

	mci_of_parse(&priv->mci);

	if (mci->f_max >= 26000000)
		mci->host_caps |= MMC_CAP_MMC_HIGHSPEED;
	if (mci->f_max >= 52000000)
		mci->host_caps |= MMC_CAP_MMC_HIGHSPEED_52MHZ;

	return mci_register(&priv->mci);

priv_free:
	free(priv);

	return ret;
}

static struct amba_id stm32_sdmmc2_ids[] = {
	/* ST Micro STM32MP15 v1.1 */
	{
		.id     = 0x10153180,
		.mask	= 0xf0ffffff,
	},
	/* ST Micro STM32MP15 v2.0 */
	{
		.id     = 0x00253180,
		.mask	= 0xf0ffffff,
	},
	/* ST Micro STM32MP13 */
	{
		.id     = 0x20253180,
		.mask	= 0xf0ffffff,
	},
	{ 0, 0 },
};

static struct amba_driver stm32_sdmmc2_driver = {
	.drv		= {
		.name	= DRIVER_NAME,
	},
	.probe		= stm32_sdmmc2_probe,
	.id_table	= stm32_sdmmc2_ids,
};

static int stm32_sdmmc2_init(void)
{
	amba_driver_register(&stm32_sdmmc2_driver);
	return 0;
}
device_initcall(stm32_sdmmc2_init);
