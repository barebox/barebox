/*
 * (C) Copyright 2001
 * Denis Peter, MPL AG Switzerland
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 *
 * Note: Part of this code has been derived from linux
 *
 */
#ifndef _USB_DEFS_H_
#define _USB_DEFS_H_

/* USB constants */

/* some HID sub classes */
#define USB_SUB_HID_NONE        0
#define USB_SUB_HID_BOOT        1

/* some UID Protocols */
#define USB_PROT_HID_NONE       0
#define USB_PROT_HID_KEYBOARD   1
#define USB_PROT_HID_MOUSE      2


/* Sub STORAGE Classes */
#define US_SC_RBC              1		/* Typically, flash devices */
#define US_SC_8020             2		/* CD-ROM */
#define US_SC_QIC              3		/* QIC-157 Tapes */
#define US_SC_UFI              4		/* Floppy */
#define US_SC_8070             5		/* Removable media */
#define US_SC_SCSI             6		/* Transparent */
#define US_SC_MIN              US_SC_RBC
#define US_SC_MAX              US_SC_SCSI

/* STORAGE Protocols */
#define US_PR_CB               1		/* Control/Bulk w/o interrupt */
#define US_PR_CBI              0		/* Control/Bulk/Interrupt */
#define US_PR_BULK             0x50		/* bulk only */

/* Descriptor types */
#define USB_DT_HID          (USB_TYPE_CLASS | 0x01)
#define USB_DT_REPORT       (USB_TYPE_CLASS | 0x02)
#define USB_DT_PHYSICAL     (USB_TYPE_CLASS | 0x03)

/* Descriptor sizes per descriptor type */
#define USB_DT_HID_SIZE         9

/* USB Packet IDs (PIDs) */
#define USB_PID_UNDEF_0             0xf0
#define USB_PID_OUT                 0xe1
#define USB_PID_ACK                 0xd2
#define USB_PID_DATA0               0xc3
#define USB_PID_UNDEF_4             0xb4
#define USB_PID_SOF                 0xa5
#define USB_PID_UNDEF_6             0x96
#define USB_PID_UNDEF_7             0x87
#define USB_PID_UNDEF_8             0x78
#define USB_PID_IN                  0x69
#define USB_PID_NAK                 0x5a
#define USB_PID_DATA1               0x4b
#define USB_PID_PREAMBLE            0x3c
#define USB_PID_SETUP               0x2d
#define USB_PID_STALL               0x1e
#define USB_PID_UNDEF_F             0x0f

/* HID requests */
#define USB_REQ_GET_REPORT          0x01
#define USB_REQ_GET_IDLE            0x02
#define USB_REQ_GET_PROTOCOL        0x03
#define USB_REQ_SET_REPORT          0x09
#define USB_REQ_SET_IDLE            0x0A
#define USB_REQ_SET_PROTOCOL        0x0B


/* "pipe" definitions */

#define PIPE_ISOCHRONOUS    0
#define PIPE_INTERRUPT      1
#define PIPE_CONTROL        2
#define PIPE_BULK           3
#define PIPE_DEVEP_MASK     0x0007ff00

#define USB_ISOCHRONOUS    0
#define USB_INTERRUPT      1
#define USB_CONTROL        2
#define USB_BULK           3

/* USB-status codes: */
#define USB_ST_ACTIVE           0x1		/* TD is active */
#define USB_ST_STALLED          0x2		/* TD is stalled */
#define USB_ST_BUF_ERR          0x4		/* buffer error */
#define USB_ST_BABBLE_DET       0x8		/* Babble detected */
#define USB_ST_NAK_REC          0x10	/* NAK Received*/
#define USB_ST_CRC_ERR          0x20	/* CRC/timeout Error */
#define USB_ST_BIT_ERR          0x40	/* Bitstuff error */
#define USB_ST_NOT_PROC         0x80000000L	/* Not yet processed */


/*************************************************************************
 * Hub defines
 */

/*
 * Port feature numbers
 */
#define USB_PORT_FEAT_HIGHSPEED      10

/* wPortStatus bits */
#define USB_PORT_STAT_SPEED	\
	(USB_PORT_STAT_LOW_SPEED | USB_PORT_STAT_HIGH_SPEED)

/* (shifted) direction/type/recipient from the USB 2.0 spec, table 9.2 */
#define DeviceRequest \
	((USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE) << 8)

#define DeviceOutRequest \
	((USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE) << 8)

#define InterfaceRequest \
	((USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE) << 8)

#define EndpointRequest \
	((USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE) << 8)

#define EndpointOutRequest \
	((USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_INTERFACE) << 8)

/* class requests from the USB 2.0 hub spec, table 11-15 */
/* GetBusState and SetHubDescriptor are optional, omitted */
#define ClearHubFeature		(0x2000 | USB_REQ_CLEAR_FEATURE)
#define ClearPortFeature	(0x2300 | USB_REQ_CLEAR_FEATURE)
#define GetHubDescriptor	(0xa000 | USB_REQ_GET_DESCRIPTOR)
#define GetHubStatus		(0xa000 | USB_REQ_GET_STATUS)
#define GetPortStatus		(0xa300 | USB_REQ_GET_STATUS)
#define SetHubFeature		(0x2000 | USB_REQ_SET_FEATURE)
#define SetPortFeature		(0x2300 | USB_REQ_SET_FEATURE)

#define USB_PORT_STAT_SUPER_SPEED   0x0600       /* faking support to XHCI */
#define USB_PORT_STAT_SPEED_MASK (USB_PORT_STAT_LOW_SPEED | USB_PORT_STAT_HIGH_SPEED)

#endif /*_USB_DEFS_H_ */
