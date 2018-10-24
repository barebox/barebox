#include <common.h>
#include <dma.h>
#include <errno.h>
#include <dma.h>
#include <init.h>
#include <clock.h>
#include <usb/ch9.h>
#include <usb/gadget.h>
#include <usb/fsl_usb2.h>
#include <io.h>
#include <asm/byteorder.h>
#include <linux/err.h>

#include <asm/mmu.h>

/* ### define USB registers here
 */
#define USB_MAX_CTRL_PAYLOAD		64
#define USB_DR_SYS_OFFSET		0x400

 /* USB DR device mode registers (Little Endian) */
struct usb_dr_device {
	/* Capability register */
	u8 res1[256];
	u16 caplength;		/* Capability Register Length */
	u16 hciversion;		/* Host Controller Interface Version */
	u32 hcsparams;		/* Host Controller Structual Parameters */
	u32 hccparams;		/* Host Controller Capability Parameters */
	u8 res2[20];
	u32 dciversion;		/* Device Controller Interface Version */
	u32 dccparams;		/* Device Controller Capability Parameters */
	u8 res3[24];
	/* Operation register */
	u32 usbcmd;		/* USB Command Register */
	u32 usbsts;		/* USB Status Register */
	u32 usbintr;		/* USB Interrupt Enable Register */
	u32 frindex;		/* Frame Index Register */
	u8 res4[4];
	u32 deviceaddr;		/* Device Address */
	u32 endpointlistaddr;	/* Endpoint List Address Register */
	u8 res5[4];
	u32 burstsize;		/* Master Interface Data Burst Size Register */
	u32 txttfilltuning;	/* Transmit FIFO Tuning Controls Register */
	u8 res6[24];
	u32 configflag;		/* Configure Flag Register */
	u32 portsc1;		/* Port 1 Status and Control Register */
	u8 res7[28];
	u32 otgsc;		/* On-The-Go Status and Control */
	u32 usbmode;		/* USB Mode Register */
	u32 endptsetupstat;	/* Endpoint Setup Status Register */
	u32 endpointprime;	/* Endpoint Initialization Register */
	u32 endptflush;		/* Endpoint Flush Register */
	u32 endptstatus;	/* Endpoint Status Register */
	u32 endptcomplete;	/* Endpoint Complete Register */
	u32 endptctrl[6];	/* Endpoint Control Registers */
};

/* ep0 transfer state */
#define WAIT_FOR_SETUP          0
#define DATA_STATE_XMIT         1
#define DATA_STATE_NEED_ZLP     2
#define WAIT_FOR_OUT_STATUS     3
#define DATA_STATE_RECV         4

/* Device Controller Capability Parameter register */
#define DCCPARAMS_DC				0x00000080
#define DCCPARAMS_DEN_MASK			0x0000001f

/* Frame Index Register Bit Masks */
#define	USB_FRINDEX_MASKS			0x3fff
/* USB CMD  Register Bit Masks */
#define  USB_CMD_RUN_STOP                     0x00000001
#define  USB_CMD_CTRL_RESET                   0x00000002
#define  USB_CMD_PERIODIC_SCHEDULE_EN         0x00000010
#define  USB_CMD_ASYNC_SCHEDULE_EN            0x00000020
#define  USB_CMD_INT_AA_DOORBELL              0x00000040
#define  USB_CMD_ASP                          0x00000300
#define  USB_CMD_ASYNC_SCH_PARK_EN            0x00000800
#define  USB_CMD_SUTW                         0x00002000
#define  USB_CMD_ATDTW                        0x00004000
#define  USB_CMD_ITC                          0x00FF0000

/* bit 15,3,2 are frame list size */
#define  USB_CMD_FRAME_SIZE_1024              0x00000000
#define  USB_CMD_FRAME_SIZE_512               0x00000004
#define  USB_CMD_FRAME_SIZE_256               0x00000008
#define  USB_CMD_FRAME_SIZE_128               0x0000000C
#define  USB_CMD_FRAME_SIZE_64                0x00008000
#define  USB_CMD_FRAME_SIZE_32                0x00008004
#define  USB_CMD_FRAME_SIZE_16                0x00008008
#define  USB_CMD_FRAME_SIZE_8                 0x0000800C

/* bit 9-8 are async schedule park mode count */
#define  USB_CMD_ASP_00                       0x00000000
#define  USB_CMD_ASP_01                       0x00000100
#define  USB_CMD_ASP_10                       0x00000200
#define  USB_CMD_ASP_11                       0x00000300
#define  USB_CMD_ASP_BIT_POS                  8

/* bit 23-16 are interrupt threshold control */
#define  USB_CMD_ITC_NO_THRESHOLD             0x00000000
#define  USB_CMD_ITC_1_MICRO_FRM              0x00010000
#define  USB_CMD_ITC_2_MICRO_FRM              0x00020000
#define  USB_CMD_ITC_4_MICRO_FRM              0x00040000
#define  USB_CMD_ITC_8_MICRO_FRM              0x00080000
#define  USB_CMD_ITC_16_MICRO_FRM             0x00100000
#define  USB_CMD_ITC_32_MICRO_FRM             0x00200000
#define  USB_CMD_ITC_64_MICRO_FRM             0x00400000
#define  USB_CMD_ITC_BIT_POS                  16

/* USB STS Register Bit Masks */
#define  USB_STS_INT                          0x00000001
#define  USB_STS_ERR                          0x00000002
#define  USB_STS_PORT_CHANGE                  0x00000004
#define  USB_STS_FRM_LST_ROLL                 0x00000008
#define  USB_STS_SYS_ERR                      0x00000010
#define  USB_STS_IAA                          0x00000020
#define  USB_STS_RESET                        0x00000040
#define  USB_STS_SOF                          0x00000080
#define  USB_STS_SUSPEND                      0x00000100
#define  USB_STS_HC_HALTED                    0x00001000
#define  USB_STS_RCL                          0x00002000
#define  USB_STS_PERIODIC_SCHEDULE            0x00004000
#define  USB_STS_ASYNC_SCHEDULE               0x00008000

/* USB INTR Register Bit Masks */
#define  USB_INTR_INT_EN                      0x00000001
#define  USB_INTR_ERR_INT_EN                  0x00000002
#define  USB_INTR_PTC_DETECT_EN               0x00000004
#define  USB_INTR_FRM_LST_ROLL_EN             0x00000008
#define  USB_INTR_SYS_ERR_EN                  0x00000010
#define  USB_INTR_ASYN_ADV_EN                 0x00000020
#define  USB_INTR_RESET_EN                    0x00000040
#define  USB_INTR_SOF_EN                      0x00000080
#define  USB_INTR_DEVICE_SUSPEND              0x00000100

/* Device Address bit masks */
#define  USB_DEVICE_ADDRESS_MASK              0xFE000000
#define  USB_DEVICE_ADDRESS_BIT_POS           25

/* endpoint list address bit masks */
#define USB_EP_LIST_ADDRESS_MASK              0xfffff800

/* PORTSCX  Register Bit Masks */
#define  PORTSCX_CURRENT_CONNECT_STATUS       0x00000001
#define  PORTSCX_CONNECT_STATUS_CHANGE        0x00000002
#define  PORTSCX_PORT_ENABLE                  0x00000004
#define  PORTSCX_PORT_EN_DIS_CHANGE           0x00000008
#define  PORTSCX_OVER_CURRENT_ACT             0x00000010
#define  PORTSCX_OVER_CURRENT_CHG             0x00000020
#define  PORTSCX_PORT_FORCE_RESUME            0x00000040
#define  PORTSCX_PORT_SUSPEND                 0x00000080
#define  PORTSCX_PORT_RESET                   0x00000100
#define  PORTSCX_LINE_STATUS_BITS             0x00000C00
#define  PORTSCX_PORT_POWER                   0x00001000
#define  PORTSCX_PORT_INDICTOR_CTRL           0x0000C000
#define  PORTSCX_PORT_TEST_CTRL               0x000F0000
#define  PORTSCX_WAKE_ON_CONNECT_EN           0x00100000
#define  PORTSCX_WAKE_ON_CONNECT_DIS          0x00200000
#define  PORTSCX_WAKE_ON_OVER_CURRENT         0x00400000
#define  PORTSCX_PHY_LOW_POWER_SPD            0x00800000
#define  PORTSCX_PORT_FORCE_FULL_SPEED        0x01000000
#define  PORTSCX_PORT_SPEED_MASK              0x0C000000
#define  PORTSCX_PORT_WIDTH                   0x10000000
#define  PORTSCX_PHY_TYPE_SEL                 0xC0000000

/* bit 11-10 are line status */
#define  PORTSCX_LINE_STATUS_SE0              0x00000000
#define  PORTSCX_LINE_STATUS_JSTATE           0x00000400
#define  PORTSCX_LINE_STATUS_KSTATE           0x00000800
#define  PORTSCX_LINE_STATUS_UNDEF            0x00000C00
#define  PORTSCX_LINE_STATUS_BIT_POS          10

/* bit 15-14 are port indicator control */
#define  PORTSCX_PIC_OFF                      0x00000000
#define  PORTSCX_PIC_AMBER                    0x00004000
#define  PORTSCX_PIC_GREEN                    0x00008000
#define  PORTSCX_PIC_UNDEF                    0x0000C000
#define  PORTSCX_PIC_BIT_POS                  14

/* bit 19-16 are port test control */
#define  PORTSCX_PTC_DISABLE                  0x00000000
#define  PORTSCX_PTC_JSTATE                   0x00010000
#define  PORTSCX_PTC_KSTATE                   0x00020000
#define  PORTSCX_PTC_SEQNAK                   0x00030000
#define  PORTSCX_PTC_PACKET                   0x00040000
#define  PORTSCX_PTC_FORCE_EN                 0x00050000
#define  PORTSCX_PTC_BIT_POS                  16

/* bit 27-26 are port speed */
#define  PORTSCX_PORT_SPEED_FULL              0x00000000
#define  PORTSCX_PORT_SPEED_LOW               0x04000000
#define  PORTSCX_PORT_SPEED_HIGH              0x08000000
#define  PORTSCX_PORT_SPEED_UNDEF             0x0C000000
#define  PORTSCX_SPEED_BIT_POS                26

/* bit 28 is parallel transceiver width for UTMI interface */
#define  PORTSCX_PTW                          0x10000000
#define  PORTSCX_PTW_8BIT                     0x00000000
#define  PORTSCX_PTW_16BIT                    0x10000000

/* bit 31-30 are port transceiver select */
#define  PORTSCX_PTS_UTMI                     0x00000000
#define  PORTSCX_PTS_ULPI                     0x80000000
#define  PORTSCX_PTS_FSLS                     0xC0000000
#define  PORTSCX_PTS_BIT_POS                  30

/* otgsc Register Bit Masks */
#define  OTGSC_CTRL_VUSB_DISCHARGE            0x00000001
#define  OTGSC_CTRL_VUSB_CHARGE               0x00000002
#define  OTGSC_CTRL_OTG_TERM                  0x00000008
#define  OTGSC_CTRL_DATA_PULSING              0x00000010
#define  OTGSC_STS_USB_ID                     0x00000100
#define  OTGSC_STS_A_VBUS_VALID               0x00000200
#define  OTGSC_STS_A_SESSION_VALID            0x00000400
#define  OTGSC_STS_B_SESSION_VALID            0x00000800
#define  OTGSC_STS_B_SESSION_END              0x00001000
#define  OTGSC_STS_1MS_TOGGLE                 0x00002000
#define  OTGSC_STS_DATA_PULSING               0x00004000
#define  OTGSC_INTSTS_USB_ID                  0x00010000
#define  OTGSC_INTSTS_A_VBUS_VALID            0x00020000
#define  OTGSC_INTSTS_A_SESSION_VALID         0x00040000
#define  OTGSC_INTSTS_B_SESSION_VALID         0x00080000
#define  OTGSC_INTSTS_B_SESSION_END           0x00100000
#define  OTGSC_INTSTS_1MS                     0x00200000
#define  OTGSC_INTSTS_DATA_PULSING            0x00400000
#define  OTGSC_INTR_USB_ID                    0x01000000
#define  OTGSC_INTR_A_VBUS_VALID              0x02000000
#define  OTGSC_INTR_A_SESSION_VALID           0x04000000
#define  OTGSC_INTR_B_SESSION_VALID           0x08000000
#define  OTGSC_INTR_B_SESSION_END             0x10000000
#define  OTGSC_INTR_1MS_TIMER                 0x20000000
#define  OTGSC_INTR_DATA_PULSING              0x40000000

/* USB MODE Register Bit Masks */
#define  USB_MODE_CTRL_MODE_IDLE              0x00000000
#define  USB_MODE_CTRL_MODE_DEVICE            0x00000002
#define  USB_MODE_CTRL_MODE_HOST              0x00000003
#define  USB_MODE_CTRL_MODE_RSV               0x00000001
#define  USB_MODE_CTRL_MODE_MASK              0x00000003
#define  USB_MODE_SETUP_LOCK_OFF              0x00000008
#define  USB_MODE_STREAM_DISABLE              0x00000010
/* Endpoint Flush Register */
#define EPFLUSH_TX_OFFSET		      0x00010000
#define EPFLUSH_RX_OFFSET		      0x00000000

/* Endpoint Setup Status bit masks */
#define  EP_SETUP_STATUS_MASK                 0x0000003F
#define  EP_SETUP_STATUS_EP0		      0x00000001

/* ENDPOINTCTRLx  Register Bit Masks */
#define  EPCTRL_TX_ENABLE                     0x00800000
#define  EPCTRL_TX_DATA_TOGGLE_RST            0x00400000	/* Not EP0 */
#define  EPCTRL_TX_DATA_TOGGLE_INH            0x00200000	/* Not EP0 */
#define  EPCTRL_TX_TYPE                       0x000C0000
#define  EPCTRL_TX_DATA_SOURCE                0x00020000	/* Not EP0 */
#define  EPCTRL_TX_EP_STALL                   0x00010000
#define  EPCTRL_RX_ENABLE                     0x00000080
#define  EPCTRL_RX_DATA_TOGGLE_RST            0x00000040	/* Not EP0 */
#define  EPCTRL_RX_DATA_TOGGLE_INH            0x00000020	/* Not EP0 */
#define  EPCTRL_RX_TYPE                       0x0000000C
#define  EPCTRL_RX_DATA_SINK                  0x00000002	/* Not EP0 */
#define  EPCTRL_RX_EP_STALL                   0x00000001

/* bit 19-18 and 3-2 are endpoint type */
#define  EPCTRL_EP_TYPE_CONTROL               0
#define  EPCTRL_EP_TYPE_ISO                   1
#define  EPCTRL_EP_TYPE_BULK                  2
#define  EPCTRL_EP_TYPE_INTERRUPT             3
#define  EPCTRL_TX_EP_TYPE_SHIFT              18
#define  EPCTRL_RX_EP_TYPE_SHIFT              2

/* SNOOPn Register Bit Masks */
#define  SNOOP_ADDRESS_MASK                   0xFFFFF000
#define  SNOOP_SIZE_ZERO                      0x00	/* snooping disable */
#define  SNOOP_SIZE_4KB                       0x0B	/* 4KB snoop size */
#define  SNOOP_SIZE_8KB                       0x0C
#define  SNOOP_SIZE_16KB                      0x0D
#define  SNOOP_SIZE_32KB                      0x0E
#define  SNOOP_SIZE_64KB                      0x0F
#define  SNOOP_SIZE_128KB                     0x10
#define  SNOOP_SIZE_256KB                     0x11
#define  SNOOP_SIZE_512KB                     0x12
#define  SNOOP_SIZE_1MB                       0x13
#define  SNOOP_SIZE_2MB                       0x14
#define  SNOOP_SIZE_4MB                       0x15
#define  SNOOP_SIZE_8MB                       0x16
#define  SNOOP_SIZE_16MB                      0x17
#define  SNOOP_SIZE_32MB                      0x18
#define  SNOOP_SIZE_64MB                      0x19
#define  SNOOP_SIZE_128MB                     0x1A
#define  SNOOP_SIZE_256MB                     0x1B
#define  SNOOP_SIZE_512MB                     0x1C
#define  SNOOP_SIZE_1GB                       0x1D
#define  SNOOP_SIZE_2GB                       0x1E	/* 2GB snoop size */

/* pri_ctrl Register Bit Masks */
#define  PRI_CTRL_PRI_LVL1                    0x0000000C
#define  PRI_CTRL_PRI_LVL0                    0x00000003

/* si_ctrl Register Bit Masks */
#define  SI_CTRL_ERR_DISABLE                  0x00000010
#define  SI_CTRL_IDRC_DISABLE                 0x00000008
#define  SI_CTRL_RD_SAFE_EN                   0x00000004
#define  SI_CTRL_RD_PREFETCH_DISABLE          0x00000002
#define  SI_CTRL_RD_PREFEFETCH_VAL            0x00000001

/* control Register Bit Masks */
#define  USB_CTRL_IOENB                       0x00000004
#define  USB_CTRL_ULPI_INT0EN                 0x00000001

/* Endpoint Queue Head data struct
 * Rem: all the variables of qh are LittleEndian Mode
 * and NEXT_POINTER_MASK should operate on a LittleEndian, Phy Addr
 */
struct ep_queue_head {
	u32 max_pkt_length;	/* Mult(31-30) , Zlt(29) , Max Pkt len
				   and IOS(15) */
	u32 curr_dtd_ptr;	/* Current dTD Pointer(31-5) */
	u32 next_dtd_ptr;	/* Next dTD Pointer(31-5), T(0) */
	u32 size_ioc_int_sts;	/* Total bytes (30-16), IOC (15),
				   MultO(11-10), STS (7-0)  */
	u32 buff_ptr0;		/* Buffer pointer Page 0 (31-12) */
	u32 buff_ptr1;		/* Buffer pointer Page 1 (31-12) */
	u32 buff_ptr2;		/* Buffer pointer Page 2 (31-12) */
	u32 buff_ptr3;		/* Buffer pointer Page 3 (31-12) */
	u32 buff_ptr4;		/* Buffer pointer Page 4 (31-12) */
	u32 res1;
	u8 setup_buffer[8];	/* Setup data 8 bytes */
	u32 res2[4];
};

/* Endpoint Queue Head Bit Masks */
#define  EP_QUEUE_HEAD_MULT_POS               30
#define  EP_QUEUE_HEAD_ZLT_SEL                0x20000000
#define  EP_QUEUE_HEAD_MAX_PKT_LEN_POS        16
#define  EP_QUEUE_HEAD_MAX_PKT_LEN(ep_info)   (((ep_info)>>16)&0x07ff)
#define  EP_QUEUE_HEAD_IOS                    0x00008000
#define  EP_QUEUE_HEAD_NEXT_TERMINATE         0x00000001
#define  EP_QUEUE_HEAD_IOC                    0x00008000
#define  EP_QUEUE_HEAD_MULTO                  0x00000C00
#define  EP_QUEUE_HEAD_STATUS_HALT	      0x00000040
#define  EP_QUEUE_HEAD_STATUS_ACTIVE          0x00000080
#define  EP_QUEUE_CURRENT_OFFSET_MASK         0x00000FFF
#define  EP_QUEUE_HEAD_NEXT_POINTER_MASK      0xFFFFFFE0
#define  EP_QUEUE_FRINDEX_MASK                0x000007FF
#define  EP_MAX_LENGTH_TRANSFER               0x4000

/* Endpoint Transfer Descriptor data struct */
/* Rem: all the variables of td are LittleEndian Mode */
struct ep_td_struct {
	u32 next_td_ptr;	/* Next TD pointer(31-5), T(0) set
				   indicate invalid */
	u32 size_ioc_sts;	/* Total bytes (30-16), IOC (15),
				   MultO(11-10), STS (7-0)  */
	u32 buff_ptr0;		/* Buffer pointer Page 0 */
	u32 buff_ptr1;		/* Buffer pointer Page 1 */
	u32 buff_ptr2;		/* Buffer pointer Page 2 */
	u32 buff_ptr3;		/* Buffer pointer Page 3 */
	u32 buff_ptr4;		/* Buffer pointer Page 4 */
	u32 res;
	/* 32 bytes */
	dma_addr_t td_dma;	/* dma address for this td */
	/* virtual address of next td specified in next_td_ptr */
	struct ep_td_struct *next_td_virt;
};

/* Endpoint Transfer Descriptor bit Masks */
#define  DTD_NEXT_TERMINATE                   0x00000001
#define  DTD_IOC                              0x00008000
#define  DTD_STATUS_ACTIVE                    0x00000080
#define  DTD_STATUS_HALTED                    0x00000040
#define  DTD_STATUS_DATA_BUFF_ERR             0x00000020
#define  DTD_STATUS_TRANSACTION_ERR           0x00000008
#define  DTD_RESERVED_FIELDS                  0x80007300
#define  DTD_ADDR_MASK                        0xFFFFFFE0
#define  DTD_PACKET_SIZE                      0x7FFF0000
#define  DTD_LENGTH_BIT_POS                   16
#define  DTD_ERROR_MASK                       (DTD_STATUS_HALTED | \
                                               DTD_STATUS_DATA_BUFF_ERR | \
                                               DTD_STATUS_TRANSACTION_ERR)
/* Alignment requirements; must be a power of two */
#define DTD_ALIGNMENT				0x20
#define QH_ALIGNMENT				2048

/* Controller dma boundary */
#define UDC_DMA_BOUNDARY			0x1000

/*-------------------------------------------------------------------------*/

/* ### driver private data
 */
struct fsl_req {
	struct usb_request req;
	struct list_head queue;
	/* ep_queue() func will add
	   a request->queue into a udc_ep->queue 'd tail */
	struct fsl_ep *ep;

	struct ep_td_struct *head, *tail;	/* For dTD List
						   cpu endian Virtual addr */
	unsigned int dtd_count;
};

#define REQ_UNCOMPLETE			1

struct fsl_ep {
	struct usb_ep ep;
	struct list_head queue;
	struct fsl_udc *udc;
	struct ep_queue_head *qh;
	const struct usb_endpoint_descriptor *desc;
	struct usb_gadget *gadget;

	char name[14];
	unsigned stopped:1;
};

#define EP_DIR_IN	1
#define EP_DIR_OUT	0

struct fsl_udc {
	struct usb_gadget gadget;
	struct usb_gadget_driver *driver;
	struct completion *done;	/* to make sure release() is done */
	struct fsl_ep *eps;
	unsigned int max_ep;
	unsigned int irq;

	struct usb_ctrlrequest local_setup_buff;
	struct otg_transceiver *transceiver;
	unsigned softconnect:1;
	unsigned vbus_active:1;
	unsigned stopped:1;
	unsigned remote_wakeup:1;

	struct ep_queue_head *ep_qh;	/* Endpoints Queue-Head */
	struct fsl_req *status_req;	/* ep0 status request */
	enum fsl_usb2_phy_modes phy_mode;

	size_t ep_qh_size;		/* size after alignment adjustment*/
	dma_addr_t ep_qh_dma;		/* dma address of QH */

	u32 max_pipes;          /* Device max pipes */
	u32 resume_state;	/* USB state to resume */
	u32 usb_state;		/* USB current state */
	u32 ep0_state;		/* Endpoint zero state */
	u32 ep0_dir;		/* Endpoint zero direction: can be
				   USB_DIR_IN or USB_DIR_OUT */
	u8 device_address;	/* Device USB address */
};

static inline struct fsl_udc *to_fsl_udc(struct usb_gadget *gadget)
{
	return container_of(gadget, struct fsl_udc, gadget);
}

/*-------------------------------------------------------------------------*/

#ifdef DEBUG
#define DBG(fmt, args...) 	printk(KERN_DEBUG "[%s]  " fmt "\n", \
				__func__, ## args)
#else
#define DBG(fmt, args...)	do{}while(0)
#endif

#if 0
static void dump_msg(const char *label, const u8 * buf, unsigned int length)
{
	unsigned int start, num, i;
	char line[52], *p;

	if (length >= 512)
		return;
	DBG("%s, length %u:\n", label, length);
	start = 0;
	while (length > 0) {
		num = min(length, 16u);
		p = line;
		for (i = 0; i < num; ++i) {
			if (i == 8)
				*p++ = ' ';
			sprintf(p, " %02x", buf[i]);
			p += 3;
		}
		*p = 0;
		printk(KERN_DEBUG "%6x: %s\n", start, line);
		buf += num;
		start += num;
		length -= num;
	}
}
#endif

#ifdef VERBOSE
#define VDBG		DBG
#else
#define VDBG(stuff...)	do{}while(0)
#endif

#define ERR(stuff...)		pr_err("udc: " stuff)
#define WARNING(stuff...)		pr_warning("udc: " stuff)
#define INFO(stuff...)		pr_info("udc: " stuff)

/*-------------------------------------------------------------------------*/

/* ### Add board specific defines here
 */

/*
 * ### pipe direction macro from device view
 */
#define USB_RECV	0	/* OUT EP */
#define USB_SEND	1	/* IN EP */

/*
 * ### internal used help routines.
 */
#define ep_index(EP)		((EP)->desc->bEndpointAddress&0xF)
#define ep_maxpacket(EP)	((EP)->ep.maxpacket)
#define ep_is_in(EP)	( (ep_index(EP) == 0) ? (EP->udc->ep0_dir == \
			USB_DIR_IN ):((EP)->desc->bEndpointAddress \
			& USB_DIR_IN)==USB_DIR_IN)
#define get_ep_by_pipe(udc, pipe)	((pipe == 1)? &udc->eps[0]: \
					&udc->eps[pipe])
#define get_pipe_by_windex(windex)	((windex & USB_ENDPOINT_NUMBER_MASK) \
					* 2 + ((windex & USB_DIR_IN) ? 1 : 0))
#define get_pipe_by_ep(EP)	(ep_index(EP) * 2 + ep_is_in(EP))

static struct usb_dr_device __iomem *dr_regs;

static const struct usb_endpoint_descriptor
fsl_ep0_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	0,
	.bmAttributes =		USB_ENDPOINT_XFER_CONTROL,
	.wMaxPacketSize =	USB_MAX_CTRL_PAYLOAD,
};

static void fsl_ep_fifo_flush(struct usb_ep *_ep);

/*-----------------------------------------------------------------
 * done() - retire a request; caller blocked irqs
 * @status : request status to be set, only works when
 *	request is still in progress.
 *--------------------------------------------------------------*/
static void done(struct fsl_ep *ep, struct fsl_req *req, int status)
{
	struct fsl_udc *udc = NULL;
	unsigned char stopped = ep->stopped;
	struct ep_td_struct *curr_td, *next_td;
	int j;

	udc = (struct fsl_udc *)ep->udc;
	/* Removed the req from fsl_ep->queue */
	list_del_init(&req->queue);

	/* req.status should be set as -EINPROGRESS in ep_queue() */
	if (req->req.status == -EINPROGRESS)
		req->req.status = status;
	else
		status = req->req.status;

	/* Free dtd for the request */
	next_td = req->head;
	for (j = 0; j < req->dtd_count; j++) {
		curr_td = next_td;
		if (j != req->dtd_count - 1) {
			next_td = curr_td->next_td_virt;
		}
		dma_free_coherent(curr_td, 0, sizeof(struct ep_td_struct));
	}

	dma_sync_single_for_cpu((unsigned long)req->req.buf, req->req.length,
				DMA_BIDIRECTIONAL);

	if (status && (status != -ESHUTDOWN))
		VDBG("complete %s req %p stat %d len %u/%u",
			ep->ep.name, &req->req, status,
			req->req.actual, req->req.length);

	ep->stopped = 1;

	/* complete() is from gadget layer,
	 * eg fsg->bulk_in_complete() */
	if (req->req.complete)
		req->req.complete(&ep->ep, &req->req);

	ep->stopped = stopped;
}

/*-----------------------------------------------------------------
 * nuke(): delete all requests related to this ep
 * called with spinlock held
 *--------------------------------------------------------------*/
static void nuke(struct fsl_ep *ep, int status)
{
	ep->stopped = 1;

	/* Flush fifo */
	fsl_ep_fifo_flush(&ep->ep);

	/* Whether this eq has request linked */
	while (!list_empty(&ep->queue)) {
		struct fsl_req *req = NULL;

		req = list_entry(ep->queue.next, struct fsl_req, queue);
		done(ep, req, status);
	}
}

static int dr_controller_setup(struct fsl_udc *udc)
{
	unsigned int tmp, portctrl, ep_num;
	unsigned int max_no_of_ep;
	uint64_t to;

	/* Config PHY interface */
	portctrl = readl(&dr_regs->portsc1);
	portctrl &= ~(PORTSCX_PHY_TYPE_SEL | PORTSCX_PORT_WIDTH);
	switch (udc->phy_mode) {
	case FSL_USB2_PHY_ULPI:
		portctrl |= PORTSCX_PTS_ULPI;
		break;
	case FSL_USB2_PHY_UTMI_WIDE:
		portctrl |= PORTSCX_PTW_16BIT;
		/* fall through */
	case FSL_USB2_PHY_UTMI:
		portctrl |= PORTSCX_PTS_UTMI;
		break;
	case FSL_USB2_PHY_SERIAL:
		portctrl |= PORTSCX_PTS_FSLS;
		break;
	case FSL_USB2_PHY_NONE:
		break;
	default:
		return -EINVAL;
	}
	if (udc->phy_mode != FSL_USB2_PHY_NONE)
		writel(portctrl, &dr_regs->portsc1);

	/* Stop and reset the usb controller */
	tmp = readl(&dr_regs->usbcmd);
	tmp &= ~USB_CMD_RUN_STOP;
	writel(tmp, &dr_regs->usbcmd);

	tmp = readl(&dr_regs->usbcmd);
	tmp |= USB_CMD_CTRL_RESET;
	writel(tmp, &dr_regs->usbcmd);

	/* Wait for reset to complete */
	to = get_time_ns();
	while (readl(&dr_regs->usbcmd) & USB_CMD_CTRL_RESET) {
		if (is_timeout(to, SECOND)) {
			printf("timeout waiting fo reset\n");
			return -ENODEV;
		}
	}

	/* Set the controller as device mode */
	tmp = readl(&dr_regs->usbmode);
	tmp &= ~USB_MODE_CTRL_MODE_MASK;	/* clear mode bits */
	tmp |= USB_MODE_CTRL_MODE_DEVICE;
	/* Disable Setup Lockout */
	tmp |= USB_MODE_SETUP_LOCK_OFF;
	writel(tmp, &dr_regs->usbmode);

	/* Clear the setup status */
	writel(0, &dr_regs->usbsts);

	tmp = udc->ep_qh_dma;
	tmp &= USB_EP_LIST_ADDRESS_MASK;
	writel(tmp, &dr_regs->endpointlistaddr);

	max_no_of_ep = (0x0000001F & readl(&dr_regs->dccparams));
	for (ep_num = 1; ep_num < max_no_of_ep; ep_num++) {
		tmp = readl(&dr_regs->endptctrl[ep_num]);
		tmp &= ~(EPCTRL_TX_TYPE | EPCTRL_RX_TYPE);
		tmp |= (EPCTRL_EP_TYPE_BULK << EPCTRL_TX_EP_TYPE_SHIFT)
		| (EPCTRL_EP_TYPE_BULK << EPCTRL_RX_EP_TYPE_SHIFT);
		writel(tmp, &dr_regs->endptctrl[ep_num]);
	}
	VDBG("vir[qh_base] is %p phy[qh_base] is 0x%8x reg is 0x%8x",
		udc->ep_qh, (int)tmp,
		readl(&dr_regs->endpointlistaddr));

	return 0;
}

/* Enable DR irq and set controller to run state */
static void dr_controller_run(struct fsl_udc *udc)
{
	u32 temp;

	/* Enable DR irq reg */
	temp = USB_INTR_INT_EN | USB_INTR_ERR_INT_EN
		| USB_INTR_PTC_DETECT_EN | USB_INTR_RESET_EN
		| USB_INTR_DEVICE_SUSPEND | USB_INTR_SYS_ERR_EN;

	writel(temp, &dr_regs->usbintr);

	/* Clear stopped bit */
	udc->stopped = 0;

	/* Set the controller as device mode */
	temp = readl(&dr_regs->usbmode);
	temp |= USB_MODE_CTRL_MODE_DEVICE;
	writel(temp, &dr_regs->usbmode);

	/* Set controller to Run */
	temp = readl(&dr_regs->usbcmd);
	temp |= USB_CMD_RUN_STOP;
	writel(temp, &dr_regs->usbcmd);

	return;
}

static void dr_controller_stop(struct fsl_udc *udc)
{
	unsigned int tmp;

	/* disable all INTR */
	writel(0, &dr_regs->usbintr);

	/* Set stopped bit for isr */
	udc->stopped = 1;

	/* disable IO output */
/*	usb_sys_regs->control = 0; */

	/* set controller to Stop */
	tmp = readl(&dr_regs->usbcmd);
	tmp &= ~USB_CMD_RUN_STOP;
	writel(tmp, &dr_regs->usbcmd);

	return;
}

static void dr_ep_setup(unsigned char ep_num, unsigned char dir,
			unsigned char ep_type)
{
	unsigned int tmp_epctrl = 0;

	tmp_epctrl = readl(&dr_regs->endptctrl[ep_num]);
	if (dir) {
		if (ep_num)
			tmp_epctrl |= EPCTRL_TX_DATA_TOGGLE_RST;
		tmp_epctrl |= EPCTRL_TX_ENABLE;
		tmp_epctrl &= ~EPCTRL_TX_TYPE;
		tmp_epctrl |= ((unsigned int)(ep_type)
				<< EPCTRL_TX_EP_TYPE_SHIFT);
	} else {
		if (ep_num)
			tmp_epctrl |= EPCTRL_RX_DATA_TOGGLE_RST;
		tmp_epctrl |= EPCTRL_RX_ENABLE;
		tmp_epctrl &= ~EPCTRL_RX_TYPE;
		tmp_epctrl |= ((unsigned int)(ep_type)
				<< EPCTRL_RX_EP_TYPE_SHIFT);
	}

	writel(tmp_epctrl, &dr_regs->endptctrl[ep_num]);
}

static void
dr_ep_change_stall(unsigned char ep_num, unsigned char dir, int value)
{
	u32 tmp_epctrl = 0;

	tmp_epctrl = readl(&dr_regs->endptctrl[ep_num]);

	if (value) {
		/* set the stall bit */
		if (dir)
			tmp_epctrl |= EPCTRL_TX_EP_STALL;
		else
			tmp_epctrl |= EPCTRL_RX_EP_STALL;
	} else {
		/* clear the stall bit and reset data toggle */
		if (dir) {
			tmp_epctrl &= ~EPCTRL_TX_EP_STALL;
			tmp_epctrl |= EPCTRL_TX_DATA_TOGGLE_RST;
		} else {
			tmp_epctrl &= ~EPCTRL_RX_EP_STALL;
			tmp_epctrl |= EPCTRL_RX_DATA_TOGGLE_RST;
		}
	}
	writel(tmp_epctrl, &dr_regs->endptctrl[ep_num]);
}

/* Get stall status of a specific ep
   Return: 0: not stalled; 1:stalled */
static int dr_ep_get_stall(unsigned char ep_num, unsigned char dir)
{
	u32 epctrl;

	epctrl = readl(&dr_regs->endptctrl[ep_num]);
	if (dir)
		return (epctrl & EPCTRL_TX_EP_STALL) ? 1 : 0;
	else
		return (epctrl & EPCTRL_RX_EP_STALL) ? 1 : 0;
}

/*------------------------------------------------------------------
* struct_ep_qh_setup(): set the Endpoint Capabilites field of QH
 * @zlt: Zero Length Termination Select (1: disable; 0: enable)
 * @mult: Mult field
 ------------------------------------------------------------------*/
static void struct_ep_qh_setup(struct fsl_udc *udc, unsigned char ep_num,
		unsigned char dir, unsigned char ep_type,
		unsigned int max_pkt_len,
		unsigned int zlt, unsigned char mult)
{
	struct ep_queue_head *p_QH = &udc->ep_qh[2 * ep_num + dir];
	unsigned int tmp = 0;

	/* set the Endpoint Capabilites in QH */
	switch (ep_type) {
	case USB_ENDPOINT_XFER_CONTROL:
		/* Interrupt On Setup (IOS). for control ep  */
		tmp = (max_pkt_len << EP_QUEUE_HEAD_MAX_PKT_LEN_POS)
			| EP_QUEUE_HEAD_IOS;
		break;
	case USB_ENDPOINT_XFER_ISOC:
		tmp = (max_pkt_len << EP_QUEUE_HEAD_MAX_PKT_LEN_POS)
			| (mult << EP_QUEUE_HEAD_MULT_POS);
		break;
	case USB_ENDPOINT_XFER_BULK:
	case USB_ENDPOINT_XFER_INT:
		tmp = max_pkt_len << EP_QUEUE_HEAD_MAX_PKT_LEN_POS;
		break;
	default:
		VDBG("error ep type is %d", ep_type);
		return;
	}
	if (zlt)
		tmp |= EP_QUEUE_HEAD_ZLT_SEL;

	p_QH->max_pkt_length = cpu_to_le32(tmp);
	p_QH->next_dtd_ptr = 1;
	p_QH->size_ioc_int_sts = 0;

	return;
}

/* Setup qh structure and ep register for ep0. */
static void ep0_setup(struct fsl_udc *udc)
{
	/* the intialization of an ep includes: fields in QH, Regs,
	 * fsl_ep struct */
	struct_ep_qh_setup(udc, 0, USB_RECV, USB_ENDPOINT_XFER_CONTROL,
			USB_MAX_CTRL_PAYLOAD, 1, 0);
	struct_ep_qh_setup(udc, 0, USB_SEND, USB_ENDPOINT_XFER_CONTROL,
			USB_MAX_CTRL_PAYLOAD, 1, 0);
	dr_ep_setup(0, USB_RECV, USB_ENDPOINT_XFER_CONTROL);
	dr_ep_setup(0, USB_SEND, USB_ENDPOINT_XFER_CONTROL);

	return;

}

/***********************************************************************
		Endpoint Management Functions
***********************************************************************/

/*-------------------------------------------------------------------------
 * when configurations are set, or when interface settings change
 * for example the do_set_interface() in gadget layer,
 * the driver will enable or disable the relevant endpoints
 * ep0 doesn't use this routine. It is always enabled.
-------------------------------------------------------------------------*/
static int fsl_ep_enable(struct usb_ep *_ep,
		const struct usb_endpoint_descriptor *desc)
{
	struct fsl_udc *udc = NULL;
	struct fsl_ep *ep = NULL;
	unsigned short max = 0;
	unsigned char mult = 0, zlt;
	int retval = -EINVAL;

	ep = container_of(_ep, struct fsl_ep, ep);

	/* catch various bogus parameters */
	if (!_ep || !desc || ep->desc
			|| (desc->bDescriptorType != USB_DT_ENDPOINT))
		return -EINVAL;

	udc = ep->udc;

	if (!udc->driver || (udc->gadget.speed == USB_SPEED_UNKNOWN))
		return -ESHUTDOWN;

	max = le16_to_cpu(desc->wMaxPacketSize);

	/* Disable automatic zlp generation.  Driver is reponsible to indicate
	 * explicitly through req->req.zero.  This is needed to enable multi-td
	 * request. */
	zlt = 1;

	/* Assume the max packet size from gadget is always correct */
	switch (desc->bmAttributes & 0x03) {
	case USB_ENDPOINT_XFER_CONTROL:
	case USB_ENDPOINT_XFER_BULK:
	case USB_ENDPOINT_XFER_INT:
		/* mult = 0.  Execute N Transactions as demonstrated by
		 * the USB variable length packet protocol where N is
		 * computed using the Maximum Packet Length (dQH) and
		 * the Total Bytes field (dTD) */
		mult = 0;
		break;
	case USB_ENDPOINT_XFER_ISOC:
		/* Calculate transactions needed for high bandwidth iso */
		mult = (unsigned char)(1 + ((max >> 11) & 0x03));
		max = max & 0x7ff;	/* bit 0~10 */
		/* 3 transactions at most */
		if (mult > 3)
			goto en_done;
		break;
	default:
		goto en_done;
	}

	ep->ep.maxpacket = max;
	ep->desc = desc;
	ep->stopped = 0;

	/* Controller related setup */
	/* Init EPx Queue Head (Ep Capabilites field in QH
	 * according to max, zlt, mult) */
	struct_ep_qh_setup(udc, (unsigned char) ep_index(ep),
			(unsigned char) ((desc->bEndpointAddress & USB_DIR_IN)
					?  USB_SEND : USB_RECV),
			(unsigned char) (desc->bmAttributes
					& USB_ENDPOINT_XFERTYPE_MASK),
			max, zlt, mult);

	/* Init endpoint ctrl register */
	dr_ep_setup((unsigned char) ep_index(ep),
			(unsigned char) ((desc->bEndpointAddress & USB_DIR_IN)
					? USB_SEND : USB_RECV),
			(unsigned char) (desc->bmAttributes
					& USB_ENDPOINT_XFERTYPE_MASK));

	retval = 0;

	VDBG("enabled %s (ep%d%s) maxpacket %d",ep->ep.name,
			ep->desc->bEndpointAddress & 0x0f,
			(desc->bEndpointAddress & USB_DIR_IN)
				? "in" : "out", max);
en_done:
	return retval;
}

/*---------------------------------------------------------------------
 * @ep : the ep being unconfigured. May not be ep0
 * Any pending and uncomplete req will complete with status (-ESHUTDOWN)
*---------------------------------------------------------------------*/
static int fsl_ep_disable(struct usb_ep *_ep)
{
	struct fsl_udc *udc = NULL;
	struct fsl_ep *ep = NULL;
	u32 epctrl;
	int ep_num;

	ep = container_of(_ep, struct fsl_ep, ep);
	if (!_ep || !ep->desc) {
		VDBG("%s not enabled", _ep ? ep->ep.name : NULL);
		return -EINVAL;
	}

	/* disable ep on controller */
	ep_num = ep_index(ep);
	epctrl = readl(&dr_regs->endptctrl[ep_num]);
	if (ep_is_in(ep)) {
		epctrl &= ~(EPCTRL_TX_ENABLE | EPCTRL_TX_TYPE);
		epctrl |= EPCTRL_EP_TYPE_BULK << EPCTRL_TX_EP_TYPE_SHIFT;
	} else {
		epctrl &= ~(EPCTRL_RX_ENABLE | EPCTRL_TX_TYPE);
		epctrl |= EPCTRL_EP_TYPE_BULK << EPCTRL_RX_EP_TYPE_SHIFT;
	}
	writel(epctrl, &dr_regs->endptctrl[ep_num]);

	udc = (struct fsl_udc *)ep->udc;

	/* nuke all pending requests (does flush) */
	nuke(ep, -ESHUTDOWN);

	ep->desc = NULL;
	ep->stopped = 1;

	VDBG("disabled %s OK", _ep->name);
	return 0;
}

static void fsl_ep_fifo_flush(struct usb_ep *_ep)
{
	struct fsl_ep *ep;
	int ep_num, ep_dir;
	u32 bits;
	uint64_t to;

	if (!_ep) {
		return;
	} else {
		ep = container_of(_ep, struct fsl_ep, ep);
		if (!ep->desc)
			return;
	}
	ep_num = ep_index(ep);
	ep_dir = ep_is_in(ep) ? USB_SEND : USB_RECV;

	if (ep_num == 0)
		bits = (1 << 16) | 1;
	else if (ep_dir == USB_SEND)
		bits = 1 << (16 + ep_num);
	else
		bits = 1 << ep_num;

	do {
		writel(bits, &dr_regs->endptflush);

		/* Wait until flush complete */
		to = get_time_ns();
		while (readl(&dr_regs->endptflush)) {
			if (is_timeout(to, SECOND)) {
				printf("timeout waiting fo flush\n");
				return;
			}
		}
		/* See if we need to flush again */
	} while (readl(&dr_regs->endptstatus) & bits);
}

/*---------------------------------------------------------------------
 * allocate a request object used by this endpoint
 * the main operation is to insert the req->queue to the eq->queue
 * Returns the request, or null if one could not be allocated
*---------------------------------------------------------------------*/
static struct usb_request *
fsl_alloc_request(struct usb_ep *_ep)
{
	struct fsl_req *req;

	req = xzalloc(sizeof *req);

	INIT_LIST_HEAD(&req->queue);

	return &req->req;
}

static void fsl_free_request(struct usb_ep *_ep, struct usb_request *_req)
{
	struct fsl_req *req = NULL;

	req = container_of(_req, struct fsl_req, req);

	if (!list_empty(&req->queue)) {
		printk("%s: Freeing queued request\n", __func__);
		dump_stack();
	}

	if (_req)
		kfree(req);
}

/*-------------------------------------------------------------------------*/
static void fsl_queue_td(struct fsl_ep *ep, struct fsl_req *req)
{
	int i = ep_index(ep) * 2 + ep_is_in(ep);
	u32 temp, bitmask, tmp_stat;
	struct ep_queue_head *dQH = &ep->udc->ep_qh[i];

	/* VDBG("QH addr Register 0x%8x", dr_regs->endpointlistaddr);
	VDBG("ep_qh[%d] addr is 0x%8x", i, (u32)&(ep->udc->ep_qh[i])); */

	bitmask = ep_is_in(ep)
		? (1 << (ep_index(ep) + 16))
		: (1 << (ep_index(ep)));

	/* check if the pipe is empty */
	if (!(list_empty(&ep->queue))) {
		/* Add td to the end */
		struct fsl_req *lastreq;
		lastreq = list_entry(ep->queue.prev, struct fsl_req, queue);
		lastreq->tail->next_td_ptr =
			cpu_to_le32(req->head->td_dma & DTD_ADDR_MASK);
		/* Read prime bit, if 1 goto done */
		if (readl(&dr_regs->endpointprime) & bitmask)
			goto out;

		do {
			/* Set ATDTW bit in USBCMD */
			temp = readl(&dr_regs->usbcmd);
			writel(temp | USB_CMD_ATDTW, &dr_regs->usbcmd);

			/* Read correct status bit */
			tmp_stat = readl(&dr_regs->endptstatus) & bitmask;

		} while (!(readl(&dr_regs->usbcmd) & USB_CMD_ATDTW));

		/* Write ATDTW bit to 0 */
		temp = readl(&dr_regs->usbcmd);
		writel(temp & ~USB_CMD_ATDTW, &dr_regs->usbcmd);

		if (tmp_stat)
			goto out;
	}

	/* Write dQH next pointer and terminate bit to 0 */
	temp = req->head->td_dma & EP_QUEUE_HEAD_NEXT_POINTER_MASK;
	dQH->next_dtd_ptr = cpu_to_le32(temp);

	/* Clear active and halt bit */
	temp = cpu_to_le32(~(EP_QUEUE_HEAD_STATUS_ACTIVE
			| EP_QUEUE_HEAD_STATUS_HALT));
	dQH->size_ioc_int_sts &= temp;

	/* Ensure that updates to the QH will occure before priming. */

	/* Prime endpoint by writing 1 to ENDPTPRIME */
	temp = ep_is_in(ep)
		? (1 << (ep_index(ep) + 16))
		: (1 << (ep_index(ep)));
	writel(temp, &dr_regs->endpointprime);
out:
	return;
}

/* Fill in the dTD structure
 * @req: request that the transfer belongs to
 * @length: return actually data length of the dTD
 * @dma: return dma address of the dTD
 * @is_last: return flag if it is the last dTD of the request
 * return: pointer to the built dTD */
static struct ep_td_struct *fsl_build_dtd(struct fsl_req *req, unsigned *length,
		dma_addr_t *dma, int *is_last)
{
	u32 swap_temp;
	struct ep_td_struct *dtd;

	/* how big will this transfer be? */
	*length = min(req->req.length - req->req.actual,
			(unsigned)EP_MAX_LENGTH_TRANSFER);

	dtd = dma_alloc_coherent(sizeof(struct ep_td_struct),
				 DMA_ADDRESS_BROKEN);
	if (dtd == NULL)
		return dtd;

	*dma = (dma_addr_t)virt_to_phys(dtd);
	dtd->td_dma = *dma;
	/* Clear reserved field */
	swap_temp = cpu_to_le32(dtd->size_ioc_sts);
	swap_temp &= ~DTD_RESERVED_FIELDS;
	dtd->size_ioc_sts = cpu_to_le32(swap_temp);

	/* Init all of buffer page pointers */
	swap_temp = (u32) (req->req.buf + req->req.actual);
	dtd->buff_ptr0 = cpu_to_le32(swap_temp);
	dtd->buff_ptr1 = cpu_to_le32(swap_temp + 0x1000);
	dtd->buff_ptr2 = cpu_to_le32(swap_temp + 0x2000);
	dtd->buff_ptr3 = cpu_to_le32(swap_temp + 0x3000);
	dtd->buff_ptr4 = cpu_to_le32(swap_temp + 0x4000);

	req->req.actual += *length;

	/* zlp is needed if req->req.zero is set */
	if (req->req.zero) {
		if (*length == 0 || (*length % req->ep->ep.maxpacket) != 0)
			*is_last = 1;
		else
			*is_last = 0;
	} else if (req->req.length == req->req.actual)
		*is_last = 1;
	else
		*is_last = 0;

	if ((*is_last) == 0)
		VDBG("multi-dtd request!");
	/* Fill in the transfer size; set active bit */
	swap_temp = ((*length << DTD_LENGTH_BIT_POS) | DTD_STATUS_ACTIVE);

	/* Enable interrupt for the last dtd of a request */
	if (*is_last && !req->req.no_interrupt)
		swap_temp |= DTD_IOC;

	dtd->size_ioc_sts = cpu_to_le32(swap_temp);

	VDBG("length = %d address= 0x%x", *length, (int)*dma);

	return dtd;
}

/* Generate dtd chain for a request */
static int fsl_req_to_dtd(struct fsl_req *req)
{
	unsigned	count;
	int		is_last;
	int		is_first =1;
	struct ep_td_struct	*last_dtd = NULL, *dtd;
	dma_addr_t dma;

	do {
		dtd = fsl_build_dtd(req, &count, &dma, &is_last);
		if (dtd == NULL)
			return -ENOMEM;

		if (is_first) {
			is_first = 0;
			req->head = dtd;
		} else {
			last_dtd->next_td_ptr = cpu_to_le32(dma);
			last_dtd->next_td_virt = dtd;
		}
		last_dtd = dtd;

		req->dtd_count++;
	} while (!is_last);

	dtd->next_td_ptr = cpu_to_le32(DTD_NEXT_TERMINATE);

	req->tail = dtd;

	return 0;
}

/* queues (submits) an I/O request to an endpoint */
static int
fsl_ep_queue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct fsl_ep *ep = container_of(_ep, struct fsl_ep, ep);
	struct fsl_req *req = container_of(_req, struct fsl_req, req);
	struct fsl_udc *udc;
	int is_iso = 0;

	/* catch various bogus parameters */
	if (!_req || !req->req.complete || !req->req.buf
			|| !list_empty(&req->queue)) {
		VDBG("%s, bad params", __func__);
		return -EINVAL;
	}
	if (unlikely(!_ep || !ep->desc)) {
		VDBG("%s, bad ep", __func__);
		return -EINVAL;
	}
	if (ep->desc->bmAttributes == USB_ENDPOINT_XFER_ISOC) {
		if (req->req.length > ep->ep.maxpacket)
			return -EMSGSIZE;
		is_iso = 1;
	}

	udc = ep->udc;
	if (!udc->driver || udc->gadget.speed == USB_SPEED_UNKNOWN)
		return -ESHUTDOWN;

	req->ep = ep;

	dma_sync_single_for_device((unsigned long)req->req.buf, req->req.length,
				   DMA_BIDIRECTIONAL);

	req->req.status = -EINPROGRESS;
	req->req.actual = 0;
	req->dtd_count = 0;

	/* build dtds and push them to device queue */
	if (!fsl_req_to_dtd(req)) {
		fsl_queue_td(ep, req);
	} else {
		return -ENOMEM;
	}

	/* Update ep0 state */
	if ((ep_index(ep) == 0))
		udc->ep0_state = DATA_STATE_XMIT;

	/* irq handler advances the queue */
	if (req != NULL)
		list_add_tail(&req->queue, &ep->queue);

	return 0;
}

/* dequeues (cancels, unlinks) an I/O request from an endpoint */
static int fsl_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct fsl_ep *ep = container_of(_ep, struct fsl_ep, ep);
	struct fsl_req *req;
	int ep_num, stopped, ret = 0;
	u32 epctrl;

	if (!_ep || !_req || !ep->desc)
		return -EINVAL;

	stopped = ep->stopped;

	/* Stop the ep before we deal with the queue */
	ep->stopped = 1;
	ep_num = ep_index(ep);
	epctrl = readl(&dr_regs->endptctrl[ep_num]);
	if (ep_is_in(ep))
		epctrl &= ~EPCTRL_TX_ENABLE;
	else
		epctrl &= ~EPCTRL_RX_ENABLE;
	writel(epctrl, &dr_regs->endptctrl[ep_num]);

	/* make sure it's actually queued on this endpoint */
	list_for_each_entry(req, &ep->queue, queue) {
		if (&req->req == _req)
			break;
	}
	if (&req->req != _req) {
		ret = -EINVAL;
		goto out;
	}

	/* The request is in progress, or completed but not dequeued */
	if (ep->queue.next == &req->queue) {
		_req->status = -ECONNRESET;
		fsl_ep_fifo_flush(_ep);	/* flush current transfer */

		/* The request isn't the last request in this ep queue */
		if (req->queue.next != &ep->queue) {
			struct ep_queue_head *qh;
			struct fsl_req *next_req;

			qh = ep->qh;
			next_req = list_entry(req->queue.next, struct fsl_req,
					queue);

			/* Point the QH to the first TD of next request */
			writel((u32) next_req->head, &qh->curr_dtd_ptr);
		}

		/* The request hasn't been processed, patch up the TD chain */
	} else {
		struct fsl_req *prev_req;

		prev_req = list_entry(req->queue.prev, struct fsl_req, queue);
		writel(readl(&req->tail->next_td_ptr),
				&prev_req->tail->next_td_ptr);

	}

	done(ep, req, -ECONNRESET);

	/* Enable EP */
out:	epctrl = readl(&dr_regs->endptctrl[ep_num]);
	if (ep_is_in(ep))
		epctrl |= EPCTRL_TX_ENABLE;
	else
		epctrl |= EPCTRL_RX_ENABLE;
	writel(epctrl, &dr_regs->endptctrl[ep_num]);
	ep->stopped = stopped;

	return ret;
}

/*-------------------------------------------------------------------------*/

/*-----------------------------------------------------------------
 * modify the endpoint halt feature
 * @ep: the non-isochronous endpoint being stalled
 * @value: 1--set halt  0--clear halt
 * Returns zero, or a negative error code.
*----------------------------------------------------------------*/
static int fsl_ep_set_halt(struct usb_ep *_ep, int value)
{
	struct fsl_ep *ep = NULL;
	int status = -EOPNOTSUPP;	/* operation not supported */
	unsigned char ep_dir = 0, ep_num = 0;
	struct fsl_udc *udc = NULL;

	ep = container_of(_ep, struct fsl_ep, ep);
	udc = ep->udc;
	if (!_ep || !ep->desc) {
		status = -EINVAL;
		goto out;
	}

	if (ep->desc->bmAttributes == USB_ENDPOINT_XFER_ISOC) {
		status = -EOPNOTSUPP;
		goto out;
	}

	/* Attempt to halt IN ep will fail if any transfer requests
	 * are still queue */
	if (value && ep_is_in(ep) && !list_empty(&ep->queue)) {
		status = -EAGAIN;
		goto out;
	}

	status = 0;
	ep_dir = ep_is_in(ep) ? USB_SEND : USB_RECV;
	ep_num = (unsigned char)(ep_index(ep));
	dr_ep_change_stall(ep_num, ep_dir, value);

	if (ep_index(ep) == 0) {
		udc->ep0_state = WAIT_FOR_SETUP;
		udc->ep0_dir = 0;
	}
out:
	VDBG(" %s %s halt stat %d", ep->ep.name,
			value ?  "set" : "clear", status);

	return status;
}

static struct usb_ep_ops fsl_ep_ops = {
	.enable = fsl_ep_enable,
	.disable = fsl_ep_disable,

	.alloc_request = fsl_alloc_request,
	.free_request = fsl_free_request,

	.queue = fsl_ep_queue,
	.dequeue = fsl_ep_dequeue,

	.set_halt = fsl_ep_set_halt,
	.fifo_flush = fsl_ep_fifo_flush,	/* flush fifo */
};

static void udc_reset_ep_queue(struct fsl_udc *udc, u8 pipe)
{
	struct fsl_ep *ep = get_ep_by_pipe(udc, pipe);

	if (ep->name)
		nuke(ep, -ESHUTDOWN);
}

/* Clear up all ep queues */
static int reset_queues(struct fsl_udc *udc)
{
	u8 pipe;

	for (pipe = 0; pipe < udc->max_pipes; pipe++)
		udc_reset_ep_queue(udc, pipe);

	/* report disconnect; the driver is already quiesced */
	udc->driver->disconnect(&udc->gadget);

	return 0;
}

/* Tripwire mechanism to ensure a setup packet payload is extracted without
 * being corrupted by another incoming setup packet */
static void tripwire_handler(struct fsl_udc *udc, u8 ep_num, u8 *buffer_ptr)
{
	u32 temp;
	struct ep_queue_head *qh;

	qh = &udc->ep_qh[ep_num * 2 + EP_DIR_OUT];

	/* Clear bit in ENDPTSETUPSTAT */
	temp = readl(&dr_regs->endptsetupstat);
	writel(temp | (1 << ep_num), &dr_regs->endptsetupstat);

	/* while a hazard exists when setup package arrives */
	do {
		/* Set Setup Tripwire */
		temp = readl(&dr_regs->usbcmd);
		writel(temp | USB_CMD_SUTW, &dr_regs->usbcmd);

		/* Copy the setup packet to local buffer */
		memcpy(buffer_ptr, (u8 *) qh->setup_buffer, 8);
	} while (!(readl(&dr_regs->usbcmd) & USB_CMD_SUTW));

	/* Clear Setup Tripwire */
	temp = readl(&dr_regs->usbcmd);
	writel(temp & ~USB_CMD_SUTW, &dr_regs->usbcmd);
}

/* Set protocol stall on ep0, protocol stall will automatically be cleared
   on new transaction */
static void ep0stall(struct fsl_udc *udc)
{
	u32 tmp;

	/* must set tx and rx to stall at the same time */
	tmp = readl(&dr_regs->endptctrl[0]);
	tmp |= EPCTRL_TX_EP_STALL | EPCTRL_RX_EP_STALL;
	writel(tmp, &dr_regs->endptctrl[0]);
	udc->ep0_state = WAIT_FOR_SETUP;
	udc->ep0_dir = 0;
}

/* Prime a status phase for ep0 */
static int ep0_prime_status(struct fsl_udc *udc, int direction)
{
	struct fsl_req *req = udc->status_req;
	struct fsl_ep *ep;

	if (direction == EP_DIR_IN)
		udc->ep0_dir = USB_DIR_IN;
	else
		udc->ep0_dir = USB_DIR_OUT;

	ep = &udc->eps[0];
	udc->ep0_state = WAIT_FOR_OUT_STATUS;

	req->ep = ep;
	req->req.length = 0;
	req->req.status = -EINPROGRESS;
	req->req.actual = 0;
	req->req.complete = NULL;
	req->dtd_count = 0;

	if (fsl_req_to_dtd(req) == 0)
		fsl_queue_td(ep, req);
	else
		return -ENOMEM;

	list_add_tail(&req->queue, &ep->queue);

	return 0;
}

/*
 * ch9 Set address
 */
static void ch9setaddress(struct fsl_udc *udc, u16 value, u16 index, u16 length)
{
	/* Save the new address to device struct */
	udc->device_address = (u8) value;
	/* Update usb state */
	udc->usb_state = USB_STATE_ADDRESS;
	/* Status phase */
	if (ep0_prime_status(udc, EP_DIR_IN))
		ep0stall(udc);
}

/*
 * ch9 Get status
 */
static void ch9getstatus(struct fsl_udc *udc, u8 request_type, u16 value,
		u16 index, u16 length)
{
	u16 tmp = 0;		/* Status, cpu endian */
	struct fsl_req *req;
	struct fsl_ep *ep;

	ep = &udc->eps[0];

	if ((request_type & USB_RECIP_MASK) == USB_RECIP_DEVICE) {
		/* Get device status */
		tmp = 1 << USB_DEVICE_SELF_POWERED;
		tmp |= udc->remote_wakeup << USB_DEVICE_REMOTE_WAKEUP;
	} else if ((request_type & USB_RECIP_MASK) == USB_RECIP_INTERFACE) {
		/* Get interface status */
		/* We don't have interface information in udc driver */
		tmp = 0;
	} else if ((request_type & USB_RECIP_MASK) == USB_RECIP_ENDPOINT) {
		/* Get endpoint status */
		struct fsl_ep *target_ep;

		target_ep = get_ep_by_pipe(udc, get_pipe_by_windex(index));

		/* stall if endpoint doesn't exist */
		if (!target_ep->desc)
			goto stall;
		tmp = dr_ep_get_stall(ep_index(target_ep), ep_is_in(target_ep))
				<< USB_ENDPOINT_HALT;
	}

	udc->ep0_dir = USB_DIR_IN;
	/* Borrow the per device status_req */
	req = udc->status_req;
	/* Fill in the reqest structure */
	*((u16 *) req->req.buf) = cpu_to_le16(tmp);
	req->ep = ep;
	req->req.length = 2;
	req->req.status = -EINPROGRESS;
	req->req.actual = 0;
	req->req.complete = NULL;
	req->dtd_count = 0;

	/* prime the data phase */
	if ((fsl_req_to_dtd(req) == 0))
		fsl_queue_td(ep, req);
	else			/* no mem */
		goto stall;

	list_add_tail(&req->queue, &ep->queue);
	udc->ep0_state = DATA_STATE_XMIT;
	return;
stall:
	ep0stall(udc);
}

static void setup_received_irq(struct fsl_udc *udc,
		struct usb_ctrlrequest *setup)
{
	u16 wValue = le16_to_cpu(setup->wValue);
	u16 wIndex = le16_to_cpu(setup->wIndex);
	u16 wLength = le16_to_cpu(setup->wLength);

	udc_reset_ep_queue(udc, 0);

	/* We process some stardard setup requests here */
	switch (setup->bRequest) {
	case USB_REQ_GET_STATUS:
		/* Data+Status phase from udc */
		if ((setup->bRequestType & (USB_DIR_IN | USB_TYPE_MASK))
					!= (USB_DIR_IN | USB_TYPE_STANDARD))
			break;
		ch9getstatus(udc, setup->bRequestType, wValue, wIndex, wLength);
		return;

	case USB_REQ_SET_ADDRESS:
		/* Status phase from udc */
		if (setup->bRequestType != (USB_DIR_OUT | USB_TYPE_STANDARD
						| USB_RECIP_DEVICE))
			break;
		ch9setaddress(udc, wValue, wIndex, wLength);
		return;

	case USB_REQ_CLEAR_FEATURE:
	case USB_REQ_SET_FEATURE:
		/* Status phase from udc */
	{
		int rc = -EOPNOTSUPP;

		if ((setup->bRequestType & (USB_RECIP_MASK | USB_TYPE_MASK))
				== (USB_RECIP_ENDPOINT | USB_TYPE_STANDARD)) {
			int pipe = get_pipe_by_windex(wIndex);
			struct fsl_ep *ep;

			if (wValue != 0 || wLength != 0 || pipe > udc->max_ep)
				break;
			ep = get_ep_by_pipe(udc, pipe);

			rc = fsl_ep_set_halt(&ep->ep,
					(setup->bRequest == USB_REQ_SET_FEATURE)
						? 1 : 0);

		} else if ((setup->bRequestType & (USB_RECIP_MASK
				| USB_TYPE_MASK)) == (USB_RECIP_DEVICE
				| USB_TYPE_STANDARD)) {
			/* Note: The driver has not include OTG support yet.
			 * This will be set when OTG support is added */
			if (!gadget_is_otg(&udc->gadget))
				break;
			else if (setup->bRequest == USB_DEVICE_B_HNP_ENABLE)
				udc->gadget.b_hnp_enable = 1;
			else if (setup->bRequest == USB_DEVICE_A_HNP_SUPPORT)
				udc->gadget.a_hnp_support = 1;
			else if (setup->bRequest ==
					USB_DEVICE_A_ALT_HNP_SUPPORT)
				udc->gadget.a_alt_hnp_support = 1;
			else
				break;
			rc = 0;
		} else
			break;

		if (rc == 0) {
			if (ep0_prime_status(udc, EP_DIR_IN))
				ep0stall(udc);
		}
		return;
	}

	default:
		break;
	}

	/* Requests handled by gadget */
	if (wLength) {
		/* Data phase from gadget, status phase from udc */
		udc->ep0_dir = (setup->bRequestType & USB_DIR_IN)
				?  USB_DIR_IN : USB_DIR_OUT;
		if (udc->driver->setup(&udc->gadget,
				&udc->local_setup_buff) < 0)
			ep0stall(udc);
		udc->ep0_state = (setup->bRequestType & USB_DIR_IN)
				?  DATA_STATE_XMIT : DATA_STATE_RECV;
	} else {
		/* No data phase, IN status from gadget */
		udc->ep0_dir = USB_DIR_IN;
		if (udc->driver->setup(&udc->gadget,
				&udc->local_setup_buff) < 0)
			ep0stall(udc);
		udc->ep0_state = WAIT_FOR_OUT_STATUS;
	}
}

/* Process reset interrupt */
static void reset_irq(struct fsl_udc *udc)
{
	u32 temp;
	uint64_t to;

	/* Clear the device address */
	temp = readl(&dr_regs->deviceaddr);
	writel(temp & ~USB_DEVICE_ADDRESS_MASK, &dr_regs->deviceaddr);

	udc->device_address = 0;

	/* Clear usb state */
	udc->resume_state = 0;
	udc->ep0_dir = 0;
	udc->ep0_state = WAIT_FOR_SETUP;
	udc->remote_wakeup = 0;	/* default to 0 on reset */
	udc->gadget.b_hnp_enable = 0;
	udc->gadget.a_hnp_support = 0;
	udc->gadget.a_alt_hnp_support = 0;

	/* Clear all the setup token semaphores */
	temp = readl(&dr_regs->endptsetupstat);
	writel(temp, &dr_regs->endptsetupstat);

	/* Clear all the endpoint complete status bits */
	temp = readl(&dr_regs->endptcomplete);
	writel(temp, &dr_regs->endptcomplete);

	to = get_time_ns();
	while (readl(&dr_regs->endpointprime)) {
		if (is_timeout(to, SECOND)) {
			printf("timeout waiting fo reset\n");
			return;
		}
	}

	/* Write 1s to the flush register */
	writel(0xffffffff, &dr_regs->endptflush);

	if (readl(&dr_regs->portsc1) & PORTSCX_PORT_RESET) {
		/* Reset all the queues, include XD, dTD, EP queue
		 * head and TR Queue */
		reset_queues(udc);
		udc->usb_state = USB_STATE_DEFAULT;
	} else {
		/* initialize usb hw reg except for regs for EP, not
		 * touch usbintr reg */
		dr_controller_setup(udc);

		/* Reset all internal used Queues */
		reset_queues(udc);

		ep0_setup(udc);

		/* Enable DR IRQ reg, Set Run bit, change udc state */
		dr_controller_run(udc);
		udc->usb_state = USB_STATE_ATTACHED;
	}
}

/* Process a port change interrupt */
static void port_change_irq(struct fsl_udc *udc)
{
	u32 speed;

	/* Bus resetting is finished */
	if (!(readl(&dr_regs->portsc1) & PORTSCX_PORT_RESET)) {
		/* Get the speed */
		speed = (readl(&dr_regs->portsc1)
				& PORTSCX_PORT_SPEED_MASK);
		switch (speed) {
		case PORTSCX_PORT_SPEED_HIGH:
			udc->gadget.speed = USB_SPEED_HIGH;
			break;
		case PORTSCX_PORT_SPEED_FULL:
			udc->gadget.speed = USB_SPEED_FULL;
			break;
		case PORTSCX_PORT_SPEED_LOW:
			udc->gadget.speed = USB_SPEED_LOW;
			break;
		default:
			udc->gadget.speed = USB_SPEED_UNKNOWN;
			break;
		}
	}

	/* Update USB state */
	if (!udc->resume_state)
		udc->usb_state = USB_STATE_DEFAULT;
}

/* Process request for Data or Status phase of ep0
 * prime status phase if needed */
static void ep0_req_complete(struct fsl_udc *udc, struct fsl_ep *ep0,
		struct fsl_req *req)
{
	if (udc->usb_state == USB_STATE_ADDRESS) {
		/* Set the new address */
		u32 new_address = (u32) udc->device_address;
		writel(new_address << USB_DEVICE_ADDRESS_BIT_POS,
				&dr_regs->deviceaddr);
	}

	done(ep0, req, 0);

	switch (udc->ep0_state) {
	case DATA_STATE_XMIT:
		/* receive status phase */
		if (ep0_prime_status(udc, EP_DIR_OUT))
			ep0stall(udc);
		break;
	case DATA_STATE_RECV:
		/* send status phase */
		if (ep0_prime_status(udc, EP_DIR_IN))
			ep0stall(udc);
		break;
	case WAIT_FOR_OUT_STATUS:
		udc->ep0_state = WAIT_FOR_SETUP;
		break;
	case WAIT_FOR_SETUP:
		ERR("Unexpect ep0 packets\n");
		break;
	default:
		ep0stall(udc);
		break;
	}
}

/* process-ep_req(): free the completed Tds for this req */
static int process_ep_req(struct fsl_udc *udc, int pipe,
		struct fsl_req *curr_req)
{
	struct ep_td_struct *curr_td;
	int	td_complete, actual, remaining_length, j, tmp;
	int	status = 0;
	int	errors = 0;
	struct  ep_queue_head *curr_qh = &udc->ep_qh[pipe];
	int direction = pipe % 2;

	curr_td = curr_req->head;
	td_complete = 0;
	actual = curr_req->req.length;

	for (j = 0; j < curr_req->dtd_count; j++) {
		remaining_length = (le32_to_cpu(curr_td->size_ioc_sts)
					& DTD_PACKET_SIZE)
				>> DTD_LENGTH_BIT_POS;
		actual -= remaining_length;

		if ((errors = le32_to_cpu(curr_td->size_ioc_sts) &
						DTD_ERROR_MASK)) {
			if (errors & DTD_STATUS_HALTED) {
				ERR("dTD error %08x QH=%d\n", errors, pipe);
				/* Clear the errors and Halt condition */
				tmp = le32_to_cpu(curr_qh->size_ioc_int_sts);
				tmp &= ~errors;
				curr_qh->size_ioc_int_sts = cpu_to_le32(tmp);
				status = -EPIPE;
				/* FIXME: continue with next queued TD? */

				break;
			}
			if (errors & DTD_STATUS_DATA_BUFF_ERR) {
				VDBG("Transfer overflow");
				status = -EPROTO;
				break;
			} else if (errors & DTD_STATUS_TRANSACTION_ERR) {
				VDBG("ISO error");
				status = -EILSEQ;
				break;
			} else
				ERR("Unknown error has occured (0x%x)!\n",
					errors);

		} else if (le32_to_cpu(curr_td->size_ioc_sts)
				& DTD_STATUS_ACTIVE) {
			VDBG("Request not complete");
			status = REQ_UNCOMPLETE;
			return status;
		} else if (remaining_length) {
			if (direction) {
				VDBG("Transmit dTD remaining length not zero");
				status = -EPROTO;
				break;
			} else {
				td_complete++;
				break;
			}
		} else {
			td_complete++;
			VDBG("dTD transmitted successful");
		}

		if (j != curr_req->dtd_count - 1)
			curr_td = (struct ep_td_struct *)curr_td->next_td_virt;
	}

	if (status)
		return status;

	curr_req->req.actual = actual;

	return 0;
}

/* Process a DTD completion interrupt */
static void dtd_complete_irq(struct fsl_udc *udc)
{
	u32 bit_pos;
	int i, ep_num, direction, bit_mask, status;
	struct fsl_ep *curr_ep;
	struct fsl_req *curr_req, *temp_req;

	/* Clear the bits in the register */
	bit_pos = readl(&dr_regs->endptcomplete);
	writel(bit_pos, &dr_regs->endptcomplete);

	if (!bit_pos)
		return;

	for (i = 0; i < udc->max_ep * 2; i++) {
		ep_num = i >> 1;
		direction = i % 2;

		bit_mask = 1 << (ep_num + 16 * direction);

		if (!(bit_pos & bit_mask))
			continue;

		curr_ep = get_ep_by_pipe(udc, i);

		/* If the ep is configured */
		if (curr_ep->name == NULL) {
			WARNING("Invalid EP?");
			continue;
		}

		/* process the req queue until an uncomplete request */
		list_for_each_entry_safe(curr_req, temp_req, &curr_ep->queue,
				queue) {
			status = process_ep_req(udc, i, curr_req);

			VDBG("status of process_ep_req= %d, ep = %d\n",
					status, ep_num);
			if (status == REQ_UNCOMPLETE)
				break;
			/* write back status to req */
			curr_req->req.status = status;

			if (ep_num == 0) {
				ep0_req_complete(udc, curr_ep, curr_req);
				break;
			} else
				done(curr_ep, curr_req, status);
		}
	}
}

/*
 * USB device controller interrupt handler
 */
static void fsl_udc_gadget_poll(struct usb_gadget *gadget)
{
	struct fsl_udc *udc = to_fsl_udc(gadget);
	u32 irq_src;

	/* Disable ISR for OTG host mode */
	if (udc->stopped)
		return;

	irq_src = readl(&dr_regs->usbsts) & readl(&dr_regs->usbintr);
	/* Clear notification bits */
	writel(irq_src, &dr_regs->usbsts);

	/* USB Interrupt */
	if (irq_src & USB_STS_INT) {
		/* Setup package, we only support ep0 as control ep */
		if (readl(&dr_regs->endptsetupstat) & EP_SETUP_STATUS_EP0) {
			tripwire_handler(udc, 0,
					(u8 *) (&udc->local_setup_buff));
			setup_received_irq(udc, &udc->local_setup_buff);
		}

		/* completion of dtd */
		if (readl(&dr_regs->endptcomplete))
			dtd_complete_irq(udc);
	}

	/* Port Change */
	if (irq_src & USB_STS_PORT_CHANGE)
		port_change_irq(udc);

	/* Reset Received */
	if (irq_src & USB_STS_RESET)
		reset_irq(udc);

	/* Sleep Enable (Suspend) */
	if (irq_src & USB_STS_SUSPEND)
		udc->driver->disconnect(gadget);

	if (irq_src & (USB_STS_ERR | USB_STS_SYS_ERR))
		printf("Error IRQ %x\n", irq_src);
}

/*----------------------------------------------------------------*
 * Hook to gadget drivers
 * Called by initialization code of gadget drivers
*----------------------------------------------------------------*/
static int fsl_udc_start(struct usb_gadget *gadget, struct usb_gadget_driver *driver)
{
	struct fsl_udc *udc = to_fsl_udc(gadget);

	/*
	 * We currently have PHY no driver which could call vbus_connect,
	 * so when the USB gadget core calls usb_gadget_connect() the
	 * driver decides to disable the device because it has no vbus.
	 * Work around this by enabling vbus here.
	 */
	usb_gadget_vbus_connect(gadget);

	/* hook up the driver */
	udc->driver = driver;

	/* Enable DR IRQ reg and Set usbcmd reg  Run bit */
	dr_controller_run(udc);
	udc->usb_state = USB_STATE_ATTACHED;
	udc->ep0_state = WAIT_FOR_SETUP;
	udc->ep0_dir = 0;

	return 0;
}

/* Disconnect from gadget driver */
static int fsl_udc_stop(struct usb_gadget *gadget, struct usb_gadget_driver *driver)
{
	struct fsl_udc *udc = to_fsl_udc(gadget);
	struct fsl_ep *loop_ep;

	/* stop DR, disable intr */
	dr_controller_stop(udc);

	/* in fact, no needed */
	udc->usb_state = USB_STATE_ATTACHED;
	udc->ep0_state = WAIT_FOR_SETUP;
	udc->ep0_dir = 0;

	/* stand operation */
	udc->gadget.speed = USB_SPEED_UNKNOWN;
	nuke(&udc->eps[0], -ESHUTDOWN);
	list_for_each_entry(loop_ep, &udc->gadget.ep_list,
			ep.ep_list)
		nuke(loop_ep, -ESHUTDOWN);

	return 0;
}

static int struct_udc_setup(struct fsl_udc *udc,
		struct device_d *dev)
{
	struct fsl_usb2_platform_data *pdata = dev->platform_data;
	size_t size;

	if (pdata)
		udc->phy_mode = pdata->phy_mode;
	else
		udc->phy_mode = FSL_USB2_PHY_NONE;

	udc->eps = kzalloc(sizeof(struct fsl_ep) * udc->max_ep, GFP_KERNEL);
	if (!udc->eps) {
		ERR("malloc fsl_ep failed\n");
		return -1;
	}

	/* initialized QHs, take care of alignment */
	size = udc->max_ep * sizeof(struct ep_queue_head);
	if (size < QH_ALIGNMENT)
		size = QH_ALIGNMENT;
	else if ((size % QH_ALIGNMENT) != 0) {
		size += QH_ALIGNMENT + 1;
		size &= ~(QH_ALIGNMENT - 1);
	}

	udc->ep_qh = dma_alloc_coherent(size, DMA_ADDRESS_BROKEN);
	if (!udc->ep_qh) {
		ERR("malloc QHs for udc failed\n");
		kfree(udc->eps);
		return -1;
	}
	udc->ep_qh_dma = (dma_addr_t)virt_to_phys(udc->ep_qh);

	udc->ep_qh_size = size;

	/* Initialize ep0 status request structure */
	/* FIXME: fsl_alloc_request() ignores ep argument */
	udc->status_req = container_of(fsl_alloc_request(NULL),
			struct fsl_req, req);
	/* allocate a small amount of memory to get valid address */
	udc->status_req->req.buf = dma_alloc(8);
	udc->status_req->req.length = 8;
	udc->resume_state = USB_STATE_NOTATTACHED;
	udc->usb_state = USB_STATE_POWERED;
	udc->ep0_dir = 0;
	udc->remote_wakeup = 0;	/* default to 0 on reset */

	return 0;
}

/*----------------------------------------------------------------------
 * Get the current frame number (from DR frame_index Reg )
 *----------------------------------------------------------------------*/
static int fsl_get_frame(struct usb_gadget *gadget)
{
	return (int)(readl(&dr_regs->frindex) & USB_FRINDEX_MASKS);
}

/*-----------------------------------------------------------------------
 * Tries to wake up the host connected to this gadget
 -----------------------------------------------------------------------*/
static int fsl_wakeup(struct usb_gadget *gadget)
{
	struct fsl_udc *udc = container_of(gadget, struct fsl_udc, gadget);
	u32 portsc;

	/* Remote wakeup feature not enabled by host */
	if (!udc->remote_wakeup)
		return -EINVAL;

	portsc = readl(&dr_regs->portsc1);
	/* not suspended? */
	if (!(portsc & PORTSCX_PORT_SUSPEND))
		return 0;
	/* trigger force resume */
	portsc |= PORTSCX_PORT_FORCE_RESUME;
	writel(portsc, &dr_regs->portsc1);
	return 0;
}

static int can_pullup(struct fsl_udc *udc)
{
	return udc->driver && udc->softconnect && udc->vbus_active;
}

/* Notify controller that VBUS is powered, Called by whatever
   detects VBUS sessions */
static int fsl_vbus_session(struct usb_gadget *gadget, int is_active)
{
	struct fsl_udc	*udc;

	udc = container_of(gadget, struct fsl_udc, gadget);
	VDBG("VBUS %s", is_active ? "on" : "off");
	udc->vbus_active = (is_active != 0);
	if (can_pullup(udc))
		writel((readl(&dr_regs->usbcmd) | USB_CMD_RUN_STOP),
				&dr_regs->usbcmd);
	else
		writel((readl(&dr_regs->usbcmd) & ~USB_CMD_RUN_STOP),
				&dr_regs->usbcmd);
	return 0;
}

/* constrain controller's VBUS power usage
 * This call is used by gadget drivers during SET_CONFIGURATION calls,
 * reporting how much power the device may consume.  For example, this
 * could affect how quickly batteries are recharged.
 *
 * Returns zero on success, else negative errno.
 */
static int fsl_vbus_draw(struct usb_gadget *gadget, unsigned mA)
{
	return -EINVAL;
}

/* Change Data+ pullup status
 * this func is used by usb_gadget_connect/disconnet
 */
static int fsl_pullup(struct usb_gadget *gadget, int is_on)
{
	struct fsl_udc *udc;

	udc = container_of(gadget, struct fsl_udc, gadget);
	udc->softconnect = (is_on != 0);

	if (can_pullup(udc)) {
		writel((readl(&dr_regs->usbcmd) | USB_CMD_RUN_STOP),
				&dr_regs->usbcmd);
	} else {
		writel((readl(&dr_regs->usbcmd) & ~USB_CMD_RUN_STOP),
				&dr_regs->usbcmd);
	}

	return 0;
}

/* defined in gadget.h */
static struct usb_gadget_ops fsl_gadget_ops = {
	.get_frame = fsl_get_frame,
	.wakeup = fsl_wakeup,
/*	.set_selfpowered = fsl_set_selfpowered,	*/ /* Always selfpowered */
	.vbus_session = fsl_vbus_session,
	.vbus_draw = fsl_vbus_draw,
	.pullup = fsl_pullup,
	.udc_start = fsl_udc_start,
	.udc_stop = fsl_udc_stop,
	.udc_poll = fsl_udc_gadget_poll,
};

/*----------------------------------------------------------------
 * Setup the fsl_ep struct for eps
 * Link fsl_ep->ep to gadget->ep_list
 * ep0out is not used so do nothing here
 * ep0in should be taken care
 *--------------------------------------------------------------*/
static int __init struct_ep_setup(struct fsl_udc *udc, unsigned char index,
		char *name, int link)
{
	struct fsl_ep *ep = &udc->eps[index];

	ep->udc = udc;
	strcpy(ep->name, name);
	ep->ep.name = ep->name;

	ep->ep.ops = &fsl_ep_ops;
	ep->stopped = 0;

	/* for ep0: maxP defined in desc
	 * for other eps, maxP is set by epautoconfig() called by gadget layer
	 */
	usb_ep_set_maxpacket_limit(&ep->ep, (unsigned short) ~0);

	/* the queue lists any req for this ep */
	INIT_LIST_HEAD(&ep->queue);

	/* gagdet.ep_list used for ep_autoconfig so no ep0 */
	if (link)
		list_add_tail(&ep->ep.ep_list, &udc->gadget.ep_list);
	ep->gadget = &udc->gadget;
	ep->qh = &udc->ep_qh[index];

	return 0;
}

struct fsl_udc *ci_udc_register(struct device_d *dev, void __iomem *regs)
{
	struct fsl_udc *udc_controller;
	int ret, i;
	u32 dccparams;

	udc_controller = xzalloc(sizeof(*udc_controller));
	udc_controller->stopped = 1;

	dr_regs = regs;

	/* Read Device Controller Capability Parameters register */
	dccparams = readl(&dr_regs->dccparams);
	if (!(dccparams & DCCPARAMS_DC)) {
		dev_err(dev, "This SOC doesn't support device role\n");
		ret = -ENODEV;
		goto err_out;
	}
	/* Get max device endpoints */
	/* DEN is bidirectional ep number, max_ep doubles the number */
	udc_controller->max_ep = (dccparams & DCCPARAMS_DEN_MASK) * 2;

	/* Initialize the udc structure including QH member and other member */
	if (struct_udc_setup(udc_controller, dev)) {
		ERR("Can't initialize udc data structure\n");
		ret = -ENOMEM;
		goto err_out;
	}

	/* initialize usb hw reg except for regs for EP,
	 * leave usbintr reg untouched */
	dr_controller_setup(udc_controller);

	/* Setup gadget structure */
	udc_controller->gadget.ops = &fsl_gadget_ops;
	udc_controller->gadget.ep0 = &udc_controller->eps[0].ep;
	INIT_LIST_HEAD(&udc_controller->gadget.ep_list);
	udc_controller->gadget.speed = USB_SPEED_UNKNOWN;
	udc_controller->gadget.max_speed = USB_SPEED_HIGH;
	udc_controller->gadget.name = "fsl-usb2-udc";

	/* setup QH and epctrl for ep0 */
	ep0_setup(udc_controller);

	/* setup udc->eps[] for ep0 */
	struct_ep_setup(udc_controller, 0, "ep0", 0);
	/* for ep0: the desc defined here;
	 * for other eps, gadget layer called ep_enable with defined desc
	 */
	udc_controller->eps[0].desc = &fsl_ep0_desc;
	usb_ep_set_maxpacket_limit(&udc_controller->eps[0].ep,
				   USB_MAX_CTRL_PAYLOAD);

	/* setup the udc->eps[] for non-control endpoints and link
	 * to gadget.ep_list */
	for (i = 1; i < (int)(udc_controller->max_ep / 2); i++) {
		char name[14];

		sprintf(name, "ep%dout", i);
		struct_ep_setup(udc_controller, i * 2, name, 1);
		sprintf(name, "ep%din", i);
		struct_ep_setup(udc_controller, i * 2 + 1, name, 1);
	}

	ret = usb_add_gadget_udc_release(dev, &udc_controller->gadget,
			NULL);
	if (ret)
		goto err_out;

	return udc_controller;
err_out:
	free(udc_controller);

	return ERR_PTR(ret);
}

void ci_udc_unregister(struct fsl_udc *udc)
{
	usb_del_gadget_udc(&udc->gadget);
	free(udc);
}

static int fsl_udc_probe(struct device_d *dev)
{
	struct fsl_udc *udc;
	void __iomem *regs = dev_request_mem_region(dev, 0);

	if (IS_ERR(regs))
		return PTR_ERR(regs);

	udc = ci_udc_register(dev, regs);
	if (IS_ERR(udc))
		return PTR_ERR(udc);

	dev->priv = udc;

	return 0;
}

static void fsl_udc_remove(struct device_d *dev)
{
	struct fsl_udc *udc = dev->priv;

	ci_udc_unregister(udc);
}

static struct driver_d fsl_udc_driver = {
        .name   = "fsl-udc",
        .probe  = fsl_udc_probe,
	.remove = fsl_udc_remove,
};
device_platform_driver(fsl_udc_driver);
