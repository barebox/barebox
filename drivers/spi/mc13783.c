/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <spi/spi.h>
#include <xfuncs.h>
#include <errno.h>
#include <mach/pmic.h>

#define REG_INTERRUPT_STATUS_0			0x0
#define	REG_INTERRUPT_MASK			0x1
#define	REG_INTERRUPT_SENSE_0			0x2
#define	REG_INTERRUPT_STATUS_1			0x3
#define	REG_INTERRUPT_MASK_1			0x4
#define	REG_INTERRUPT_SENSE_1			0x5
#define	REG_POWER_UP_MODE_SENSE			0x6
#define	REG_REVISION				0x7
#define	REG_SEMAPHORE				0x8
#define	REG_ARBITRATION_PERIPHERAL_AUDIO	0x9
#define	REG_ARBITRATION_SWITCHERS		0xa
#define	REG_ARBITRATION_REGULATORS(x)		(0xb + (x)) /* 0 .. 1 */
#define	REG_POWER_CONTROL(x)			(0xd + (x)) /* 0 .. 2 */
#define	REG_REGEN_ASSIGNMENT			0x10
#define	REG_CONTROL_SPARE			0x11
#define	REG_MEMORY_A				0x12
#define	REG_MEMORY_B				0x13
#define	REG_RTC_TIME				0x14
#define	REG_RTC_ALARM				0x15
#define	REG_RTC_DAY				0x16
#define	REG_RTC_DAY_ALARM			0x17
#define	REG_SWITCHERS(x)			(0x18 + (x)) /* 0 .. 5 */
#define	REG_REGULATOR_SETTING(x)		(0x1e + (x)) /* 0 .. 1 */
#define	REG_REGULATOR_MODE(x)			(0x20 + (x)) /* 0 .. 1 */
#define	REG_POWER_MISCELLANEOUS			0x22
#define	REG_POWER_SPARE				0x23
#define	REG_AUDIO_RX_0				0x24
#define	REG_AUDIO_RX_1				0x25
#define	REG_AUDIO_TX				0x26
#define	REG_AUDIO_SSI_NETWORK			0x27
#define	REG_AUDIO_CODEC				0x28
#define	REG_AUDIO_STEREO_DAC			0x29
#define	REG_AUDIO_SPARE				0x2a
#define	REG_ADC(x)				(0x2b + (x)) /* 0 .. 4 */
#define	REG_CHARGER				0x30
#define	REG_USB					0x31
#define	REG_CHARGE_USB_SPARE			0x32
#define	REG_LED_CONTROL(x)			(0x33 + (x)) /* 0 .. 5 */
#define	REG_SPARE				0x39
#define	REG_TRIM(x)				(0x3a + (x)) /* 0 .. 1 */
#define	REG_TEST(x)				(0x3c + (x)) /* 0 .. 3 */

#define MXC_PMIC_REG_NUM(reg)	(((reg) & 0x3f) << 25)
#define MXC_PMIC_WRITE		(1 << 31)

#define SWX_VOLTAGE(x)		((x) & 0x3f)
#define SWX_VOLTAGE_DVS(x)	(((x) & 0x3f) << 6)
#define SWX_VOLTAGE_STANDBY(x)	(((x) & 0x3f) << 12)
#define SWX_VOLTAGE_1_450	0x16

#define SWX_MODE_OFF		0
#define SWX_MODE_NO_PULSE_SKIP	1
#define SWX_MODE_PULSE_SKIP	2
#define SWX_MODE_LOW_POWER_PFM	3

#define SW1A_MODE(x)		(((x) & 0x3) << 0)
#define SW1A_MODE_STANDBY(x)	(((x) & 0x3) << 2)
#define SW1B_MODE(x)		(((x) & 0x3) << 10)
#define SW1B_MODE_STANDBY(x)	(((x) & 0x3) << 12)
#define SW1A_SOFTSTART		(1 << 9)
#define SW1B_SOFTSTART		(1 << 17)
#define SW_PLL_FACTOR(x)	(((x) - 28) << 19)

struct pmic_priv {
	struct cdev cdev;
	struct spi_device *spi;
};

static int spi_rw(struct spi_device *spi, void * buf, size_t len)
{
	int ret;

	struct spi_transfer t = {
		.tx_buf = (const void *)buf,
		.rx_buf = buf,
		.len = len,
		.cs_change = 0,
		.delay_usecs = 0,
	};
	struct spi_message m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	if ((ret = spi_sync(spi, &m)))
		return ret;
	return 0;
}

static uint32_t pmic_read_reg(struct pmic_priv *pmic, int reg)
{
	uint32_t buf;

	buf = MXC_PMIC_REG_NUM(reg);

	spi_rw(pmic->spi, &buf, 4);

	return buf;
}

static void pmic_write_reg(struct pmic_priv *pmic, int reg, uint32_t val)
{
	uint32_t buf = MXC_PMIC_REG_NUM(reg) | MXC_PMIC_WRITE | (val & 0xffffff);

	spi_rw(pmic->spi, &buf, 4);
}

static struct pmic_priv *pmic_device;

int pmic_power(void)
{
	if(!pmic_device) {
		printf("%s: no pmic device available\n", __FUNCTION__);
		return -ENODEV;
	}

	pmic_write_reg(pmic_device, REG_SWITCHERS(0),
			SWX_VOLTAGE(SWX_VOLTAGE_1_450) |
			SWX_VOLTAGE_DVS(SWX_VOLTAGE_1_450) |
			SWX_VOLTAGE_STANDBY(SWX_VOLTAGE_1_450));

	pmic_write_reg(pmic_device, REG_SWITCHERS(4),
		SW1A_MODE(SWX_MODE_NO_PULSE_SKIP) |
		SW1A_MODE_STANDBY(SWX_MODE_NO_PULSE_SKIP)|
		SW1A_SOFTSTART |
		SW1B_MODE(SWX_MODE_NO_PULSE_SKIP) |
		SW1B_MODE_STANDBY(SWX_MODE_NO_PULSE_SKIP) |
		SW1B_SOFTSTART |
		SW_PLL_FACTOR(32)
		);

	return 0;
}

ssize_t pmic_read(struct cdev *cdev, void *_buf, size_t count, ulong offset, ulong flags)
{
	int i = count >> 2;
	uint32_t *buf = _buf;

	offset >>= 2;

	while (i) {
		*buf = pmic_read_reg(pmic_device, offset);
		buf++;
		i--;
		offset++;
	}

	return count;
}

ssize_t pmic_write(struct cdev *cdev, const void *_buf, size_t count, ulong offset, ulong flags)
{
	int i = count >> 2;
	const uint32_t *buf = _buf;

	offset >>= 2;

	while (i) {
		pmic_write_reg(pmic_device, offset, *buf);
		buf++;
		i--;
		offset++;
	}

	return count;
}

static struct file_operations pmic_fops = {
	.lseek	= dev_lseek_default,
	.read	= pmic_read,
	.write	= pmic_write,
};

static int pmic_probe(struct device_d *dev)
{
	struct spi_device *spi = (struct spi_device *)dev->type_data;

	if (pmic_device)
		return -EBUSY;

	pmic_device = xzalloc(sizeof(*pmic_device));

	pmic_device->cdev.name = "pmic";
	pmic_device->cdev.size = 256;
	pmic_device->cdev.dev = dev;
	pmic_device->cdev.ops = &pmic_fops;

	spi->mode = SPI_MODE_0 | SPI_CS_HIGH;
	spi->bits_per_word = 32;
	pmic_device->spi = spi;

	devfs_create(&pmic_device->cdev);

	return 0;
}

static struct driver_d pmic_driver = {
	.name  = "mc13783",
	.probe = pmic_probe,
};

static int pmic_init(void)
{
        register_driver(&pmic_driver);
        return 0;
}

device_initcall(pmic_init);

