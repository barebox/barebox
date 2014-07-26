/*
 * xHCI USB 3.0 Specification
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * Some code borrowed from the Linux xHCI driver
 *   Author: Sarah Sharp
 *   Copyright (C) 2008 Intel Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __XHCI_H
#define __XHCI_H

#define NUM_COMMAND_TRBS	8
#define NUM_TRANSFER_TRBS	8
#define NUM_EVENT_SEGM		1	/* only one supported */
#define NUM_EVENT_TRBS		16	/* minimum 16 TRBS */
#define MIN_EP_RINGS		3	/* Control + Bulk In/Out */
#define MAX_EP_RINGS		(MIN_EP_RINGS * USB_MAXCHILDREN)

/* Up to 16 ms to halt an HC */
#define XHCI_MAX_HALT_USEC	(16 * 1000)

/* Command and Status registers offset from the Operational Registers address */
#define XHCI_CMD_OFFSET		0x00
#define XHCI_STS_OFFSET		0x04
/* HCCPARAMS offset from PCI base address */
#define XHCI_HCC_PARAMS_OFFSET	0x10
/* xHCI PCI Configuration Registers */
#define XHCI_SBRN_OFFSET	0x60

/* Max number of USB devices for any host controller - limit in section 6.1 */
#define MAX_HC_SLOTS		256
/* Section 5.3.3 - MaxPorts */
#define MAX_HC_PORTS		127

/*
 * xHCI register interface.
 * This corresponds to the eXtensible Host Controller Interface (xHCI)
 * Revision 0.95 specification
 */

/**
 * struct xhci_cap_regs - xHCI Host Controller Capability Registers.
 * @hc_capbase:		length of the capabilities register and HC version number
 * @hcs_params1:	HCSPARAMS1 - Structural Parameters 1
 * @hcs_params2:	HCSPARAMS2 - Structural Parameters 2
 * @hcs_params3:	HCSPARAMS3 - Structural Parameters 3
 * @hcc_params:		HCCPARAMS - Capability Parameters
 * @db_off:		DBOFF - Doorbell array offset
 * @run_regs_off:	RTSOFF - Runtime register space offset
 */
struct xhci_cap_regs {
	__le32	hc_capbase;
	__le32	hcs_params1;
	__le32	hcs_params2;
	__le32	hcs_params3;
	__le32	hcc_params;
	__le32	db_off;
	__le32	run_regs_off;
	/* Reserved up to (CAPLENGTH - 0x1C) */
};

/* hc_capbase bitmasks */
/* bits 7:0 - how long is the Capabilities register */
#define HC_LENGTH(p)		((p) & 0x00ff)
/* bits 31:16   */
#define HC_VERSION(p)		(((p) >> 16) & 0xffff)

/* HCSPARAMS1 - hcs_params1 - bitmasks */
/* bits 0:7, Max Device Slots */
#define HCS_MAX_SLOTS(p)	(((p) >> 0) & 0xff)
#define HCS_SLOTS_MASK		0xff
/* bits 8:18, Max Interrupters */
#define HCS_MAX_INTRS(p)	(((p) >> 8) & 0x7ff)
/* bits 24:31, Max Ports - max value is 0x7F = 127 ports */
#define HCS_MAX_PORTS(p)	(((p) >> 24) & 0x7f)

/* HCSPARAMS2 - hcs_params2 - bitmasks */
/* bits 0:3, frames or uframes that SW needs to queue transactions
 * ahead of the HW to meet periodic deadlines */
#define HCS_IST(p)		(((p) >> 0) & 0xf)
/* bits 4:7, max number of Event Ring segments */
#define HCS_ERST_MAX(p)		(((p) >> 4) & 0xf)
/* bit 26 Scratchpad restore - for save/restore HW state - not used yet */
/* bits 27:31 number of Scratchpad buffers SW must allocate for the HW */
#define HCS_MAX_SCRATCHPAD(p)	(((p) >> 27) & 0x1f)

/* HCSPARAMS3 - hcs_params3 - bitmasks */
/* bits 0:7, Max U1 to U0 latency for the roothub ports */
#define HCS_U1_LATENCY(p)	(((p) >> 0) & 0xff)
/* bits 16:31, Max U2 to U0 latency for the roothub ports */
#define HCS_U2_LATENCY(p)	(((p) >> 16) & 0xffff)

/* HCCPARAMS - hcc_params - bitmasks */
/* true: HC can use 64-bit address pointers */
#define HCC_64BIT_ADDR(p)	((p) & BIT(0))
/* true: HC can do bandwidth negotiation */
#define HCC_BANDWIDTH_NEG(p)	((p) & BIT(1))
/* true: HC uses 64-byte Device Context structures
 * FIXME 64-byte context structures aren't supported yet.
 */
#define HCC_64BYTE_CONTEXT(p)	((p) & BIT(2))
#define HCC_CTX_SIZE(p)		(HCC_64BYTE_CONTEXT(p) ? 64 : 32)
/* true: HC has port power switches */
#define HCC_PPC(p)		((p) & BIT(3))
/* true: HC has port indicators */
#define HCC_INDICATOR(p)	((p) & BIT(4))
/* true: HC has Light HC Reset Capability */
#define HCC_LIGHT_RESET(p)	((p) & BIT(5))
/* true: HC supports latency tolerance messaging */
#define HCC_LTC(p)		((p) & BIT(6))
/* true: no secondary Stream ID Support */
#define HCC_NSS(p)		((p) & BIT(7))
/* Max size for Primary Stream Arrays - 2^(n+1), where n is bits 12:15 */
#define HCC_MAX_PSA(p)		(1 << ((((p) >> 12) & 0xf) + 1))
/* Extended Capabilities pointer from PCI base - section 5.3.6 */
#define HCC_EXT_CAPS(p)		(((p) >> 16) & 0xffff)

/* db_off bitmask - bits 0:1 reserved */
#define DBOFF_MASK		(~0x3)

/* run_regs_off bitmask - bits 0:4 reserved */
#define RTSOFF_MASK		(~0x1f)

/* Number of registers per port */
#define NUM_PORT_REGS	4

#define PORTSC		0
#define PORTPMSC	1
#define PORTLI		2
#define PORTHLPMC	3

/**
 * struct xhci_op_regs - xHCI Host Controller Operational Registers.
 * @command:            USBCMD - xHC command register
 * @status:             USBSTS - xHC status register
 * @page_size:          This indicates the page size that the host controller
 *                      supports.  If bit n is set, the HC supports a page size
 *                      of 2^(n+12), up to a 128MB page size.
 *                      4K is the minimum page size.
 * @cmd_ring:           CRP - 64-bit Command Ring Pointer
 * @dcbaa_ptr:          DCBAAP - 64-bit Device Context Base Address Array Pointer
 * @config_reg:         CONFIG - Configure Register
 * @port_status_base:   PORTSCn - base address for Port Status and Control
 *                      Each port has a Port Status and Control register,
 *                      followed by a Port Power Management Status and Control
 *                      register, a Port Link Info register, and a reserved
 *                      register.
 * @port_power_base:    PORTPMSCn - base address for
 *                      Port Power Management Status and Control
 * @port_link_base:     PORTLIn - base address for Port Link Info (current
 *                      Link PM state and control) for USB 2.1 and USB 3.0
 *                      devices.
 */
struct xhci_op_regs {
	__le32	command;
	__le32	status;
	__le32	page_size;
	__le32	reserved1;
	__le32	reserved2;
	__le32	dev_notification;
	__le64	cmd_ring;
	/* rsvd: offset 0x20-2F */
	__le32	reserved3[4];
	__le64	dcbaa_ptr;
	__le32	config_reg;
	/* rsvd: offset 0x3C-3FF */
	__le32	reserved4[241];
	/* port 1 registers, which serve as a base address for other ports */
	__le32	port_status_base;
	__le32	port_power_base;
	__le32	port_link_base;
	__le32	reserved5;
	/* registers for ports 2-255 */
	__le32	reserved6[NUM_PORT_REGS*254];
};

/* USBCMD - USB command - command bitmasks */
/* start/stop HC execution - do not write unless HC is halted*/
#define CMD_RUN		BIT(0)
/* Reset HC - resets internal HC state machine and all registers (except
 * PCI config regs).  HC does NOT drive a USB reset on the downstream ports.
 * The xHCI driver must reinitialize the xHC after setting this bit.
 */
#define CMD_RESET	BIT(1)
/* Event Interrupt Enable - a '1' allows interrupts from the host controller */
#define CMD_EIE		BIT(2)
/* Host System Error Interrupt Enable - get out-of-band signal for HC errors */
#define CMD_HSEIE	BIT(3)
/* bits 4:6 are reserved (and should be preserved on writes). */
/* light reset (port status stays unchanged) - reset completed when this is 0 */
#define CMD_LRESET	BIT(7)
/* host controller save/restore state. */
#define CMD_CSS		BIT(8)
#define CMD_CRS		BIT(9)
/* Enable Wrap Event - '1' means xHC generates an event when MFINDEX wraps. */
#define CMD_EWE		BIT(10)
/* MFINDEX power management - '1' means xHC can stop MFINDEX counter if all root
 * hubs are in U3 (selective suspend), disconnect, disabled, or powered-off.
 * '0' means the xHC can power it off if all ports are in the disconnect,
 * disabled, or powered-off state.
 */
#define CMD_PM_INDEX	BIT(11)
/* bits 12:31 are reserved (and should be preserved on writes). */
#define XHCI_IRQS	(CMD_EIE | CMD_HSEIE | CMD_EWE)

/* IMAN - Interrupt Management Register */
#define IMAN_IE		BIT(1)
#define IMAN_IP		BIT(0)

/* USBSTS - USB status - status bitmasks */
/* HC not running - set to 1 when run/stop bit is cleared. */
#define STS_HALT	BIT(0)
/* serious error, e.g. PCI parity error.  The HC will clear the run/stop bit. */
#define STS_FATAL	BIT(2)
/* event interrupt - clear this prior to clearing any IP flags in IR set*/
#define STS_EINT	BIT(3)
/* port change detect */
#define STS_PORT	BIT(4)
/* bits 5:7 reserved and zeroed */
/* save state status - '1' means xHC is saving state */
#define STS_SAVE	BIT(8)
/* restore state status - '1' means xHC is restoring state */
#define STS_RESTORE	BIT(9)
/* true: save or restore error */
#define STS_SRE		BIT(10)
/* true: Controller Not Ready to accept doorbell or op reg writes after reset */
#define STS_CNR		BIT(11)
/* true: internal Host Controller Error - SW needs to reset and reinitialize */
#define STS_HCE		BIT(12)
/* bits 13:31 reserved and should be preserved */

/*
 * DNCTRL - Device Notification Control Register - dev_notification bitmasks
 * Generate a device notification event when the HC sees a transaction with a
 * notification type that matches a bit set in this bit field.
 */
#define DEV_NOTE_MASK		(0xffff)
#define ENABLE_DEV_NOTE(x)	BIT(x)
/* Most of the device notification types should only be used for debug.
 * SW does need to pay attention to function wake notifications.
 */
#define DEV_NOTE_FWAKE		ENABLE_DEV_NOTE(1)

/* CRCR - Command Ring Control Register - cmd_ring bitmasks */
/* bit 0 is the command ring cycle state */
/* stop ring operation after completion of the currently executing command */
#define CMD_RING_PAUSE		BIT(1)
/* stop ring immediately - abort the currently executing command */
#define CMD_RING_ABORT		BIT(2)
/* true: command ring is running */
#define CMD_RING_RUNNING	BIT(3)
/* bits 4:5 reserved and should be preserved */
/* Command Ring pointer - bit mask for the lower 32 bits. */
#define CMD_RING_RSVD_BITS	(0x3f)

/* CONFIG - Configure Register - config_reg bitmasks */
/* bits 0:7 - maximum number of device slots enabled (NumSlotsEn) */
#define MAX_DEVS(p)		((p) & 0xff)
/* bits 8:31 - reserved and should be preserved */

/* PORTSC - Port Status and Control Register - port_status_base bitmasks */
/* true: device connected */
#define PORT_CONNECT		BIT(0)
/* true: port enabled */
#define PORT_PE			BIT(1)
/* bit 2 reserved and zeroed */
/* true: port has an over-current condition */
#define PORT_OC			BIT(3)
/* true: port reset signaling asserted */
#define PORT_RESET		BIT(4)
/* Port Link State - bits 5:8
 * A read gives the current link PM state of the port,
 * a write with Link State Write Strobe set sets the link state.
 */
#define PORT_PLS_MASK		(0xf << 5)
#define XDEV_U0			(0x0 << 5)
#define XDEV_U2			(0x2 << 5)
#define XDEV_U3			(0x3 << 5)
#define XDEV_RESUME		(0xf << 5)
/* true: port has power (see HCC_PPC) */
#define PORT_POWER		BIT(9)
/* bits 10:13 indicate device speed:
 * 0 - undefined speed - port hasn't be initialized by a reset yet
 * 1 - full speed
 * 2 - low speed
 * 3 - high speed
 * 4 - super speed
 * 5-15 reserved
 */
#define DEV_SPEED_MASK		(0xf << 10)
#define XDEV_FS			(0x1 << 10)
#define XDEV_LS			(0x2 << 10)
#define XDEV_HS			(0x3 << 10)
#define XDEV_SS			(0x4 << 10)
#define DEV_UNDEFSPEED(p)	(((p) & DEV_SPEED_MASK) == (0x0<<10))
#define DEV_FULLSPEED(p)	(((p) & DEV_SPEED_MASK) == XDEV_FS)
#define DEV_LOWSPEED(p)		(((p) & DEV_SPEED_MASK) == XDEV_LS)
#define DEV_HIGHSPEED(p)	(((p) & DEV_SPEED_MASK) == XDEV_HS)
#define DEV_SUPERSPEED(p)	(((p) & DEV_SPEED_MASK) == XDEV_SS)
/* Bits 20:23 in the Slot Context are the speed for the device */
#define SLOT_SPEED_FS		(XDEV_FS << 10)
#define SLOT_SPEED_LS		(XDEV_LS << 10)
#define SLOT_SPEED_HS		(XDEV_HS << 10)
#define SLOT_SPEED_SS		(XDEV_SS << 10)
/* Port Indicator Control */
#define PORT_LED_OFF		(0 << 14)
#define PORT_LED_AMBER		(1 << 14)
#define PORT_LED_GREEN		(2 << 14)
#define PORT_LED_MASK		(3 << 14)
/* Port Link State Write Strobe - set this when changing link state */
#define PORT_LINK_STROBE	BIT(16)
/* true: connect status change */
#define PORT_CSC		BIT(17)
/* true: port enable change */
#define PORT_PEC		BIT(18)
/* true: warm reset for a USB 3.0 device is done.  A "hot" reset puts the port
 * into an enabled state, and the device into the default state.  A "warm" reset
 * also resets the link, forcing the device through the link training sequence.
 * SW can also look at the Port Reset register to see when warm reset is done.
 */
#define PORT_WRC		BIT(19)
/* true: over-current change */
#define PORT_OCC		BIT(20)
/* true: reset change - 1 to 0 transition of PORT_RESET */
#define PORT_RC			BIT(21)
/* port link status change - set on some port link state transitions:
 *  Transition                          Reason
 *  ------------------------------------------------------------------------------
 *  - U3 to Resume                      Wakeup signaling from a device
 *  - Resume to Recovery to U0          USB 3.0 device resume
 *  - Resume to U0                      USB 2.0 device resume
 *  - U3 to Recovery to U0              Software resume of USB 3.0 device complete
 *  - U3 to U0                          Software resume of USB 2.0 device complete
 *  - U2 to U0                          L1 resume of USB 2.1 device complete
 *  - U0 to U0 (???)                    L1 entry rejection by USB 2.1 device
 *  - U0 to disabled                    L1 entry error with USB 2.1 device
 *  - Any state to inactive             Error on USB 3.0 port
 */
#define PORT_PLC		BIT(22)
/* port configure error change - port failed to configure its link partner */
#define PORT_CEC		BIT(23)
/* Cold Attach Status - xHC can set this bit to report device attached during
 * Sx state. Warm port reset should be perfomed to clear this bit and move port
 * to connected state.
 */
#define PORT_CAS		BIT(24)
/* wake on connect (enable) */
#define PORT_WKCONN_E		BIT(25)
/* wake on disconnect (enable) */
#define PORT_WKDISC_E		BIT(26)
/* wake on over-current (enable) */
#define PORT_WKOC_E		BIT(27)
/* bits 28:29 reserved */
/* true: device is removable - for USB 3.0 roothub emulation */
#define PORT_DEV_REMOVE		BIT(30)
/* Initiate a warm port reset - complete when PORT_WRC is '1' */
#define PORT_WR			BIT(31)

/* We mark duplicate entries with -1 */
#define DUPLICATE_ENTRY		((u8)(-1))

/* Port Power Management Status and Control - port_power_base bitmasks */
/* Inactivity timer value for transitions into U1, in microseconds.
 * Timeout can be up to 127us.  0xFF means an infinite timeout.
 */
#define PORT_U1_TIMEOUT(p)	((p) & 0xff)
#define PORT_U1_TIMEOUT_MASK	0xff
/* Inactivity timer value for transitions into U2 */
#define PORT_U2_TIMEOUT(p)	(((p) & 0xff) << 8)
#define PORT_U2_TIMEOUT_MASK	(0xff << 8)
/* Bits 24:31 for port testing */

/* USB2 Protocol PORTSPMSC */
#define PORT_L1S_MASK		0x7
#define PORT_L1S_SUCCESS	0x1
#define PORT_RWE		BIT(3)
#define PORT_HIRD(p)		(((p) & 0xf) << 4)
#define PORT_HIRD_MASK		(0xf << 4)
#define PORT_L1DS_MASK		(0xff << 8)
#define PORT_L1DS(p)		(((p) & 0xff) << 8)
#define PORT_HLE		BIT(16)

/* USB2 Protocol PORTHLPMC */
#define PORT_HIRDM(p)		((p) & 3)
#define PORT_L1_TIMEOUT(p)	(((p) & 0xff) << 2)
#define PORT_BESLD(p)		(((p) & 0xf) << 10)

/* use 512 microseconds as USB2 LPM L1 default timeout. */
#define XHCI_L1_TIMEOUT		512

/* Set default HIRD/BESL value to 4 (350/400us) for USB2 L1 LPM resume latency.
 * Safe to use with mixed HIRD and BESL systems (host and device) and is used
 * by other operating systems.
 *
 * XHCI 1.0 errata 8/14/12 Table 13 notes:
 * "Software should choose xHC BESL/BESLD field values that do not violate a
 * device's resume latency requirements,
 * e.g. not program values > '4' if BLC = '1' and a HIRD device is attached,
 * or not program values < '4' if BLC = '0' and a BESL device is attached.
 */
#define XHCI_DEFAULT_BESL	4

/**
 * struct xhci_intr_reg - Interrupt Register Set
 * @irq_pending:	IMAN - Interrupt Management Register.  Used to enable
 * 			interrupts and check for pending interrupts.
 * @irq_control:	IMOD - Interrupt Moderation Register.
 * 			Used to throttle interrupts.
 * @erst_size:		Number of segments in the Event Ring Segment Table (ERST).
 * @erst_base:		ERST base address.
 * @erst_dequeue:	Event ring dequeue pointer.
 *
 * Each interrupter (defined by a MSI-X vector) has an event ring and an Event
 * Ring Segment Table (ERST) associated with it.  The event ring is comprised of
 * multiple segments of the same size.  The HC places events on the ring and
 * "updates the Cycle bit in the TRBs to indicate to software the current
 * position of the Enqueue Pointer." The HCD (Linux) processes those events and
 * updates the dequeue pointer.
 */
struct xhci_intr_reg {
	__le32	irq_pending;
	__le32	irq_control;
	__le32	erst_size;
	__le32	rsvd;
	__le64	erst_base;
	__le64	erst_dequeue;
};

/* irq_pending bitmasks */
#define ER_IRQ_PENDING(p)	((p) & 0x1)
/* bits 2:31 need to be preserved */
/* THIS IS BUGGY - FIXME - IP IS WRITE 1 TO CLEAR */
#define ER_IRQ_CLEAR(p)		((p) & 0xfffffffe)
#define ER_IRQ_ENABLE(p)	((ER_IRQ_CLEAR(p)) | 0x2)
#define ER_IRQ_DISABLE(p)	((ER_IRQ_CLEAR(p)) & ~(0x2))

/* irq_control bitmasks */
/* Minimum interval between interrupts (in 250ns intervals).  The interval
 * between interrupts will be longer if there are no events on the event ring.
 * Default is 4000 (1 ms).
 */
#define ER_IRQ_INTERVAL_MASK	0xffff
/* Counter used to count down the time to the next interrupt - HW use only */
#define ER_IRQ_COUNTER_MASK	(0xffff << 16)

/* erst_size bitmasks */
/* Preserve bits 16:31 of erst_size */
#define ERST_SIZE_MASK		(0xffff << 16)

/* erst_dequeue bitmasks */
/* Dequeue ERST Segment Index (DESI) - Segment number (or alias)
 * where the current dequeue pointer lies.  This is an optional HW hint.
 */
#define ERST_DESI_MASK		0x7
/* Event Handler Busy (EHB) - is the event ring scheduled to be serviced by
 * a work queue (or delayed service routine)?
 */
#define ERST_EHB		BIT(3)
#define ERST_PTR_MASK		0xf

/**
 * struct xhci_run_regs
 * @microframe_index: MFINDEX - current microframe number
 *
 * Section 5.5 Host Controller Runtime Registers:
 * "Software should read and write these registers using only Dword (32 bit)
 * or larger accesses"
 */
struct xhci_run_regs {
	__le32			microframe_index;
	__le32			rsvd[7];
	struct xhci_intr_reg	ir_set[128];
};

/**
 * struct doorbell_array
 *
 * Bits  0 -  7: Endpoint target
 * Bits  8 - 15: RsvdZ
 * Bits 16 - 31: Stream ID
 *
 * Section 5.6
 */
struct xhci_doorbell_array {
	__le32	doorbell[256];
};

#define DB_VALUE(ep, stream)	((((ep) + 1) & 0xff) | ((stream) << 16))
#define DB_VALUE_HOST		0x00000000

/**
 * struct xhci_slot_ctx
 * @dev_info:	Route string, device speed, hub info, and last valid endpoint
 * @dev_info2:	Max exit latency for device number, root hub port number
 * @tt_info:	tt_info is used to construct split transaction tokens
 * @dev_state:	slot state and device address
 *
 * Slot Context - section 6.2.1.1.  This assumes the HC uses 32-byte context
 * structures.  If the HC uses 64-byte contexts, there is an additional 32 bytes
 * reserved at the end of the slot context for HC internal use.
 */
struct xhci_slot_ctx {
	__le32	dev_info;
	__le32	dev_info2;
	__le32	tt_info;
	__le32	dev_state;
	/* offset 0x10 to 0x1f reserved for HC internal use */
	__le32	reserved[4];
};

/* dev_info bitmasks */
/* Route String - 0:19 */
#define ROUTE_STRING_MASK	0xfffff
/* Device speed - values defined by PORTSC Device Speed field - 20:23 */
#define DEV_SPEED		(0xf << 20)
/* bit 24 reserved */
/* Is this LS/FS device connected through a HS hub? - bit 25 */
#define DEV_MTT			BIT(25)
/* Set if the device is a hub - bit 26 */
#define DEV_HUB			BIT(26)
/* Index of the last valid endpoint context in this device context - 27:31 */
#define LAST_CTX_MASK		(0x1f << 27)
#define LAST_CTX(p)		((p) << 27)
#define LAST_CTX_TO_EP_NUM(p)	(((p) >> 27) - 1)
#define SLOT_FLAG		BIT(0)
#define EP0_FLAG		BIT(1)

/* dev_info2 bitmasks */
/* Max Exit Latency (ms) - worst case time to wake up all links in dev path */
#define MAX_EXIT		0xffff
/* Root hub port number that is needed to access the USB device */
#define ROOT_HUB_PORT(p)	(((p) & 0xff) << 16)
#define DEVINFO_TO_ROOT_HUB_PORT(p)	(((p) >> 16) & 0xff)
/* Maximum number of ports under a hub device */
#define XHCI_MAX_PORTS(p)	(((p) & 0xff) << 24)

/* tt_info bitmasks */
/*
 * TT Hub Slot ID - for low or full speed devices attached to a high-speed hub
 * The Slot ID of the hub that isolates the high speed signaling from
 * this low or full-speed device.  '0' if attached to root hub port.
 */
#define TT_SLOT			0xff
/*
 * The number of the downstream facing port of the high-speed hub
 * '0' if the device is not low or full speed.
 */
#define TT_PORT			(0xff << 8)
#define TT_THINK_TIME(p)	(((p) & 0x3) << 16)

/* dev_state bitmasks */
/* USB device address - assigned by the HC */
#define DEV_ADDR_MASK		0xff
/* bits 8:26 reserved */
/* Slot state */
#define SLOT_STATE		(0x1f << 27)
#define GET_SLOT_STATE(p)	(((p) & (0x1f << 27)) >> 27)

#define SLOT_STATE_DISABLED	0x0
#define SLOT_STATE_ENABLED	SLOT_STATE_DISABLED
#define SLOT_STATE_DEFAULT	0x1
#define SLOT_STATE_ADDRESSED	0x2
#define SLOT_STATE_CONFIGURED   0x3

/**
 * struct xhci_ep_ctx
 * @ep_info:	endpoint state, streams, mult, and interval information.
 * @ep_info2:	information on endpoint type, max packet size, max burst size,
 * 		error count, and whether the HC will force an event for all
 * 		transactions.
 * @deq:	64-bit ring dequeue pointer address.  If the endpoint only
 * 		defines one stream, this points to the endpoint transfer ring.
 * 		Otherwise, it points to a stream context array, which has a
 * 		ring pointer for each flow.
 * @tx_info:	Average TRB lengths for the endpoint ring and
 * 		max payload within an Endpoint Service Interval Time (ESIT).
 *
 * Endpoint Context - section 6.2.1.2.  This assumes the HC uses 32-byte context
 * structures.  If the HC uses 64-byte contexts, there is an additional 32 bytes
 * reserved at the end of the endpoint context for HC internal use.
 */
struct xhci_ep_ctx {
	__le32	ep_info;
	__le32	ep_info2;
	__le64	deq;
	__le32	tx_info;
	/* offset 0x14 - 0x1f reserved for HC internal use */
	__le32	reserved[3];
};

/* ep_info bitmasks */
/*
 * Endpoint State - bits 0:2
 * 0 - disabled
 * 1 - running
 * 2 - halted due to halt condition - ok to manipulate endpoint ring
 * 3 - stopped
 * 4 - TRB error
 * 5-7 - reserved
 */
#define EP_STATE_MASK		0xf
#define EP_STATE_DISABLED	0x0
#define EP_STATE_RUNNING	0x1
#define EP_STATE_HALTED		0x2
#define EP_STATE_STOPPED	0x3
#define EP_STATE_ERROR		0x4
/* Mult - Max number of burtst within an interval, in EP companion desc. */
#define EP_MULT(p)		(((p) & 0x3) << 8)
#define CTX_TO_EP_MULT(p)	(((p) >> 8) & 0x3)
/* bits 10:14 are Max Primary Streams */
/* bit 15 is Linear Stream Array */
/* Interval - period between requests to an endpoint - 125u increments. */
#define EP_INTERVAL(p)		(((p) & 0xff) << 16)
#define EP_INTERVAL_TO_UFRAMES(p)	(1 << (((p) >> 16) & 0xff))
#define CTX_TO_EP_INTERVAL(p)	(((p) >> 16) & 0xff)
#define EP_MAXPSTREAMS_MASK	(0x1f << 10)
#define EP_MAXPSTREAMS(p)	(((p) << 10) & EP_MAXPSTREAMS_MASK)
/* Endpoint is set up with a Linear Stream Array (vs. Secondary Stream Array) */
#define EP_HAS_LSA		BIT(15)

/* ep_info2 bitmasks */
/*
 * Force Event - generate transfer events for all TRBs for this endpoint
 * This will tell the HC to ignore the IOC and ISP flags (for debugging only).
 */
#define FORCE_EVENT		BIT(0)
#define ERROR_COUNT(p)		(((p) & 0x3) << 1)
#define CTX_TO_EP_TYPE(p)	(((p) >> 3) & 0x7)
#define EP_TYPE(p)		((p) << 3)
#define ISOC_OUT_EP		0x1
#define BULK_OUT_EP		0x2
#define INT_OUT_EP		0x3
#define CTRL_EP			0x4
#define ISOC_IN_EP		0x5
#define BULK_IN_EP		0x6
#define INT_IN_EP		0x7
/* bit 6 reserved */
/* bit 7 is Host Initiate Disable - for disabling stream selection */
#define MAX_BURST(p)		(((p) & 0xff) << 8)
#define CTX_TO_MAX_BURST(p)	(((p) >> 8) & 0xff)
#define MAX_PACKET(p)		(((p) & 0xffff) << 16)
#define MAX_PACKET_MASK		(0xffff << 16)
#define MAX_PACKET_DECODED(p)	(((p) >> 16) & 0xffff)

/* Get max packet size from ep desc. Bit 10..0 specify the max packet size.
 * USB2.0 spec 9.6.6.
 */
#define GET_MAX_PACKET(p)	((p) & 0x7ff)

/* tx_info bitmasks */
#define AVG_TRB_LENGTH_FOR_EP(p)	((p) & 0xffff)
#define MAX_ESIT_PAYLOAD_FOR_EP(p)	(((p) & 0xffff) << 16)
#define CTX_TO_MAX_ESIT_PAYLOAD(p)	(((p) >> 16) & 0xffff)

/* deq bitmasks */
#define EP_CTX_CYCLE_MASK	BIT(0)
#define SCTX_DEQ_MASK		(~0xfL)

/**
 * struct xhci_input_control_context
 * Input control context; see section 6.2.5.
 *
 * @drop_context:	set the bit of the endpoint context you want to disable
 * @add_context:	set the bit of the endpoint context you want to enable
 */
struct xhci_input_control_ctx {
	__le32	drop_flags;
	__le32	add_flags;
	__le32	rsvd2[6];
};

#define EP_IS_ADDED(ctrl_ctx, i)	\
	(le32_to_cpu(ctrl_ctx->add_flags) & (1 << (i + 1)))
#define EP_IS_DROPPED(ctrl_ctx, i)	\
	(le32_to_cpu(ctrl_ctx->drop_flags) & (1 << (i + 1)))

/* drop context bitmasks */
#define DROP_EP(x)	BIT(x)
/* add context bitmasks */
#define ADD_EP(x)	BIT(x)

struct xhci_stream_ctx {
	/* 64-bit stream ring address, cycle state, and stream type */
	__le64	stream_ring;
	/* offset 0x14 - 0x1f reserved for HC internal use */
	__le32	reserved[2];
};

/* Stream Context Types (section 6.4.1) - bits 3:1 of stream ctx deq ptr */
#define SCT_FOR_CTX(p)		(((p) & 0x7) << 1)
/* Secondary stream array type, dequeue pointer is to a transfer ring */
#define SCT_SEC_TR		0x0
/* Primary stream array type, dequeue pointer is to a transfer ring */
#define SCT_PRI_TR		0x1
/* Dequeue pointer is for a secondary stream array (SSA) with 8 entries */
#define SCT_SSA_8		0x2
#define SCT_SSA_16		0x3
#define SCT_SSA_32		0x4
#define SCT_SSA_64		0x5
#define SCT_SSA_128		0x6
#define SCT_SSA_256		0x7

#define SMALL_STREAM_ARRAY_SIZE		256
#define MEDIUM_STREAM_ARRAY_SIZE	1024

/* "Block" sizes in bytes the hardware uses for different device speeds.
 * The logic in this part of the hardware limits the number of bits the hardware
 * can use, so must represent bandwidth in a less precise manner to mimic what
 * the scheduler hardware computes.
 */
#define FS_BLOCK		1
#define HS_BLOCK		4
#define SS_BLOCK		16
#define DMI_BLOCK		32

/* Each device speed has a protocol overhead (CRC, bit stuffing, etc) associated
 * with each byte transferred.  SuperSpeed devices have an initial overhead to
 * set up bursts.  These are in blocks, see above.  LS overhead has already been
 * translated into FS blocks.
 */
#define DMI_OVERHEAD		8
#define DMI_OVERHEAD_BURST	4
#define SS_OVERHEAD		8
#define SS_OVERHEAD_BURST	32
#define HS_OVERHEAD		26
#define FS_OVERHEAD		20
#define LS_OVERHEAD		128

/* The TTs need to claim roughly twice as much bandwidth (94 bytes per
 * microframe ~= 24Mbps) of the HS bus as the devices can actually use because
 * of overhead associated with split transfers crossing microframe boundaries.
 * 31 blocks is pure protocol overhead.
 */
#define TT_HS_OVERHEAD		(31 + 94)
#define TT_DMI_OVERHEAD		(25 + 12)

/* Bandwidth limits in blocks */
#define FS_BW_LIMIT		1285
#define TT_BW_LIMIT		1320
#define HS_BW_LIMIT		1607
#define SS_BW_LIMIT_IN		3906
#define DMI_BW_LIMIT_IN		3906
#define SS_BW_LIMIT_OUT		3906
#define DMI_BW_LIMIT_OUT	3906

/* Percentage of bus bandwidth reserved for non-periodic transfers */
#define FS_BW_RESERVED		10
#define HS_BW_RESERVED		20
#define SS_BW_RESERVED		10

enum xhci_overhead_type {
	LS_OVERHEAD_TYPE = 0,
	FS_OVERHEAD_TYPE,
	HS_OVERHEAD_TYPE,
};

struct xhci_transfer_event {
	/* 64-bit buffer address, or immediate data */
	__le64	buffer;
	__le32	transfer_len;
	/* This field is interpreted differently based on the type of TRB */
	__le32	flags;
};

/* Transfer event TRB length bit mask */
/* bits 0:23 */
#define EVENT_TRB_LEN(p)	((p) & 0xffffff)

/** Transfer Event bit fields **/
#define TRB_TO_EP_ID(p)		(((p) >> 16) & 0x1f)

/* Completion Code - only applicable for some types of TRBs */
#define COMP_CODE_MASK		(0xff << 24)
#define GET_COMP_CODE(p)	(((p) & COMP_CODE_MASK) >> 24)
#define COMP_SUCCESS		1
/* Data Buffer Error */
#define COMP_DB_ERR		2
/* Babble Detected Error */
#define COMP_BABBLE		3
/* USB Transaction Error */
#define COMP_TX_ERR		4
/* TRB Error - some TRB field is invalid */
#define COMP_TRB_ERR		5
/* Stall Error - USB device is stalled */
#define COMP_STALL		6
/* Resource Error - HC doesn't have memory for that device configuration */
#define COMP_ENOMEM		7
/* Bandwidth Error - not enough room in schedule for this dev config */
#define COMP_BW_ERR		8
/* No Slots Available Error - HC ran out of device slots */
#define COMP_ENOSLOTS		9
/* Invalid Stream Type Error */
#define COMP_STREAM_ERR		10
/* Slot Not Enabled Error - doorbell rung for disabled device slot */
#define COMP_EBADSLT		11
/* Endpoint Not Enabled Error */
#define COMP_EBADEP		12
/* Short Packet */
#define COMP_SHORT_TX		13
/* Ring Underrun - doorbell rung for an empty isoc OUT ep ring */
#define COMP_UNDERRUN		14
/* Ring Overrun - isoc IN ep ring is empty when ep is scheduled to RX */
#define COMP_OVERRUN		15
/* Virtual Function Event Ring Full Error */
#define COMP_VF_FULL		16
/* Parameter Error - Context parameter is invalid */
#define COMP_EINVAL		17
/* Bandwidth Overrun Error - isoc ep exceeded its allocated bandwidth */
#define COMP_BW_OVER		18
/* Context State Error - illegal context state transition requested */
#define COMP_CTX_STATE		19
/* No Ping Response Error - HC didn't get PING_RESPONSE in time to TX */
#define COMP_PING_ERR		20
/* Event Ring is full */
#define COMP_ER_FULL		21
/* Incompatible Device Error */
#define COMP_DEV_ERR		22
/* Missed Service Error - HC couldn't service an isoc ep within interval */
#define COMP_MISSED_INT		23
/* Successfully stopped command ring */
#define COMP_CMD_STOP		24
/* Successfully aborted current command and stopped command ring */
#define COMP_CMD_ABORT		25
/* Stopped - transfer was terminated by a stop endpoint command */
#define COMP_STOP		26
/* Same as COMP_EP_STOPPED, but the transferred length in the event is invalid */
#define COMP_STOP_INVAL		27
/* Control Abort Error - Debug Capability - control pipe aborted */
#define COMP_DBG_ABORT		28
/* Max Exit Latency Too Large Error */
#define COMP_MEL_ERR		29
/* TRB type 30 reserved */
/* Isoc Buffer Overrun - an isoc IN ep sent more data than could fit in TD */
#define COMP_BUFF_OVER		31
/* Event Lost Error - xHC has an "internal event overrun condition" */
#define COMP_ISSUES		32
/* Undefined Error - reported when other error codes don't apply */
#define COMP_UNKNOWN		33
/* Invalid Stream ID Error */
#define COMP_STRID_ERR		34
/* Secondary Bandwidth Error - may be returned by a Configure Endpoint cmd */
#define COMP_2ND_BW_ERR		35
/* Split Transaction Error */
#define COMP_SPLIT_ERR		36

struct xhci_link_trb {
	/* 64-bit segment pointer*/
	__le64	segment_ptr;
	__le32	intr_target;
	__le32	control;
};

/* control bitfields */
#define LINK_TOGGLE		BIT(1)

/* Command completion event TRB */
struct xhci_event_cmd {
	/* Pointer to command TRB, or the value passed by the event data trb */
	__le64	cmd_trb;
	__le32	status;
	__le32	flags;
};

/* flags bitmasks */

/* Address device - disable SetAddress */
#define TRB_BSR			BIT(9)
enum xhci_setup_dev {
	SETUP_CONTEXT_ONLY,
	SETUP_CONTEXT_ADDRESS,
};

/* bits 16:23 are the virtual function ID */
/* bits 24:31 are the slot ID */
#define TRB_TO_SLOT_ID(p)	(((p) & (0xff<<24)) >> 24)
#define SLOT_ID_FOR_TRB(p)	(((p) & 0xff) << 24)

/* Configure Endpoint Command TRB - deconfigure */
#define TRB_DC			BIT(9)

/* Stop Endpoint TRB - ep_index to endpoint ID for this TRB */
#define TRB_TO_EP_INDEX(p)	((((p) & (0x1f << 16)) >> 16) - 1)
#define EP_ID_FOR_TRB(p)	((((p) + 1) & 0x1f) << 16)

#define SUSPEND_PORT_FOR_TRB(p)	(((p) & 1) << 23)
#define TRB_TO_SUSPEND_PORT(p)	(((p) & (1 << 23)) >> 23)
#define LAST_EP_INDEX		30

/* Set TR Dequeue Pointer command TRB fields, 6.4.3.9 */
#define TRB_TO_STREAM_ID(p)	((((p) & (0xffff << 16)) >> 16))
#define STREAM_ID_FOR_TRB(p)	((((p)) & 0xffff) << 16)
#define SCT_FOR_TRB(p)		(((p) << 1) & 0x7)

/* Port Status Change Event TRB fields */
/* Port ID - bits 31:24 */
#define GET_PORT_ID(p)		(((p) & (0xff << 24)) >> 24)

/* Normal TRB fields */
/* transfer_len bitmasks - bits 0:16 */
#define TRB_LEN(p)		((p) & 0x1ffff)
/* Interrupter Target - which MSI-X vector to target the completion event at */
#define TRB_INTR_TARGET(p)	(((p) & 0x3ff) << 22)
#define GET_INTR_TARGET(p)	(((p) >> 22) & 0x3ff)
#define TRB_TBC(p)		(((p) & 0x3) << 7)
#define TRB_TLBPC(p)		(((p) & 0xf) << 16)

/* Cycle bit - indicates TRB ownership by HC or HCD */
#define TRB_CYCLE		BIT(0)
/*
 * Force next event data TRB to be evaluated before task switch.
 * Used to pass OS data back after a TD completes.
 */
#define TRB_ENT			BIT(1)
/* Interrupt on short packet */
#define TRB_ISP			BIT(2)
/* Set PCIe no snoop attribute */
#define TRB_NO_SNOOP		BIT(3)
/* Chain multiple TRBs into a TD */
#define TRB_CHAIN		BIT(4)
/* Interrupt on completion */
#define TRB_IOC			BIT(5)
/* The buffer pointer contains immediate data */
#define TRB_IDT			BIT(6)

/* Block Event Interrupt */
#define TRB_BEI			BIT(9)

/* Control transfer TRB specific fields */
#define TRB_DIR_IN		BIT(16)
#define TRB_TX_TYPE(p)		((p) << 16)
#define TRB_DATA_OUT		2
#define TRB_DATA_IN		3

/* Isochronous TRB specific fields */
#define TRB_SIA			BIT(31)

struct xhci_generic_trb {
	__le32	field[4];
};

union xhci_trb {
	struct xhci_link_trb		link;
	struct xhci_transfer_event	trans_event;
	struct xhci_event_cmd		event_cmd;
	struct xhci_generic_trb		generic;
};

/* TRB bit mask */
#define TRB_TYPE_BITMASK	(0xfc00)
#define TRB_TYPE(p)		((p) << 10)
#define TRB_FIELD_TO_TYPE(p)	(((p) & TRB_TYPE_BITMASK) >> 10)
/* TRB type IDs */
/* bulk, interrupt, isoc scatter/gather, and control data stage */
#define TRB_NORMAL		1
/* setup stage for control transfers */
#define TRB_SETUP		2
/* data stage for control transfers */
#define TRB_DATA		3
/* status stage for control transfers */
#define TRB_STATUS		4
/* isoc transfers */
#define TRB_ISOC		5
/* TRB for linking ring segments */
#define TRB_LINK		6
#define TRB_EVENT_DATA		7
/* Transfer Ring No-op (not for the command ring) */
#define TRB_TR_NOOP		8
/* Command TRBs */
/* Enable Slot Command */
#define TRB_ENABLE_SLOT		9
/* Disable Slot Command */
#define TRB_DISABLE_SLOT	10
/* Address Device Command */
#define TRB_ADDR_DEV		11
/* Configure Endpoint Command */
#define TRB_CONFIG_EP		12
/* Evaluate Context Command */
#define TRB_EVAL_CONTEXT	13
/* Reset Endpoint Command */
#define TRB_RESET_EP		14
/* Stop Transfer Ring Command */
#define TRB_STOP_RING		15
/* Set Transfer Ring Dequeue Pointer Command */
#define TRB_SET_DEQ		16
/* Reset Device Command */
#define TRB_RESET_DEV		17
/* Force Event Command (opt) */
#define TRB_FORCE_EVENT		18
/* Negotiate Bandwidth Command (opt) */
#define TRB_NEG_BANDWIDTH	19
/* Set Latency Tolerance Value Command (opt) */
#define TRB_SET_LT		20
/* Get port bandwidth Command */
#define TRB_GET_BW		21
/* Force Header Command - generate a transaction or link management packet */
#define TRB_FORCE_HEADER	22
/* No-op Command - not for transfer rings */
#define TRB_CMD_NOOP		23
/* TRB IDs 24-31 reserved */
/* Event TRBS */
/* Transfer Event */
#define TRB_TRANSFER		32
/* Command Completion Event */
#define TRB_COMPLETION		33
/* Port Status Change Event */
#define TRB_PORT_STATUS		34
/* Bandwidth Request Event (opt) */
#define TRB_BANDWIDTH_EVENT	35
/* Doorbell Event (opt) */
#define TRB_DOORBELL		36
/* Host Controller Event */
#define TRB_HC_EVENT		37
/* Device Notification Event - device sent function wake notification */
#define TRB_DEV_NOTE		38
/* MFINDEX Wrap Event - microframe counter wrapped */
#define TRB_MFINDEX_WRAP	39
/* TRB IDs 40-47 reserved, 48-63 is vendor-defined */

/* Nec vendor-specific command completion event. */
#define TRB_NEC_CMD_COMP	48
/* Get NEC firmware revision. */
#define TRB_NEC_GET_FW		49

#define TRB_TYPE_LINK(x)	(((x) & TRB_TYPE_BITMASK) == TRB_TYPE(TRB_LINK))
/* Above, but for __le32 types -- can avoid work by swapping constants: */
#define TRB_TYPE_LINK_LE32(x)	\
	(((x) & cpu_to_le32(TRB_TYPE_BITMASK)) == cpu_to_le32(TRB_TYPE(TRB_LINK)))
#define TRB_TYPE_NOOP_LE32(x)	\
	(((x) & cpu_to_le32(TRB_TYPE_BITMASK)) == cpu_to_le32(TRB_TYPE(TRB_TR_NOOP)))

#define NEC_FW_MINOR(p)		(((p) >> 0) & 0xff)
#define NEC_FW_MAJOR(p)		(((p) >> 8) & 0xff)

/*
 * TRBS_PER_SEGMENT must be a multiple of 4,
 * since the command ring is 64-byte aligned.
 * It must also be greater than 16.
 */
#define TRBS_PER_SEGMENT	64
/* Allow two commands + a link TRB, along with any reserved command TRBs */
#define MAX_RSVD_CMD_TRBS	(TRBS_PER_SEGMENT - 3)
#define TRB_SEGMENT_SIZE	(TRBS_PER_SEGMENT * 16)
#define TRB_SEGMENT_SHIFT	(ilog2(TRB_SEGMENT_SIZE))
/* TRB buffer pointers can't cross 64KB boundaries */
#define TRB_MAX_BUFF_SHIFT	16
#define TRB_MAX_BUFF_SIZE	(1 << TRB_MAX_BUFF_SHIFT)

/* xHCI command default timeout value */
#define XHCI_CMD_DEFAULT_TIMEOUT	(5 * SECOND)

struct xhci_erst_entry {
	/* 64-bit event ring segment address */
	__le64	seg_addr;
	__le32	seg_size;
	/* Set to zero */
	__le32	rsvd;
};

/*
 * Each segment table entry is 4*32bits long.  1K seems like an ok size:
 * (1K bytes * 8bytes/bit) / (4*32 bits) = 64 segment entries in the table,
 * meaning 64 ring segments.
 * Initial allocated size of the ERST, in number of entries */
#define ERST_NUM_SEGS			1
/* Initial allocated size of the ERST, in number of entries */
#define ERST_SIZE			64
/* Initial number of event segment rings allocated */
#define ERST_ENTRIES			1
/* Poll every 60 seconds */
#define POLL_TIMEOUT			60
/* Stop endpoint command timeout (secs) for URB cancellation watchdog timer */
#define XHCI_STOP_EP_CMD_TIMEOUT	5
/* XXX: Make these module parameters */

/*
 * It can take up to 20 ms to transition from RExit to U0 on the
 * Intel Lynx Point LP xHCI host.
 */
#define XHCI_MAX_REXIT_TIMEOUT	(20 * MSECONDS)

#define XHCI_MAX_EXT_CAPS	50

#define XHCI_EXT_PORT_MAJOR(x)	(((x) >> 24) & 0xff)
#define XHCI_EXT_PORT_OFF(x)	((x) & 0xff)
#define XHCI_EXT_PORT_COUNT(x)	(((x) >> 8) & 0xff)

/* Extended capability register fields */
#define XHCI_EXT_CAPS_ID(p)	(((p)>>0)&0xff)
#define XHCI_EXT_CAPS_NEXT(p)	(((p)>>8)&0xff)
#define XHCI_EXT_CAPS_VAL(p)	((p)>>16)
/* Extended capability IDs - ID 0 reserved */
#define XHCI_EXT_CAPS_LEGACY	1
#define XHCI_EXT_CAPS_PROTOCOL	2
#define XHCI_EXT_CAPS_PM	3
#define XHCI_EXT_CAPS_VIRT	4
#define XHCI_EXT_CAPS_ROUTE	5
/* IDs 6-9 reserved */
#define XHCI_EXT_CAPS_DEBUG	10
/* USB Legacy Support Capability - section 7.1.1 */
#define XHCI_HC_BIOS_OWNED	BIT(16)
#define XHCI_HC_OS_OWNED	BIT(24)

/* USB Legacy Support Capability - section 7.1.1 */
/* Add this offset, plus the value of xECP in HCCPARAMS to the base address */
#define XHCI_LEGACY_SUPPORT_OFFSET	0x00

/* USB Legacy Support Control and Status Register  - section 7.1.2 */
/* Add this offset, plus the value of xECP in HCCPARAMS to the base address */
#define XHCI_LEGACY_CONTROL_OFFSET	0x04
/* bits 1:3, 5:12, and 17:19 need to be preserved; bits 21:28 should be zero */
#define XHCI_LEGACY_DISABLE_SMI		((0x7 << 1) + (0xff << 5) + (0x7 << 17))
#define XHCI_LEGACY_SMI_EVENTS		(0x7 << 29)

/* USB 2.0 xHCI 0.96 L1C capability - section 7.2.2.1.3.2 */
#define XHCI_L1C		BIT(16)

/* USB 2.0 xHCI 1.0 hardware LMP capability - section 7.2.2.1.3.2 */
#define XHCI_HLC		BIT(19)
#define XHCI_BLC		BIT(20)

/*
 * Registers should always be accessed with double word or quad word accesses.
 *
 * Some xHCI implementations may support 64-bit address pointers.  Registers
 * with 64-bit address pointers should be written to with dword accesses by
 * writing the low dword first (ptr[0]), then the high dword (ptr[1]) second.
 * xHCI implementations that do not support 64-bit address pointers will ignore
 * the high dword, and write order is irrelevant.
 */
static inline u64 xhci_read_64(__le64 __iomem *regs)
{
	__u32 __iomem *ptr = (__u32 __iomem *)regs;
	u64 val_lo = readl(ptr);
	u64 val_hi = readl(ptr + 1);
	return val_lo + (val_hi << 32);
}
static inline void xhci_write_64(const u64 val, __le64 __iomem *regs)
{
	__u32 __iomem *ptr = (__u32 __iomem *)regs;
	u32 val_lo = lower_32_bits(val);
	u32 val_hi = upper_32_bits(val);

	writel(val_lo, ptr);
	writel(val_hi, ptr + 1);
}

/*
 * Barebox xHCI housekeeping structs
 */

enum xhci_ring_type {
        TYPE_CTRL = 0,
        TYPE_ISOC,
        TYPE_BULK,
        TYPE_INTR,
        TYPE_STREAM,
        TYPE_COMMAND,
        TYPE_EVENT,
};

struct xhci_ring {
	struct list_head list;
	union xhci_trb *trbs;
	union xhci_trb *enqueue;
	union xhci_trb *dequeue;
	enum xhci_ring_type type;
	int num_trbs;
	int cycle_state;
};

struct xhci_device_context {
	struct xhci_slot_ctx slot;
	struct xhci_ep_ctx ep[31];
};

struct xhci_input_context {
	struct xhci_input_control_ctx icc;
	struct xhci_slot_ctx slot;
	struct xhci_ep_ctx ep[31];
};

struct xhci_virtual_device {
	struct list_head list;
	struct usb_device *udev;
	void *dma;
	size_t dma_size;
	int slot_id;
	struct xhci_ring *ep[USB_MAXENDPOINTS];
	struct xhci_input_context *in_ctx;
	struct xhci_device_context *out_ctx;
};

struct usb_root_hub_info {
	struct usb_hub_descriptor hub;
	struct usb_device_descriptor device;
	struct usb_config_descriptor config;
	struct usb_interface_descriptor interface;
	struct usb_endpoint_descriptor endpoint;
} __attribute__ ((packed));

struct xhci_hcd {
	struct usb_host host;
	struct device_d *dev;
	struct xhci_cap_regs __iomem *cap_regs;
	struct xhci_op_regs __iomem *op_regs;
	struct xhci_run_regs __iomem *run_regs;
	struct xhci_doorbell_array __iomem *dba;
	struct xhci_intr_reg __iomem *ir_set;
	/* Cached register copies of read-only HC data */
	u32 hcs_params1;
	u32 hcs_params2;
	u32 hcs_params3;
	u32 hcc_capbase;
	u32 hcc_params;
	u16 hci_version;
	int max_slots;
	int num_sp;
	int page_size;
	int page_shift;
	void *dma;
	size_t dma_size;
	__le64 *dcbaa;
	void *sp;
	__le64 *sp_array;
	struct xhci_ring cmd_ring;
	struct xhci_ring event_ring;
	struct xhci_ring *rings;
	struct list_head rings_list;
	struct xhci_erst_entry *event_erst;
	u8 *port_array;
	int rootdev;
	struct list_head vdev_list;
	u32 *ext_caps;
	unsigned int num_ext_caps;
	__le32 __iomem **usb_ports;
	unsigned int num_usb_ports;
	struct usb_root_hub_info usb_info;
};

#define to_xhci_hcd(_h)	\
	container_of(_h, struct xhci_hcd, host)

int xhci_handshake(void __iomem *p, u32 mask, u32 done, int usec);

int xhci_issue_command(struct xhci_hcd *xhci, union xhci_trb *trb);
int xhci_wait_for_event(struct xhci_hcd *xhci, u8 type, union xhci_trb *trb);

int xhci_virtdev_reset(struct xhci_virtual_device *vdev);
int xhci_virtdev_detach(struct xhci_virtual_device *vdev);

int xhci_hub_setup_ports(struct xhci_hcd *xhci);
void xhci_hub_port_power(struct xhci_hcd *xhci, int port, bool enable);
int xhci_hub_control(struct usb_device *dev, unsigned long pipe,
		     void *buffer, int length, struct devrequest *req);

static inline void xhci_print_trb(struct xhci_hcd *xhci,
				  union xhci_trb *trb, const char *desc)
{
	dev_dbg(xhci->dev, "%s [%08x %08x %08x %08x]\n", desc,
		trb->generic.field[0], trb->generic.field[1],
		trb->generic.field[2], trb->generic.field[3]);
}

#endif
