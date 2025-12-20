/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_PROTOCOL_SNP_H_
#define __EFI_PROTOCOL_SNP_H_

#include <efi/types.h>

struct efi_network_statistics {
	uint64_t RxTotalFrames;
	uint64_t RxGoodFrames;
	uint64_t RxUndersizeFrames;
	uint64_t RxOversizeFrames;
	uint64_t RxDroppedFrames;
	uint64_t RxUnicastFrames;
	uint64_t RxBroadcastFrames;
	uint64_t RxMulticastFrames;
	uint64_t RxCrcErrorFrames;
	uint64_t RxTotalBytes;
	uint64_t TxTotalFrames;
	uint64_t TxGoodFrames;
	uint64_t TxUndersizeFrames;
	uint64_t TxOversizeFrames;
	uint64_t TxDroppedFrames;
	uint64_t TxUnicastFrames;
	uint64_t TxBroadcastFrames;
	uint64_t TxMulticastFrames;
	uint64_t TxCrcErrorFrames;
	uint64_t TxTotalBytes;
	uint64_t Collisions;
	uint64_t UnsupportedProtocol;
};

enum efi_simple_network_state {
	EfiSimpleNetworkStopped,
	EfiSimpleNetworkStarted,
	EfiSimpleNetworkInitialized,
	EfiSimpleNetworkMaxState
};

#define EFI_SIMPLE_NETWORK_RECEIVE_UNICAST               0x01
#define EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST             0x02
#define EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST             0x04
#define EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS           0x08
#define EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST 0x10

#define EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT        0x01
#define EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT       0x02
#define EFI_SIMPLE_NETWORK_COMMAND_INTERRUPT        0x04
#define EFI_SIMPLE_NETWORK_SOFTWARE_INTERRUPT       0x08

#define MAX_MCAST_FILTER_CNT    16
struct efi_simple_network_mode {
	uint32_t State;
	uint32_t HwAddressSize;
	uint32_t MediaHeaderSize;
	uint32_t MaxPacketSize;
	uint32_t NvRamSize;
	uint32_t NvRamAccessSize;
	uint32_t ReceiveFilterMask;
	uint32_t ReceiveFilterSetting;
	uint32_t MaxMCastFilterCount;
	uint32_t MCastFilterCount;
	struct efi_mac_address MCastFilter[MAX_MCAST_FILTER_CNT];
	struct efi_mac_address CurrentAddress;
	struct efi_mac_address BroadcastAddress;
	struct efi_mac_address PermanentAddress;
	uint8_t IfType;
        bool MacAddressChangeable;
	bool MultipleTxSupported;
	bool MediaPresentSupported;
	bool MediaPresent;
};

#define EFI_SIMPLE_NETWORK_INTERFACE_REVISION   0x00010000

struct efi_simple_network {
	uint64_t Revision;
	efi_status_t (EFIAPI *start) (struct efi_simple_network *This);
	efi_status_t (EFIAPI *stop) (struct efi_simple_network *This);
	efi_status_t (EFIAPI *initialize) (struct efi_simple_network *This,
			unsigned long ExtraRxBufferSize, unsigned long ExtraTxBufferSize);
	efi_status_t (EFIAPI *reset) (struct efi_simple_network *This, bool ExtendedVerification);
	efi_status_t (EFIAPI *shutdown) (struct efi_simple_network *This);
	efi_status_t (EFIAPI *receive_filters) (struct efi_simple_network *This,
			uint32_t Enable, uint32_t Disable, bool ResetMCastFilter,
			unsigned long MCastFilterCnt, struct efi_mac_address *MCastFilter);
	efi_status_t (EFIAPI *station_address) (struct efi_simple_network *This,
			bool Reset, struct efi_mac_address *New);
	efi_status_t (EFIAPI *statistics) (struct efi_simple_network *This,
			bool Reset, unsigned long *StatisticsSize,
			struct efi_network_statistics *StatisticsTable);
	efi_status_t (EFIAPI *mcast_ip_to_mac) (struct efi_simple_network *This,
			bool IPv6, union efi_ip_address *IP, struct efi_mac_address *MAC);
	efi_status_t (EFIAPI *nvdata) (struct efi_simple_network *This,
			bool ReadWrite, unsigned long Offset, unsigned long BufferSize,
			void *Buffer);
	efi_status_t (EFIAPI *get_status) (struct efi_simple_network *This,
			uint32_t *InterruptStatus, void **TxBuf);
	efi_status_t (EFIAPI *transmit) (struct efi_simple_network *This,
			unsigned long HeaderSize, unsigned long BufferSize, void *Buffer,
			struct efi_mac_address *SrcAddr, struct efi_mac_address *DestAddr,
			uint16_t *Protocol);
	efi_status_t (EFIAPI *receive) (struct efi_simple_network *This,
			unsigned long *HeaderSize, unsigned long *BufferSize, void *Buffer,
			struct efi_mac_address *SrcAddr, struct efi_mac_address *DestAddr, uint16_t *Protocol);
	void *WaitForPacket;
	struct efi_simple_network_mode *Mode;
};

#endif
