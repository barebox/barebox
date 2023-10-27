// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <linux/usb/ch9.h>
#include <soc/fsl/fsl_udc.h>
#include <mach/imx/imx8mm-regs.h>
#include <mach/imx/imx6-regs.h>
#include <mach/imx/imx7-regs.h>

static void fsl_queue_td(struct usb_dr_device *dr, struct ep_td_struct *dtd,
			 int ep_is_in)
{
	int ep_index = 0;
	int i = ep_index * 2 + ep_is_in;
	u32 bitmask;
	volatile struct ep_queue_head *dQH =
		(void *)(unsigned long)readl(&dr->endpointlistaddr);
	unsigned long td_dma = (unsigned long)dtd;

	dQH = &dQH[i];

	bitmask = ep_is_in ? (1 << (ep_index + 16)) : (1 << (ep_index));

	dQH->next_dtd_ptr = cpu_to_le32(td_dma & EP_QUEUE_HEAD_NEXT_POINTER_MASK);

	dQH->size_ioc_int_sts &= cpu_to_le32(~(EP_QUEUE_HEAD_STATUS_ACTIVE
			| EP_QUEUE_HEAD_STATUS_HALT));

	writel(bitmask, &dr->endpointprime);
}

static struct ep_td_struct dtd_data __attribute__((aligned(64)));
static struct ep_td_struct dtd_status __attribute__((aligned(64)));

static int fsl_ep_queue(struct usb_dr_device *dr, struct ep_td_struct *dtd,
			void *buf, int len)
{
	u32 swap_temp;

	memset(dtd, 0, sizeof(*dtd));

	/* Clear reserved field */
	swap_temp = cpu_to_le32(dtd->size_ioc_sts);
	swap_temp &= ~DTD_RESERVED_FIELDS;
	dtd->size_ioc_sts = cpu_to_le32(swap_temp);

	swap_temp = (unsigned long)buf;
	dtd->buff_ptr0 = cpu_to_le32(swap_temp);
	dtd->buff_ptr1 = cpu_to_le32(swap_temp + 0x1000);
	dtd->buff_ptr2 = cpu_to_le32(swap_temp + 0x2000);
	dtd->buff_ptr3 = cpu_to_le32(swap_temp + 0x3000);
	dtd->buff_ptr4 = cpu_to_le32(swap_temp + 0x4000);

	/* Fill in the transfer size; set active bit */
	swap_temp = ((len << DTD_LENGTH_BIT_POS) | DTD_STATUS_ACTIVE) | DTD_IOC;

	writel(cpu_to_le32(swap_temp), &dtd->size_ioc_sts);

	dtd->next_td_ptr = cpu_to_le32(DTD_NEXT_TERMINATE);

	fsl_queue_td(dr, dtd, len ? 0 : 1);

	return 0;
}

enum state {
	state_init = 0,
	state_expect_command,
	state_transfer_data,
	state_complete,
};

#define MAX_TRANSFER_SIZE 2048

static enum state state;
static uint8_t databuf[MAX_TRANSFER_SIZE] __attribute__((aligned(64)));
static int actual;
static int to_transfer;
static void *image;

static void tripwire_handler(struct usb_dr_device *dr, u8 ep_num)
{
	uint32_t val;
	struct ep_queue_head *qh;
	struct ep_queue_head *dQH = (void *)(unsigned long)readl(&dr->endpointlistaddr);
	struct usb_ctrlrequest *ctrl;

	qh = &dQH[ep_num * 2];

	val = readl(&dr->endptsetupstat);
	val |= 1 << ep_num;
	writel(val, &dr->endptsetupstat);

	do {
		val = readl(&dr->usbcmd);
		val |= USB_CMD_SUTW;
		writel(val, &dr->usbcmd);

		ctrl = (void *)qh->setup_buffer;
		if ((ctrl->wValue & 0xff) == 1)
			state = state_expect_command;

	} while (!(readl(&dr->usbcmd) & USB_CMD_SUTW));

	val = readl(&dr->usbcmd);
	val &= ~USB_CMD_SUTW;
	writel(val, &dr->usbcmd);

	fsl_ep_queue(dr, &dtd_data, databuf, MAX_TRANSFER_SIZE);
}

static void dtd_complete_irq(struct usb_dr_device *dr)
{
	struct ep_td_struct *dtd = &dtd_data;
	u32 bit_pos;
	int len;

	/* Clear the bits in the register */
	bit_pos = readl(&dr->endptcomplete);
	writel(bit_pos, &dr->endptcomplete);

	if (!(bit_pos & 1))
		return;

	len = MAX_TRANSFER_SIZE -
		(le32_to_cpu(dtd->size_ioc_sts) >> DTD_LENGTH_BIT_POS);

	if (state == state_expect_command) {
		state = state_transfer_data;
		to_transfer = databuf[8] << 24 |
				databuf[9] << 16 |
				databuf[10] << 8 |
				databuf[11];
	} else {
		memcpy(image + actual, &databuf[1], len - 1);
		actual += len - 1;
		to_transfer -= len - 1;

		if (to_transfer <= 0)
			state = state_complete;
	}

	fsl_ep_queue(dr, &dtd_status, NULL, 0);
}

static int usb_irq(struct usb_dr_device *dr)
{
	uint32_t irq_src = readl(&dr->usbsts);

	irq_src &= ~0x80;

	if (!irq_src)
		return -EAGAIN;

	/* Clear notification bits */
	writel(irq_src, &dr->usbsts);

	/* USB Interrupt */
	if (irq_src & USB_STS_INT) {
		/* Setup package, we only support ep0 as control ep */
		if (readl(&dr->endptsetupstat) & EP_SETUP_STATUS_EP0)
			tripwire_handler(dr, 0);

		/* completion of dtd */
		if (readl(&dr->endptcomplete))
			dtd_complete_irq(dr);
	}

	if (state == state_complete)
		return 0;
	else
		return -EAGAIN;
}

int imx_barebox_load_usb(void __iomem *dr, void *dest)
{
	int ret;

	image = dest;

	while (1) {
		ret = usb_irq(dr);
		if (!ret)
			break;
	}

	return 0;
}

int imx_barebox_start_usb(void __iomem *dr, void *dest)
{
	void __noreturn (*bb)(void);
	int ret;

	ret = imx_barebox_load_usb(dr, dest);
	if (ret)
		return ret;

	printf("Downloading complete, start barebox\n");
	bb = dest;
	bb();
}

int imx6_barebox_load_usb(void *dest)
{
	return imx_barebox_load_usb(IOMEM(MX6_OTG_BASE_ADDR), dest);
}

int imx6_barebox_start_usb(void *dest)
{
	return imx_barebox_start_usb(IOMEM(MX6_OTG_BASE_ADDR), dest);
}

int imx7_barebox_load_usb(void *dest)
{
	return imx_barebox_load_usb(IOMEM(MX7_OTG1_BASE_ADDR), dest);
}

int imx7_barebox_start_usb(void *dest)
{
	return imx_barebox_start_usb(IOMEM(MX7_OTG1_BASE_ADDR), dest);
}

int imx8mm_barebox_load_usb(void *dest)
{
	return imx_barebox_load_usb(IOMEM(MX8MM_USB1_BASE_ADDR), dest);
}

int imx8mm_barebox_start_usb(void *dest)
{
	return imx_barebox_start_usb(IOMEM(MX8MM_USB1_BASE_ADDR), dest);
}
