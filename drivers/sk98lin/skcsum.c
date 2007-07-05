/******************************************************************************
 *
 * Name:	skcsum.c
 * Project:	GEnesis, PCI Gigabit Ethernet Adapter
 * Version:	$Revision: 1.10 $
 * Date:	$Date: 2002/04/11 10:02:04 $
 * Purpose:	Store/verify Internet checksum in send/receive packets.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *	(C)Copyright 1998-2001 SysKonnect GmbH.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	The information in this file is provided "AS IS" without warranty.
 *
 ******************************************************************************/

/******************************************************************************
 *
 * History:
 *
 *	$Log: skcsum.c,v $
 *	Revision 1.10  2002/04/11 10:02:04  rwahl
 *	Fix in SkCsGetSendInfo():
 *	- function did not return ProtocolFlags in every case.
 *	- pseudo header csum calculated wrong for big endian.
 *
 *	Revision 1.9  2001/06/13 07:42:08  gklug
 *	fix: NetNumber was wrong in CLEAR_STAT event
 *	add: check for good NetNumber in Clear STAT
 *
 *	Revision 1.8  2001/02/06 11:15:36  rassmann
 *	Supporting two nets on dual-port adapters.
 *
 *	Revision 1.7  2000/06/29 13:17:05  rassmann
 *	Corrected reception of a packet with UDP checksum == 0 (which means there
 *	is no UDP checksum).
 *
 *	Revision 1.6  2000/02/21 12:35:10  cgoos
 *	Fixed license header comment.
 *
 *	Revision 1.5  2000/02/21 11:05:19  cgoos
 *	Merged changes back to common source.
 *	Fixed rx path for BIG ENDIAN architecture.
 *
 *	Revision 1.1  1999/07/26 15:28:12  mkarl
 *	added return SKCS_STATUS_IP_CSUM_ERROR_UDP and
 *	SKCS_STATUS_IP_CSUM_ERROR_TCP to pass the NidsTester
 *	changed from common source to windows specific source
 *	therefore restarting with v1.0
 *
 *	Revision 1.3  1999/05/10 08:39:33  mkarl
 *	prevent overflows in SKCS_HTON16
 *	fixed a bug in pseudo header checksum calculation
 *	added some comments
 *
 *	Revision 1.2  1998/10/22 11:53:28  swolf
 *	Now using SK_DBG_MSG.
 *
 *	Revision 1.1  1998/09/01 15:35:41  swolf
 *	initial revision
 *
 *	13-May-1998 sw	Created.
 *
 ******************************************************************************/

#include <config.h>

#ifdef CONFIG_SK98

#ifdef SK_USE_CSUM	/* Check if CSUM is to be used. */

#ifndef lint
static const char SysKonnectFileId[] = "@(#)"
	"$Id: skcsum.c,v 1.10 2002/04/11 10:02:04 rwahl Exp $"
	" (C) SysKonnect.";
#endif	/* !lint */

/******************************************************************************
 *
 * Description:
 *
 * This is the "GEnesis" common module "CSUM".
 *
 * This module contains the code necessary to calculate, store, and verify the
 * Internet Checksum of IP, TCP, and UDP frames.
 *
 * "GEnesis" is an abbreviation of "Gigabit Ethernet Network System in Silicon"
 * and is the code name of this SysKonnect project.
 *
 * Compilation Options:
 *
 *	SK_USE_CSUM - Define if CSUM is to be used. Otherwise, CSUM will be an
 *	empty module.
 *
 *	SKCS_OVERWRITE_PROTO - Define to overwrite the default protocol id
 *	definitions. In this case, all SKCS_PROTO_xxx definitions must be made
 *	external.
 *
 *	SKCS_OVERWRITE_STATUS - Define to overwrite the default return status
 *	definitions. In this case, all SKCS_STATUS_xxx definitions must be made
 *	external.
 *
 * Include File Hierarchy:
 *
 *	"h/skdrv1st.h"
 *	"h/skcsum.h"
 *	 "h/sktypes.h"
 *	 "h/skqueue.h"
 *	"h/skdrv2nd.h"
 *
 ******************************************************************************/

#include "h/skdrv1st.h"
#include "h/skcsum.h"
#include "h/skdrv2nd.h"

/* defines ********************************************************************/

/* The size of an Ethernet MAC header. */
#define SKCS_ETHERNET_MAC_HEADER_SIZE			(6+6+2)

/* The size of the used topology's MAC header. */
#define	SKCS_MAC_HEADER_SIZE	SKCS_ETHERNET_MAC_HEADER_SIZE

/* The size of the IP header without any option fields. */
#define SKCS_IP_HEADER_SIZE						20

/*
 * Field offsets within the IP header.
 */

/* "Internet Header Version" and "Length". */
#define SKCS_OFS_IP_HEADER_VERSION_AND_LENGTH	0

/* "Total Length". */
#define SKCS_OFS_IP_TOTAL_LENGTH				2

/* "Flags" "Fragment Offset". */
#define SKCS_OFS_IP_FLAGS_AND_FRAGMENT_OFFSET	6

/* "Next Level Protocol" identifier. */
#define SKCS_OFS_IP_NEXT_LEVEL_PROTOCOL			9

/* Source IP address. */
#define SKCS_OFS_IP_SOURCE_ADDRESS				12

/* Destination IP address. */
#define SKCS_OFS_IP_DESTINATION_ADDRESS			16


/*
 * Field offsets within the UDP header.
 */

/* UDP checksum. */
#define SKCS_OFS_UDP_CHECKSUM					6

/* IP "Next Level Protocol" identifiers (see RFC 790). */
#define SKCS_PROTO_ID_TCP		6	/* Transport Control Protocol */
#define SKCS_PROTO_ID_UDP		17	/* User Datagram Protocol */

/* IP "Don't Fragment" bit. */
#define SKCS_IP_DONT_FRAGMENT	SKCS_HTON16(0x4000)

/* Add a byte offset to a pointer. */
#define SKCS_IDX(pPtr, Ofs)	((void *) ((char *) (pPtr) + (Ofs)))

/*
 * Macros that convert host to network representation and vice versa, i.e.
 * little/big endian conversion on little endian machines only.
 */
#ifdef SK_LITTLE_ENDIAN
#define SKCS_HTON16(Val16)	(((unsigned) (Val16) >> 8) | (((Val16) & 0xFF) << 8))
#endif	/* SK_LITTLE_ENDIAN */
#ifdef SK_BIG_ENDIAN
#define SKCS_HTON16(Val16)	(Val16)
#endif	/* SK_BIG_ENDIAN */
#define SKCS_NTOH16(Val16)	SKCS_HTON16(Val16)

/* typedefs *******************************************************************/

/* function prototypes ********************************************************/

/******************************************************************************
 *
 *	SkCsGetSendInfo - get checksum information for a send packet
 *
 * Description:
 *	Get all checksum information necessary to send a TCP or UDP packet. The
 *	function checks the IP header passed to it. If the high-level protocol
 *	is either TCP or UDP the pseudo header checksum is calculated and
 *	returned.
 *
 *	The function returns the total length of the IP header (including any
 *	IP option fields), which is the same as the start offset of the IP data
 *	which in turn is the start offset of the TCP or UDP header.
 *
 *	The function also returns the TCP or UDP pseudo header checksum, which
 *	should be used as the start value for the hardware checksum calculation.
 *	(Note that any actual pseudo header checksum can never calculate to
 *	zero.)
 *
 * Note:
 *	There is a bug in the ASIC which may lead to wrong checksums.
 *
 * Arguments:
 *	pAc - A pointer to the adapter context struct.
 *
 *	pIpHeader - Pointer to IP header. Must be at least the IP header *not*
 *	including any option fields, i.e. at least 20 bytes.
 *
 *	Note: This pointer will be used to address 8-, 16-, and 32-bit
 *	variables with the respective alignment offsets relative to the pointer.
 *	Thus, the pointer should point to a 32-bit aligned address. If the
 *	target system cannot address 32-bit variables on non 32-bit aligned
 *	addresses, then the pointer *must* point to a 32-bit aligned address.
 *
 *	pPacketInfo - A pointer to the packet information structure for this
 *	packet. Before calling this SkCsGetSendInfo(), the following field must
 *	be initialized:
 *
 *		ProtocolFlags - Initialize with any combination of
 *		SKCS_PROTO_XXX bit flags. SkCsGetSendInfo() will only work on
 *		the protocols specified here. Any protocol(s) not specified
 *		here will be ignored.
 *
 *		Note: Only one checksum can be calculated in hardware. Thus, if
 *		SKCS_PROTO_IP is specified in the 'ProtocolFlags',
 *		SkCsGetSendInfo() must calculate the IP header checksum in
 *		software. It might be a better idea to have the calling
 *		protocol stack calculate the IP header checksum.
 *
 * Returns: N/A
 *	On return, the following fields in 'pPacketInfo' may or may not have
 *	been filled with information, depending on the protocol(s) found in the
 *	packet:
 *
 *	ProtocolFlags - Returns the SKCS_PROTO_XXX bit flags of the protocol(s)
 *	that were both requested by the caller and actually found in the packet.
 *	Protocol(s) not specified by the caller and/or not found in the packet
 *	will have their respective SKCS_PROTO_XXX bit flags reset.
 *
 *	Note: For IP fragments, TCP and UDP packet information is ignored.
 *
 *	IpHeaderLength - The total length in bytes of the complete IP header
 *	including any option fields is returned here. This is the start offset
 *	of the IP data, i.e. the TCP or UDP header if present.
 *
 *	IpHeaderChecksum - If IP has been specified in the 'ProtocolFlags', the
 *	16-bit Internet Checksum of the IP header is returned here. This value
 *	is to be stored into the packet's 'IP Header Checksum' field.
 *
 *	PseudoHeaderChecksum - If this is a TCP or UDP packet and if TCP or UDP
 *	has been specified in the 'ProtocolFlags', the 16-bit Internet Checksum
 *	of the TCP or UDP pseudo header is returned here.
 */


/******************************************************************************
 *
 *	SkCsSetReceiveFlags - set checksum receive flags
 *
 * Description:
 *	Use this function to set the various receive flags. According to the
 *	protocol flags set by the caller, the start offsets within received
 *	packets of the two hardware checksums are returned. These offsets must
 *	be stored in all receive descriptors.
 *
 * Arguments:
 *	pAc - Pointer to adapter context struct.
 *
 *	ReceiveFlags - Any combination of SK_PROTO_XXX flags of the protocols
 *	for which the caller wants checksum information on received frames.
 *
 *	pChecksum1Offset - The start offset of the first receive descriptor
 *	hardware checksum to be calculated for received frames is returned
 *	here.
 *
 *	pChecksum2Offset - The start offset of the second receive descriptor
 *	hardware checksum to be calculated for received frames is returned
 *	here.
 *
 * Returns: N/A
 *	Returns the two hardware checksum start offsets.
 */
void SkCsSetReceiveFlags(
SK_AC		*pAc,				/* Adapter context struct. */
unsigned	ReceiveFlags,		/* New receive flags. */
unsigned	*pChecksum1Offset,	/* Offset for hardware checksum 1. */
unsigned	*pChecksum2Offset,	/* Offset for hardware checksum 2. */
int			NetNumber)
{
	/* Save the receive flags. */

	pAc->Csum.ReceiveFlags[NetNumber] = ReceiveFlags;

	/* First checksum start offset is the IP header. */
	*pChecksum1Offset = SKCS_MAC_HEADER_SIZE;

	/*
	 * Second checksum start offset is the IP data. Note that this may vary
	 * if there are any IP header options in the actual packet.
	 */
	*pChecksum2Offset = SKCS_MAC_HEADER_SIZE + SKCS_IP_HEADER_SIZE;
}	/* SkCsSetReceiveFlags */

#ifndef SkCsCalculateChecksum

/******************************************************************************
 *
 *	SkCsCalculateChecksum - calculate checksum for specified data
 *
 * Description:
 *	Calculate and return the 16-bit Internet Checksum for the specified
 *	data.
 *
 * Arguments:
 *	pData - Pointer to data for which the checksum shall be calculated.
 *	Note: The pointer should be aligned on a 16-bit boundary.
 *
 *	Length - Length in bytes of data to checksum.
 *
 * Returns:
 *	The 16-bit Internet Checksum for the specified data.
 *
 *	Note: The checksum is calculated in the machine's natural byte order,
 *	i.e. little vs. big endian. Thus, the resulting checksum is different
 *	for the same input data on little and big endian machines.
 *
 *	However, when written back to the network packet, the byte order is
 *	always in correct network order.
 */
unsigned SkCsCalculateChecksum(
void		*pData,		/* Data to checksum. */
unsigned	Length)		/* Length of data. */
{
	SK_U16 *pU16;		/* Pointer to the data as 16-bit words. */
	unsigned long Checksum;	/* Checksum; must be at least 32 bits. */

	/* Sum up all 16-bit words. */

	pU16 = (SK_U16 *) pData;
	for (Checksum = 0; Length > 1; Length -= 2) {
		Checksum += *pU16++;
	}

	/* If this is an odd number of bytes, add-in the last byte. */

	if (Length > 0) {
#ifdef SK_BIG_ENDIAN
		/* Add the last byte as the high byte. */
		Checksum += ((unsigned) *(SK_U8 *) pU16) << 8;
#else	/* !SK_BIG_ENDIAN */
		/* Add the last byte as the low byte. */
		Checksum += *(SK_U8 *) pU16;
#endif	/* !SK_BIG_ENDIAN */
	}

	/* Add-in any carries. */

	SKCS_OC_ADD(Checksum, Checksum, 0);

	/* Add-in any new carry. */

	SKCS_OC_ADD(Checksum, Checksum, 0);

	/* Note: All bits beyond the 16-bit limit are now zero. */

	return ((unsigned) Checksum);
}	/* SkCsCalculateChecksum */

#endif /* SkCsCalculateChecksum */

/******************************************************************************
 *
 *	SkCsEvent - the CSUM event dispatcher
 *
 * Description:
 *	This is the event handler for the CSUM module.
 *
 * Arguments:
 *	pAc - Pointer to adapter context.
 *
 *	Ioc - I/O context.
 *
 *	Event -	 Event id.
 *
 *	Param - Event dependent parameter.
 *
 * Returns:
 *	The 16-bit Internet Checksum for the specified data.
 *
 *	Note: The checksum is calculated in the machine's natural byte order,
 *	i.e. little vs. big endian. Thus, the resulting checksum is different
 *	for the same input data on little and big endian machines.
 *
 *	However, when written back to the network packet, the byte order is
 *	always in correct network order.
 */
int SkCsEvent(
SK_AC		*pAc,	/* Pointer to adapter context. */
SK_IOC		Ioc,	/* I/O context. */
SK_U32		Event,	/* Event id. */
SK_EVPARA	Param)	/* Event dependent parameter. */
{
	int ProtoIndex;
	int	NetNumber;

	switch (Event) {
	/*
	 * Clear protocol statistics.
	 *
	 * Param - Protocol index, or -1 for all protocols.
	 *		 - Net number.
	 */
	case SK_CSUM_EVENT_CLEAR_PROTO_STATS:

		ProtoIndex = (int)Param.Para32[1];
		NetNumber = (int)Param.Para32[0];
		if (ProtoIndex < 0) {	/* Clear for all protocols. */
			if (NetNumber >= 0) {
				memset(&pAc->Csum.ProtoStats[NetNumber][0], 0,
					sizeof(pAc->Csum.ProtoStats[NetNumber]));
			}
		}
		else {					/* Clear for individual protocol. */
			memset(&pAc->Csum.ProtoStats[NetNumber][ProtoIndex], 0,
				sizeof(pAc->Csum.ProtoStats[NetNumber][ProtoIndex]));
		}
		break;
	default:
		break;
	}
	return (0);	/* Success. */
}	/* SkCsEvent */

#endif	/* SK_USE_CSUM */

#endif /* CONFIG_SK98 */
