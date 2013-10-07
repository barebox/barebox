/*
 * (c) 2004 Sascha Hauer <sascha@saschahauer.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <notifier.h>
#include <io.h>
#include <linux/err.h>
#include <linux/clk.h>

#define URXD0	0x0	/* Receiver Register */
#define URTX0	0x40	/* Transmitter Register */
#define UCR1	0x80	/* Control Register 1 */
#define UCR2	0x84	/* Control Register 2 */
#define UCR3	0x88	/* Control Register 3 */
#define UCR4	0x8c	/* Control Register 4 */
#define UFCR	0x90	/* FIFO Control Register */
#define USR1	0x94	/* Status Register 1 */
#define USR2	0x98	/* Status Register 2 */
#define UESC	0x9c	/* Escape Character Register */
#define UTIM	0xa0	/* Escape Timer Register */
#define UBIR	0xa4	/* BRM Incremental Register */
#define UBMR	0xa8	/* BRM Modulator Register */
#define UBRC	0xac	/* Baud Rate Count Register */

/* UART Control Register Bit Fields.*/
#define  URXD_CHARRDY    (1<<15)
#define  URXD_ERR        (1<<14)
#define  URXD_OVRRUN     (1<<13)
#define  URXD_FRMERR     (1<<12)
#define  URXD_BRK        (1<<11)
#define  URXD_PRERR      (1<<10)
#define  UCR1_ADEN       (1<<15) /* Auto dectect interrupt */
#define  UCR1_ADBR       (1<<14) /* Auto detect baud rate */
#define  UCR1_TRDYEN     (1<<13) /* Transmitter ready interrupt enable */
#define  UCR1_IDEN       (1<<12) /* Idle condition interrupt */
#define  UCR1_RRDYEN     (1<<9)	 /* Recv ready interrupt enable */
#define  UCR1_RDMAEN     (1<<8)	 /* Recv ready DMA enable */
#define  UCR1_IREN       (1<<7)	 /* Infrared interface enable */
#define  UCR1_TXMPTYEN   (1<<6)	 /* Transimitter empty interrupt enable */
#define  UCR1_RTSDEN     (1<<5)	 /* RTS delta interrupt enable */
#define  UCR1_SNDBRK     (1<<4)	 /* Send break */
#define  UCR1_TDMAEN     (1<<3)	 /* Transmitter ready DMA enable */
#define  UCR1_UARTCLKEN  (1<<2)	 /* UART clock enabled */
#define  UCR1_DOZE       (1<<1)	 /* Doze */
#define  UCR1_UARTEN     (1<<0)	 /* UART enabled */
#define  UCR2_ESCI     	 (1<<15) /* Escape seq interrupt enable */
#define  UCR2_IRTS  	 (1<<14) /* Ignore RTS pin */
#define  UCR2_CTSC  	 (1<<13) /* CTS pin control */
#define  UCR2_CTS        (1<<12) /* Clear to send */
#define  UCR2_ESCEN      (1<<11) /* Escape enable */
#define  UCR2_PREN       (1<<8)  /* Parity enable */
#define  UCR2_PROE       (1<<7)  /* Parity odd/even */
#define  UCR2_STPB       (1<<6)	 /* Stop */
#define  UCR2_WS         (1<<5)	 /* Word size */
#define  UCR2_RTSEN      (1<<4)	 /* Request to send interrupt enable */
#define  UCR2_TXEN       (1<<2)	 /* Transmitter enabled */
#define  UCR2_RXEN       (1<<1)	 /* Receiver enabled */
#define  UCR2_SRST 	 (1<<0)	 /* SW reset */
#define  UCR3_DTREN 	 (1<<13) /* DTR interrupt enable */
#define  UCR3_PARERREN   (1<<12) /* Parity enable */
#define  UCR3_FRAERREN   (1<<11) /* Frame error interrupt enable */
#define  UCR3_DSR        (1<<10) /* Data set ready */
#define  UCR3_DCD        (1<<9)  /* Data carrier detect */
#define  UCR3_RI         (1<<8)  /* Ring indicator */
#define  UCR3_TIMEOUTEN  (1<<7)  /* Timeout interrupt enable */
#define  UCR3_RXDSEN	 (1<<6)  /* Receive status interrupt enable */
#define  UCR3_AIRINTEN   (1<<5)  /* Async IR wake interrupt enable */
#define  UCR3_AWAKEN	 (1<<4)  /* Async wake interrupt enable */
#define  UCR3_REF25 	 (1<<3)  /* Ref freq 25 MHz (i.MXL / i.MX1) */
#define  UCR3_REF30 	 (1<<2)  /* Ref Freq 30 MHz (i.MXL / i.MX1) */
#define  UCR3_RXDMUXSEL  (1<<2)  /* RXD Muxed input select (i.MX27) */
#define  UCR3_INVT  	 (1<<1)  /* Inverted Infrared transmission */
#define  UCR3_BPEN  	 (1<<0)  /* Preset registers enable */
#define  UCR4_CTSTL_32   (32<<10) /* CTS trigger level (32 chars) */
#define  UCR4_INVR  	 (1<<9)  /* Inverted infrared reception */
#define  UCR4_ENIRI 	 (1<<8)  /* Serial infrared interrupt enable */
#define  UCR4_WKEN  	 (1<<7)  /* Wake interrupt enable */
#define  UCR4_REF16 	 (1<<6)  /* Ref freq 16 MHz */
#define  UCR4_IRSC  	 (1<<5)  /* IR special case */
#define  UCR4_TCEN  	 (1<<3)  /* Transmit complete interrupt enable */
#define  UCR4_BKEN  	 (1<<2)  /* Break condition interrupt enable */
#define  UCR4_OREN  	 (1<<1)  /* Receiver overrun interrupt enable */
#define  UCR4_DREN  	 (1<<0)  /* Recv data ready interrupt enable */
#define  UFCR_RXTL_SHF   0       /* Receiver trigger level shift */
#define  UFCR_RFDIV      (7<<7)  /* Reference freq divider mask */
#define  UFCR_TXTL_SHF   10      /* Transmitter trigger level shift */
#define  USR1_PARITYERR  (1<<15) /* Parity error interrupt flag */
#define  USR1_RTSS  	 (1<<14) /* RTS pin status */
#define  USR1_TRDY  	 (1<<13) /* Transmitter ready interrupt/dma flag */
#define  USR1_RTSD  	 (1<<12) /* RTS delta */
#define  USR1_ESCF  	 (1<<11) /* Escape seq interrupt flag */
#define  USR1_FRAMERR    (1<<10) /* Frame error interrupt flag */
#define  USR1_RRDY       (1<<9)	 /* Receiver ready interrupt/dma flag */
#define  USR1_TIMEOUT    (1<<7)	 /* Receive timeout interrupt status */
#define  USR1_RXDS  	 (1<<6)	 /* Receiver idle interrupt flag */
#define  USR1_AIRINT	 (1<<5)	 /* Async IR wake interrupt flag */
#define  USR1_AWAKE 	 (1<<4)	 /* Aysnc wake interrupt flag */
#define  USR2_ADET  	 (1<<15) /* Auto baud rate detect complete */
#define  USR2_TXFE  	 (1<<14) /* Transmit buffer FIFO empty */
#define  USR2_DTRF  	 (1<<13) /* DTR edge interrupt flag */
#define  USR2_IDLE  	 (1<<12) /* Idle condition */
#define  USR2_IRINT 	 (1<<8)	 /* Serial infrared interrupt flag */
#define  USR2_WAKE  	 (1<<7)	 /* Wake */
#define  USR2_RTSF  	 (1<<4)	 /* RTS edge interrupt flag */
#define  USR2_TXDC  	 (1<<3)	 /* Transmitter complete */
#define  USR2_BRCD  	 (1<<2)	 /* Break condition */
#define  USR2_ORE        (1<<1)	 /* Overrun error */
#define  USR2_RDR        (1<<0)	 /* Recv data ready */
#define  UTS_FRCPERR	 (1<<13) /* Force parity error */
#define  UTS_LOOP        (1<<12) /* Loop tx and rx */
#define  UTS_TXEMPTY	 (1<<6)	 /* TxFIFO empty */
#define  UTS_RXEMPTY	 (1<<5)	 /* RxFIFO empty */
#define  UTS_TXFULL 	 (1<<4)	 /* TxFIFO full */
#define  UTS_RXFULL 	 (1<<3)	 /* RxFIFO full */
#define  UTS_SOFTRST	 (1<<0)	 /* Software reset */

/*
 * create default values for different platforms
 */
struct imx_serial_devtype_data {
	u32 ucr1_val;
	u32 ucr3_val;
	u32 ucr4_val;
	u32 uts;
	u32 onems;
};

static struct imx_serial_devtype_data imx1_data = {
	.ucr1_val = UCR1_UARTCLKEN,
	.ucr3_val = 0,
	.ucr4_val = UCR4_CTSTL_32 | UCR4_REF16,
	.uts = 0xd0,
	.onems = 0,
};

static struct imx_serial_devtype_data imx21_data = {
	.ucr1_val = 0,
	.ucr3_val = 0x700 | UCR3_RXDMUXSEL,
	.ucr4_val = UCR4_CTSTL_32,
	.uts = 0xb4,
	.onems = 0xb0,
};

struct imx_serial_priv {
	struct console_device cdev;
	int baudrate;
	struct notifier_block notify;
	void __iomem *regs;
	struct clk *clk;
	struct imx_serial_devtype_data *devtype;
};

static int imx_serial_reffreq(struct imx_serial_priv *priv)
{
	ulong rfdiv;

	rfdiv = (readl(priv->regs + UFCR) >> 7) & 7;
	rfdiv = rfdiv < 6 ? 6 - rfdiv : 7;

	return clk_get_rate(priv->clk) / rfdiv;
}

/*
 * Initialise the serial port with the given baudrate. The settings
 * are always 8 data bits, no parity, 1 stop bit, no start bits.
 *
 */
static int imx_serial_init_port(struct console_device *cdev)
{
	struct imx_serial_priv *priv = container_of(cdev,
					struct imx_serial_priv, cdev);
	void __iomem *regs = priv->regs;
	uint32_t val;

	writel(priv->devtype->ucr1_val, regs + UCR1);
	writel(UCR2_WS | UCR2_IRTS, regs + UCR2);
	writel(priv->devtype->ucr3_val, regs + UCR3);
	writel(priv->devtype->ucr4_val, regs + UCR4);
	writel(0x0000002B, regs + UESC);
	writel(0, regs + UTIM);
	writel(0, regs + UBIR);
	writel(0, regs + UBMR);
	writel(0, regs + priv->devtype->uts);


	/* Configure FIFOs */
	writel(0xa81, regs + UFCR);


	if (priv->devtype->onems)
		writel(imx_serial_reffreq(priv) / 1000, regs + priv->devtype->onems);

	/* Enable FIFOs */
	val = readl(regs + UCR2);
	val |= UCR2_SRST | UCR2_RXEN | UCR2_TXEN;
	writel(val, regs + UCR2);

  	/* Clear status flags */
	val = readl(regs + USR2);
	val |= USR2_ADET | USR2_DTRF | USR2_IDLE | USR2_IRINT | USR2_WAKE |
	       USR2_RTSF | USR2_BRCD | USR2_ORE | USR2_RDR;
	writel(val, regs + USR2);

  	/* Clear status flags */
	val = readl(regs + USR2);
	val |= USR1_PARITYERR | USR1_RTSD | USR1_ESCF | USR1_FRAMERR | USR1_AIRINT |
	       USR1_AWAKE;
	writel(val, regs + USR2);

	return 0;
}

static void imx_serial_putc(struct console_device *cdev, char c)
{
	struct imx_serial_priv *priv = container_of(cdev,
					struct imx_serial_priv, cdev);

	/* Wait for Tx FIFO not full */
	while (readl(priv->regs + priv->devtype->uts) & UTS_TXFULL);

        writel(c, priv->regs + URTX0);
}

static int imx_serial_tstc(struct console_device *cdev)
{
	struct imx_serial_priv *priv = container_of(cdev,
					struct imx_serial_priv, cdev);

	/* If receive fifo is empty, return false */
	if (readl(priv->regs + priv->devtype->uts) & UTS_RXEMPTY)
		return 0;
	return 1;
}

static int imx_serial_getc(struct console_device *cdev)
{
	struct imx_serial_priv *priv = container_of(cdev,
					struct imx_serial_priv, cdev);
	unsigned char ch;

	while (readl(priv->regs + priv->devtype->uts) & UTS_RXEMPTY);

	ch = readl(priv->regs + URXD0);

	return ch;
}

static void imx_serial_flush(struct console_device *cdev)
{
	struct imx_serial_priv *priv = container_of(cdev,
					struct imx_serial_priv, cdev);

	while (!(readl(priv->regs + USR2) & USR2_TXDC));
}

static int imx_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct imx_serial_priv *priv = container_of(cdev,
					struct imx_serial_priv, cdev);
	void __iomem *regs = priv->regs;
	uint32_t val;
	uint32_t ucr1 = readl(regs + UCR1);

	/* disable UART */
	val = readl(regs + UCR1);
	val &= ~UCR1_UARTEN;
	writel(val, regs + UCR1);

	/* Set the numerator value minus one of the BRM ratio */
	writel((baudrate / 100) - 1, regs + UBIR);
	/* Set the denominator value minus one of the BRM ratio    */
	writel((imx_serial_reffreq(priv) / 1600) - 1, regs + UBMR);

	writel(ucr1, regs + UCR1);

	priv->baudrate = baudrate;

	return 0;
}

static int imx_clocksource_clock_change(struct notifier_block *nb,
			unsigned long event, void *data)
{
	struct imx_serial_priv *priv = container_of(nb,
				struct imx_serial_priv, notify);

	imx_serial_setbaudrate(&priv->cdev, priv->baudrate);

        return 0;
}

static int imx_serial_probe(struct device_d *dev)
{
	struct console_device *cdev;
	struct imx_serial_priv *priv;
	uint32_t val;
	struct imx_serial_devtype_data *devtype;
	int ret;

	ret = dev_get_drvdata(dev, (unsigned long *)&devtype);
	if (ret)
		return ret;

	priv = xzalloc(sizeof(*priv));
	priv->devtype = devtype;
	cdev = &priv->cdev;
	dev->priv = priv;

	priv->clk = clk_get(dev, NULL);
	if (IS_ERR(priv->clk)) {
		ret = PTR_ERR(priv->clk);
		goto err_free;
	}

	priv->regs = dev_request_mem_region(dev, 0);
	cdev->dev = dev;
	cdev->tstc = imx_serial_tstc;
	cdev->putc = imx_serial_putc;
	cdev->getc = imx_serial_getc;
	cdev->flush = imx_serial_flush;
	cdev->setbrg = imx_serial_setbaudrate;

	imx_serial_init_port(cdev);

	/* Enable UART */
	val = readl(priv->regs + UCR1);
	val |= UCR1_UARTEN;
	writel(val, priv->regs + UCR1);

	console_register(cdev);
	priv->notify.notifier_call = imx_clocksource_clock_change;
	clock_register_client(&priv->notify);

	return 0;

err_free:
	free(priv);

	return ret;
}

static void imx_serial_remove(struct device_d *dev)
{
	struct imx_serial_priv *priv = dev->priv;

	imx_serial_flush(&priv->cdev);
	console_unregister(&priv->cdev);
	free(priv);
}

static __maybe_unused struct of_device_id imx_serial_dt_ids[] = {
	{
		.compatible = "fsl,imx1-uart",
		.data = (unsigned long)&imx1_data,
	}, {
		.compatible = "fsl,imx21-uart",
		.data = (unsigned long)&imx21_data,
	}, {
		/* sentinel */
	}
};

static struct platform_device_id imx_serial_ids[] = {
	{
		.name = "imx1-uart",
		.driver_data = (unsigned long)&imx1_data,
	}, {
		.name = "imx21-uart",
		.driver_data = (unsigned long)&imx21_data,
	}, {
		/* sentinel */
	},
};

static struct driver_d imx_serial_driver = {
	.name   = "imx_serial",
	.probe  = imx_serial_probe,
	.remove = imx_serial_remove,
	.of_compatible = DRV_OF_COMPAT(imx_serial_dt_ids),
	.id_table = imx_serial_ids,
};
console_platform_driver(imx_serial_driver);
