// SPDX-License-Identifier: GPL-2.0-only
/*
 * efi-snp.c - Simple Network Protocol driver for EFI
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <efi.h>
#include <efi/efi.h>
#include <efi/efi-device.h>

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
	efi_mac_address MCastFilter[MAX_MCAST_FILTER_CNT];
	efi_mac_address CurrentAddress;
	efi_mac_address BroadcastAddress;
	efi_mac_address PermanentAddress;
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
			unsigned long MCastFilterCnt, efi_mac_address *MCastFilter);
	efi_status_t (EFIAPI *station_address) (struct efi_simple_network *This,
			bool Reset, efi_mac_address *New);
	efi_status_t (EFIAPI *statistics) (struct efi_simple_network *This,
			bool Reset, unsigned long *StatisticsSize,
			struct efi_network_statistics *StatisticsTable);
	efi_status_t (EFIAPI *mcast_ip_to_mac) (struct efi_simple_network *This,
			bool IPv6, efi_ip_address *IP, efi_mac_address *MAC);
	efi_status_t (EFIAPI *nvdata) (struct efi_simple_network *This,
			bool ReadWrite, unsigned long Offset, unsigned long BufferSize,
			void *Buffer);
	efi_status_t (EFIAPI *get_status) (struct efi_simple_network *This,
			uint32_t *InterruptStatus, void **TxBuf);
	efi_status_t (EFIAPI *transmit) (struct efi_simple_network *This,
			unsigned long HeaderSize, unsigned long BufferSize, void *Buffer,
			efi_mac_address *SrcAddr, efi_mac_address *DestAddr,
			uint16_t *Protocol);
	efi_status_t (EFIAPI *receive) (struct efi_simple_network *This,
			unsigned long *HeaderSize, unsigned long *BufferSize, void *Buffer,
			efi_mac_address *SrcAddr, efi_mac_address *DestAddr, uint16_t *Protocol);
	void *WaitForPacket;
	struct efi_simple_network_mode *Mode;
};

struct efi_snp_priv {
	struct device_d *dev;
	struct eth_device edev;
	struct efi_simple_network *snp;
};

static inline struct efi_snp_priv *to_priv(struct eth_device *edev)
{
	return container_of(edev, struct efi_snp_priv, edev);
}

static int efi_snp_eth_send(struct eth_device *edev, void *packet, int length)
{
	struct efi_snp_priv *priv = to_priv(edev);
	efi_status_t efiret;
	void *txbuf;
	uint64_t start;

	efiret = priv->snp->transmit(priv->snp, 0, length, packet, NULL, NULL, NULL);
	if (EFI_ERROR(efiret)) {
		dev_err(priv->dev, "failed to send: %s\n", efi_strerror(efiret));
		return -efi_errno(efiret);
	}

	start = get_time_ns();

	while (!is_timeout(start, SECOND)) {
		uint32_t irq;
		priv->snp->get_status(priv->snp, &irq, &txbuf);
		if (txbuf)
			return 0;
	}

	dev_err(priv->dev, "tx time out\n");

	return -ETIMEDOUT;
}

static int efi_snp_eth_rx(struct eth_device *edev)
{
	struct efi_snp_priv *priv = to_priv(edev);
	long bufsize = PKTSIZE;
	efi_status_t efiret;

	efiret = priv->snp->receive(priv->snp, NULL, &bufsize, NetRxPackets[0], NULL, NULL, NULL);
	if (efiret == EFI_NOT_READY)
		return 0;

	if (EFI_ERROR(efiret)) {
		dev_err(priv->dev, "failed to receive: %s\n", efi_strerror(efiret));
		return -efi_errno(efiret);
	}

	net_receive(edev, NetRxPackets[0], bufsize);

	return 0;
}

static int efi_snp_eth_open(struct eth_device *edev)
{
	struct efi_snp_priv *priv = to_priv(edev);
	uint32_t mask;
	efi_status_t efiret;

	priv->snp->shutdown(priv->snp);
	priv->snp->stop(priv->snp);
	priv->snp->start(priv->snp);
	efiret = priv->snp->initialize(priv->snp, 0, 0);
	if (EFI_ERROR(efiret)) {
		dev_err(priv->dev, "Initialize failed with: %s\n", efi_strerror(efiret));
		return -efi_errno(efiret);
	}

	efiret = priv->snp->station_address(priv->snp, false,
			(efi_mac_address *)priv->snp->Mode->PermanentAddress.Addr );
	if (EFI_ERROR(efiret)) {
		dev_err(priv->dev, "failed to set MAC address: %s\n",
				efi_strerror(efiret));
		return -efi_errno(efiret);
	}

	mask = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST |
			EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST |
			EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS;
	efiret = priv->snp->receive_filters(priv->snp, mask, 0, 0, 0, 0);
	if (EFI_ERROR(efiret)) {
		dev_err(priv->dev, "failed to set receive filters: %s\n",
				efi_strerror(efiret));
		return -efi_errno(efiret);
	}

	return 0;
}

static void efi_snp_eth_halt(struct eth_device *edev)
{
	struct efi_snp_priv *priv = to_priv(edev);

	priv->snp->stop(priv->snp);
}

static int efi_snp_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct efi_snp_priv *priv = to_priv(edev);

	memcpy(adr, priv->snp->Mode->PermanentAddress.Addr, 6);

	return 0;
}

static int efi_snp_set_ethaddr(struct eth_device *edev, const unsigned char *adr)
{
	return 0;
}

int efi_snp_probe(struct efi_device *efidev)
{
	struct eth_device *edev;
	struct efi_snp_priv *priv;
	int ret;

	dev_dbg(&efidev->dev, "efi_snp_probe\n");

	priv = xzalloc(sizeof(struct efi_snp_priv));
	priv->snp = efidev->protocol;
	priv->dev = &efidev->dev;

	dev_dbg(&efidev->dev, "perm: %02x:%02x:%02x:%02x:%02x:%02x\n",
			priv->snp->Mode->PermanentAddress.Addr[0],
			priv->snp->Mode->PermanentAddress.Addr[1],
			priv->snp->Mode->PermanentAddress.Addr[2],
			priv->snp->Mode->PermanentAddress.Addr[3],
			priv->snp->Mode->PermanentAddress.Addr[4],
			priv->snp->Mode->PermanentAddress.Addr[5]);
	dev_dbg(&efidev->dev, "curr: %02x:%02x:%02x:%02x:%02x:%02x\n",
			priv->snp->Mode->CurrentAddress.Addr[0],
			priv->snp->Mode->CurrentAddress.Addr[1],
			priv->snp->Mode->CurrentAddress.Addr[2],
			priv->snp->Mode->CurrentAddress.Addr[3],
			priv->snp->Mode->CurrentAddress.Addr[4],
			priv->snp->Mode->CurrentAddress.Addr[5]);

	edev = &priv->edev;
	edev->priv = priv;
	edev->parent = &efidev->dev;

	edev->open = efi_snp_eth_open;
	edev->send = efi_snp_eth_send;
	edev->recv = efi_snp_eth_rx;
	edev->halt = efi_snp_eth_halt;
	edev->get_ethaddr = efi_snp_get_ethaddr;
	edev->set_ethaddr = efi_snp_set_ethaddr;

	ret = eth_register(edev);

        return ret;
}

static struct efi_driver efi_snp_driver = {
        .driver = {
		.name  = "efi-snp",
	},
        .probe = efi_snp_probe,
	.guid = EFI_SIMPLE_NETWORK_PROTOCOL_GUID,
};
device_efi_driver(efi_snp_driver);
