#include <common.h>
#include <init.h>
#include <asm/io.h>
#include <mach/imx-regs.h>
#include <gpio.h>

#define IMX_SPI_ACTIVE_HIGH	1
#define SPI_RETRY_TIMES		100
#define CLKCTL_CACRR		0x10
#define REV_ATLAS_LITE_2_0	0x10

/* Only for SPI master support */
struct imx_spi_dev {
	unsigned int base;	// base address of SPI module the device is connected to
	unsigned int freq;	// desired clock freq in Hz for this device
	unsigned int ss_pol;	// ss polarity: 1=active high; 0=active low
	unsigned int ss;	// slave select
	unsigned int in_sctl;	// inactive sclk ctl: 1=stay low; 0=stay high
	unsigned int in_dctl;	// inactive data ctl: 1=stay low; 0=stay high
	unsigned int ssctl;	// single burst mode vs multiple: 0=single; 1=multi
	unsigned int sclkpol;	// sclk polarity: active high=0; active low=1
	unsigned int sclkpha;	// sclk phase: 0=phase 0; 1=phase1
	unsigned int fifo_sz;	// fifo size in bytes for either tx or rx. Don't add them up!
	unsigned int ctrl_reg;
	unsigned int cfg_reg;
};

struct imx_spi_dev imx_spi_pmic = {
      .base	= MX51_CSPI1_BASE_ADDR,
      .freq	= 25000000,
      .ss_pol	= IMX_SPI_ACTIVE_HIGH,
      .ss	= 0, /* slave select 0 */
      .fifo_sz	= 32,
};

/*
 * Initialization function for a spi slave device. It must be called BEFORE
 * any spi operations. The SPI module will be -disabled- after this call.
 */
static int imx_spi_init(struct imx_spi_dev *dev)
{
	unsigned int clk_src = 66500000;
	unsigned int pre_div = 0, post_div = 0, i, reg_ctrl = 0, reg_config = 0;

	if (dev->freq == 0) {
		printf("Error: desired clock is 0\n");
		return -1;
	}

	/* control register setup */
	if (clk_src > dev->freq) {
		pre_div = clk_src / dev->freq;
		if (pre_div > 16) {
			post_div = pre_div / 16;
			pre_div = 15;
		}
		if (post_div != 0) {
			for (i = 0; i < 16; i++) {
				if ((1 << i) >= post_div)
					break;
			}
			if (i == 16) {
				printf
				    ("Error: no divider can meet the freq: %d\n",
				     dev->freq);
				return -1;
			}
			post_div = i;
		}
	}
	debug("pre_div = %d, post_div=%d\n", pre_div, post_div);
	reg_ctrl |= pre_div << 12;
	reg_ctrl |= post_div << 8;
	reg_ctrl |= 1 << (dev->ss + 4);	/* always set to master mode */

	/* configuration register setup */
	reg_config |= dev->ss_pol << (dev->ss + 12);
	reg_config |= dev->in_sctl << (dev->ss + 20);
	reg_config |= dev->in_dctl << (dev->ss + 16);
	reg_config |= dev->ssctl << (dev->ss + 8);
	reg_config |= dev->sclkpol << (dev->ss + 4);
	reg_config |= dev->sclkpha << (dev->ss + 0);

	debug("reg_ctrl = 0x%x\n", reg_ctrl);
	/* reset the spi */
	writel(0, dev->base + 0x8);
	writel(reg_ctrl, dev->base + 0x8);
	debug("reg_config = 0x%x\n", reg_config);
	writel(reg_config, dev->base + 0xC);
	/* save control register */
	dev->cfg_reg = reg_config;
	dev->ctrl_reg = reg_ctrl;

	/* clear interrupt reg */
	writel(0, dev->base + 0x10);
	writel(3 << 6, dev->base + 0x18);

	return 0;
}

static int imx_spi_xfer(struct imx_spi_dev *dev,	/* spi device pointer */
		  void *tx_buf,	/* tx buffer (has to be 4-byte aligned) */
		  void *rx_buf,	/* rx buffer (has to be 4-byte aligned) */
		  int burst_bits	/* total number of bits in one burst (or xfer) */
    )
{
	int val = SPI_RETRY_TIMES;
	unsigned int *p_buf;
	unsigned int reg;
	int len, ret_val = 0;
	int burst_bytes = burst_bits / 8;

	/* Account for rounding of non-byte aligned burst sizes */
	if ((burst_bits % 8) != 0)
		burst_bytes++;

	if (burst_bytes > dev->fifo_sz) {
		printf("Error: maximum burst size is 0x%x bytes, asking 0x%x\n",
		       dev->fifo_sz, burst_bytes);
		return -1;
	}

	dev->ctrl_reg = (dev->ctrl_reg & ~0xFFF00000) | ((burst_bits - 1) << 20);
	writel(dev->ctrl_reg | 0x1, dev->base + 0x8);
	writel(dev->cfg_reg, dev->base + 0xC);
	debug("ctrl_reg=0x%x, cfg_reg=0x%x\n",
	       readl(dev->base + 0x8), readl(dev->base + 0xC));

	/* move data to the tx fifo */
	len = burst_bytes;
	for (p_buf = tx_buf; len > 0; p_buf++, len -= 4)
		writel(*p_buf, dev->base + 0x4);

	reg = readl(dev->base + 0x8);
	reg |= (1 << 2);	/* set xch bit */
	writel(reg, dev->base + 0x8);

	/* poll on the TC bit (transfer complete) */
	while ((val-- > 0) && (readl(dev->base + 0x18) & (1 << 7)) == 0);

	/* clear the TC bit */
	writel(3 << 6, dev->base + 0x18);

	if (val == 0) {
		printf("Error: re-tried %d times without response. Give up\n",
		       SPI_RETRY_TIMES);
		ret_val = -1;
		goto error;
	}

	/* move data in the rx buf */
	len = burst_bytes;
	for (p_buf = rx_buf; len > 0; p_buf++, len -= 4)
		*p_buf = readl(dev->base + 0x0);

error:
	writel(0, dev->base + 0x8);
	return ret_val;
}

/*
 * To read/write to a PMIC register. For write, it does another read for the
 * actual register value.
 *
 * @param   reg         register number inside the PMIC
 * @param   val         data to be written to the register; don't care for read
 * @param   write       0 for read; 1 for write
 *
 * @return              the actual data in the PMIC register
 */
static unsigned int
pmic_reg(unsigned int reg, unsigned int val, unsigned int write)
{
	static unsigned int pmic_tx, pmic_rx;

	if (reg > 63 || write > 1) {
		printf("<reg num> = %d is invalid. Should be less then 63\n",
		       reg);
		return 0;
	}
	pmic_tx = (write << 31) | (reg << 25) | (val & 0x00FFFFFF);
	debug("reg=0x%x, val=0x%08x\n", reg, pmic_tx);

	imx_spi_xfer(&imx_spi_pmic, (unsigned char *) &pmic_tx,
			  (unsigned char *) &pmic_rx, (4 * 8));

	if (write) {
		pmic_tx &= ~(1 << 31);
		imx_spi_xfer(&imx_spi_pmic, (unsigned char *) &pmic_tx,
				  (unsigned char *) &pmic_rx, (4 * 8));
	}

	return pmic_rx;
}

static void show_pmic_info(void)
{
	unsigned int rev_id;
	char *rev;

	rev_id = pmic_reg(7, 0, 0);

	switch (rev_id & 0x1F) {
	case 0x1: rev = "1.0"; break;
	case 0x9: rev = "1.1"; break;
	case 0xa: rev = "1.2"; break;
	case 0x10:
		if (((rev_id >> 9) & 0x3) == 0)
			rev = "2.0";
		else
			rev = "2.0a";
		break;
	case 0x11: rev = "2.1"; break;
	case 0x18: rev = "3.0"; break;
	case 0x19: rev = "3.1"; break;
	case 0x1a: rev = "3.2"; break;
	case 0x2: rev = "3.2a"; break;
	case 0x1b: rev = "3.3"; break;
	case 0x1d: rev = "3.5"; break;
	default: rev = "unknown"; break;
	}

	printf("PMIC ID: 0x%08x [Rev: %s]\n", rev_id, rev);
}

int babbage_power_init(void)
{
	unsigned int val;
	unsigned int reg;

	imx_spi_init(&imx_spi_pmic);

	show_pmic_info();

	/* Write needed to Power Gate 2 register */
	val = pmic_reg(34, 0, 0);
	val &= ~0x10000;
	pmic_reg(34, val, 1);

	/* Write needed to update Charger 0 */
	pmic_reg(48, 0x0023807F, 1);

	/* power up the system first */
	pmic_reg(34, 0x00200000, 1);

	if (1) {
		/* Set core voltage to 1.1V */
		val = pmic_reg(24, 0, 0);
		val &= ~0x1f;
		val |= 0x14;
		pmic_reg(24, val, 1);

		/* Setup VCC (SW2) to 1.25 */
		val = pmic_reg(25, 0, 0);
		val &= ~0x1f;
		val |= 0x1a;
		pmic_reg(25, val, 1);

		/* Setup 1V2_DIG1 (SW3) to 1.25 */
		val = pmic_reg(26, 0, 0);
		val &= ~0x1f;
		val |= 0x1a;
		pmic_reg(26, val, 1);
		udelay(50);
		/* Raise the core frequency to 800MHz */
		writel(0x0, MX51_CCM_BASE_ADDR + CLKCTL_CACRR);
	} else {
		/* TO 3.0 */
		/* Setup VCC (SW2) to 1.225 */
		val = pmic_reg(25, 0, 0);
		val &= ~0x1f;
		val |= 0x19;
		pmic_reg(25, val, 1);

		/* Setup 1V2_DIG1 (SW3) to 1.2 */
		val = pmic_reg(26, 0, 0);
		val &= ~0x1f;
		val |= 0x18;
		pmic_reg(26, val, 1);
	}

	if (((pmic_reg(7, 0, 0) & 0x1F) < REV_ATLAS_LITE_2_0)
	    || (((pmic_reg(7, 0, 0) >> 9) & 0x3) == 0)) {
		/* Set switchers in PWM mode for Atlas 2.0 and lower */
		/* Setup the switcher mode for SW1 & SW2 */
		val = pmic_reg(28, 0, 0);
		val &= ~0x3c0f;
		val |= 0x1405;
		pmic_reg(28, val, 1);

		/* Setup the switcher mode for SW3 & SW4 */
		val = pmic_reg(29, 0, 0);
		val &= ~0xf0f;
		val |= 0x505;
		pmic_reg(29, val, 1);
	} else {
		/* Set switchers in Auto in NORMAL mode & STANDBY mode for Atlas 2.0a */
		/* Setup the switcher mode for SW1 & SW2 */
		val = pmic_reg(28, 0, 0);
		val &= ~0x3c0f;
		val |= 0x2008;
		pmic_reg(28, val, 1);

		/* Setup the switcher mode for SW3 & SW4 */
		val = pmic_reg(29, 0, 0);
		val &= ~0xf0f;
		val |= 0x808;
		pmic_reg(29, val, 1);
	}

	/* Set VDIG to 1.65V, VGEN3 to 1.8V, VCAM to 2.5V */
	val = pmic_reg(30, 0, 0);
	val &= ~0x34030;
	val |= 0x10020;
	pmic_reg(30, val, 1);

	/* Set VVIDEO to 2.775V, VAUDIO to 3V, VSD to 3.15V */
	val = pmic_reg(31, 0, 0);
	val &= ~0x1FC;
	val |= 0x1F4;
	pmic_reg(31, val, 1);

	/* Configure VGEN3 and VCAM regulators to use external PNP */
	val = 0x208;
	pmic_reg(33, val, 1);
	udelay(200);

	gpio_direction_output(32 + 14, 0); /* Lower reset line */

	/* Enable VGEN3, VCAM, VAUDIO, VVIDEO, VSD regulators */
	val = 0x49249;
	pmic_reg(33, val, 1);

	udelay(500);

	gpio_set_value(32 + 14, 1);

	return 0;
}

