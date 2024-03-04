#ifndef __EFI_DEVICE_PATH_H
#define __EFI_DEVICE_PATH_H

#include <efi/types.h>

/*
 * Hardware Device Path (UEFI 2.4 specification, version 2.4 § 9.3.2.)
 */

#define HARDWARE_DEVICE_PATH            0x01

#define HW_PCI_DP                       0x01
struct pci_device_path {
	struct efi_device_path header;
	u8 Function;
	u8 Device;
};

#define HW_PCCARD_DP                    0x02
struct pccard_device_path {
	struct efi_device_path header;
	u8 function_number;
};

#define HW_MEMMAP_DP                    0x03
struct memmap_device_path {
	struct efi_device_path header;
	u32 memory_type;
	efi_physical_addr_t starting_address;
	efi_physical_addr_t ending_address;
};

#define HW_VENDOR_DP                    0x04
struct vendor_device_path {
	struct efi_device_path header;
	efi_guid_t Guid;
};

struct unknown_device_vendor_device_path {
	struct vendor_device_path device_path;
	u8 legacy_drive_letter;
};

#define HW_CONTROLLER_DP            0x05
struct controller_device_path {
	struct efi_device_path header;
	u32 Controller;
};

/*
 * ACPI Device Path (UEFI 2.4 specification, version 2.4 § 9.3.3 and 9.3.4.)
 */
#define ACPI_DEVICE_PATH                 0x02

#define ACPI_DP                         0x01
struct acpi_hid_device_path {
	struct efi_device_path header;
	u32 HID;
	u32 UID;
};

#define EXPANDED_ACPI_DP		0x02
struct expanded_acpi_hid_device_path {
	struct efi_device_path header;
	u32 HID;
	u32 UID;
	u32 CID;
	u8 hid_str[1];
};

#define ACPI_ADR_DP 3
struct acpi_adr_device_path {
	struct efi_device_path header;
	u32 ADR;
};

/*
 * EISA ID Macro
 * EISA ID Definition 32-bits
 *  bits[15:0] - three character compressed ASCII EISA ID.
 *  bits[31:16] - binary number
 *   Compressed ASCII is 5 bits per character 0b00001 = 'A' 0b11010 = 'Z'
 */
#define PNP_EISA_ID_CONST       0x41d0
#define EISA_ID(_Name, _Num)    ((u32) ((_Name) | (_Num) << 16))
#define EISA_PNP_ID(_PNPId)     (EISA_ID(PNP_EISA_ID_CONST, (_PNPId)))

#define PNP_EISA_ID_MASK        0xffff
#define EISA_ID_TO_NUM(_Id)     ((_Id) >> 16)

/*
 * Messaging Device Path (UEFI 2.4 specification, version 2.4 § 9.3.5.)
 */
#define MESSAGING_DEVICE_PATH           0x03

#define MSG_ATAPI_DP                    0x01
struct atapi_device_path {
	struct efi_device_path header;
	u8 primary_secondary;
	u8 slave_master;
	u16 Lun;
};

#define MSG_SCSI_DP                     0x02
struct scsi_device_path {
	struct efi_device_path header;
	u16 Pun;
	u16 Lun;
};

#define MSG_FIBRECHANNEL_DP             0x03
struct fibrechannel_device_path {
	struct efi_device_path header;
	u32 Reserved;
	u64 WWN;
	u64 Lun;
};

/**
 * Fibre Channel Ex sub_type.
 * UEFI 2.0 specification version 2.4 § 9.3.5.6.
 */
#define MSG_FIBRECHANNELEX_DP 21
struct fibrechannelex_device_path {
	struct efi_device_path header;
	u32 Reserved;
	u8 WWN[8];		/* World Wide Name */
	u8 Lun[8];		/* Logical unit, T-10 SCSI Architecture Model 4 specification */
};

#define MSG_1394_DP                     0x04
struct f1394_device_path {
	struct efi_device_path header;
	u32 Reserved;
	u64 Guid;
};

#define MSG_USB_DP                      0x05
struct usb_device_path {
	struct efi_device_path header;
	u8 Port;
	u8 Endpoint;
};

/**
 * SATA Device Path sub_type.
 * UEFI 2.0 specification version 2.4 § 9.3.5.6.
 */
#define MSG_SATA_DP 18
struct sata_device_path {
	struct efi_device_path header;
	u16 HBAPort_number;
	u16 port_multiplier_port_number;
	u16 Lun;		/* Logical Unit Number */
};

/**
 * USB WWID Device Path sub_type.
 * UEFI 2.0 specification version 2.4 § 9.3.5.7.
 */
#define MSG_USB_WWID_DP 16
struct usb_wwid_device_path {
	struct efi_device_path header;
	u16 interface_number;
	u16 vendor_id;
	u16 product_id;
	s16 serial_number[1];	/* UTF-16 characters of the USB serial number */
};

/**
 * Device Logical Unit sub_type.
 * UEFI 2.0 specification version 2.4 § 9.3.5.8.
 */
#define MSG_DEVICE_LOGICAL_UNIT_DP 17
struct device_logical_unit_device_path {
	struct efi_device_path header;
	u8 Lun;			/* Logical Unit Number */
};

#define MSG_USB_CLASS_DP                0x0_f
struct usb_class_device_path {
	struct efi_device_path header;
	u16 vendor_id;
	u16 product_id;
	u8 device_class;
	u8 device_subclass;
	u8 device_protocol;
};

#define MSG_I2_o_DP                      0x06
struct i2_o_device_path {
	struct efi_device_path header;
	u32 Tid;
};

#define MSG_MAC_ADDR_DP                 0x0b
struct mac_addr_device_path {
	struct efi_device_path header;
	struct efi_mac_address mac_address;
	u8 if_type;
};

#define MSG_IPv4_DP                     0x0c
struct ipv4_device_path {
	struct efi_device_path header;
	struct efi_ipv4_address local_ip_address;
	struct efi_ipv4_address remote_ip_address;
	u16 local_port;
	u16 remote_port;
	u16 Protocol;
	bool static_ip_address;
	/* new from UEFI version 2, code must check length field in header */
	struct efi_ipv4_address gateway_ip_address;
	struct efi_ipv4_address subnet_mask;
};

#define MSG_IPv6_DP                     0x0d
struct ipv6_device_path {
	struct efi_device_path header;
	struct efi_ipv6_address local_ip_address;
	struct efi_ipv6_address remote_ip_address;
	u16 local_port;
	u16 remote_port;
	u16 Protocol;
	bool IPAddress_origin;
	/* new from UEFI version 2, code must check length field in header */
	u8 prefix_length;
	struct efi_ipv6_address gateway_ip_address;
};

/**
 * Device Logical Unit sub_type.
 * UEFI 2.0 specification version 2.4 § 9.3.5.8.
 */
#define MSG_VLAN_DP 20
struct vlan_device_path {
	struct efi_device_path header;
	u16 vlan_id;
};

#define MSG_INFINIBAND_DP               0x09
struct infiniband_device_path {
	struct efi_device_path header;
	u32 resource_flags;
	efi_guid_t port_gid;
	u64 service_id;
	u64 target_port_id;
	u64 device_id;
};

#define MSG_UART_DP                     0x0e
struct uart_device_path {
	struct efi_device_path header;
	u32 Reserved;
	u64 baud_rate;
	u8 data_bits;
	u8 Parity;
	u8 stop_bits;
};

#define MSG_VENDOR_DP                   0x0a
/* Use VENDOR_DEVICE_PATH struct */

#define DEVICE_PATH_MESSAGING_PC_ANSI \
    { 0xe0c14753, 0xf9be, 0x11d2,  {0x9a, 0x0c, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}  }

#define DEVICE_PATH_MESSAGING_VT_100 \
    { 0xdfa66065, 0xb419, 0x11d3,  {0x9a, 0x2d, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}  }

#define DEVICE_PATH_MESSAGING_VT_100_PLUS \
    { 0x7baec70b , 0x57e0 , 0x4c76 , { 0x8e , 0x87 , 0x2f , 0x9e , 0x28 , 0x08 , 0x83 , 0x43 } }

#define DEVICE_PATH_MESSAGING_VT_UTF8 \
    { 0xad15a0d6 , 0x8bec , 0x4acf , { 0xa0 , 0x73 , 0xd0 , 0x1d , 0xe7 , 0x7e , 0x2d , 0x88 } }

#define EFI_PC_ANSI_GUID \
    { 0xe0c14753 , 0xf9be , 0x11d2 , 0x9a , 0x0c , 0x00 , 0x90 , 0x27 , 0x3f , 0xc1 , 0x4d }

#define EFI_VT_100_GUID \
    { 0xdfa66065 , 0xb419 , 0x11d3 , 0x9a , 0x2d , 0x00 , 0x90 , 0x27 , 0x3f , 0xc1 , 0x4d }

#define EFI_VT_100_PLUS_GUID \
    { 0x7baec70b , 0x57e0 , 0x4c76 , 0x8e , 0x87 , 0x2f , 0x9e , 0x28 , 0x08 , 0x83 , 0x43 }

#define EFI_VT_UTF8_GUID \
    { 0xad15a0d6 , 0x8bec , 0x4acf , 0xa0 , 0x73 , 0xd0 , 0x1d , 0xe7 , 0x7e , 0x2d , 0x88 }

/*
 * Media Device Path (UEFI 2.4 specification, version 2.4 § 9.3.6.)
 */
#define MEDIA_DEVICE_PATH               0x04

#define MEDIA_HARDDRIVE_DP              0x01
struct harddrive_device_path {
	struct efi_device_path header;
	u32 partition_number;
	u64 partition_start;
	u64 partition_size;
	u8 signature[16];
	u8 mbr_type;
	u8 signature_type;
};

#define MBR_TYPE_PCAT                       0x01
#define MBR_TYPE_EFI_PARTITION_TABLE_HEADER 0x02

#define SIGNATURE_TYPE_MBR                  0x01
#define SIGNATURE_TYPE_GUID                 0x02

#define MEDIA_CDROM_DP                  0x02
struct cdrom_device_path {
	struct efi_device_path header;
	u32 boot_entry;
	u64 partition_start;
	u64 partition_size;
};

#define MEDIA_VENDOR_DP                 0x03
/* Use VENDOR_DEVICE_PATH struct */

#define MEDIA_FILEPATH_DP               0x04
struct filepath_device_path {
	struct efi_device_path header;
	s16 path_name[1];
};

#define SIZE_OF_FILEPATH_DEVICE_PATH offsetof(FILEPATH_DEVICE_PATH,path_name)

#define MEDIA_PROTOCOL_DP               0x05
struct media_protocol_device_path {
	struct efi_device_path header;
	efi_guid_t Protocol;
};

/**
 * PIWG Firmware File sub_type.
 * UEFI 2.0 specification version 2.4 § 9.3.6.6.
 */
#define MEDIA_PIWG_FW_FILE_DP 6
struct media_fw_vol_filepath_device_path {
	struct efi_device_path header;
	efi_guid_t fv_file_name;
};

/**
 * PIWG Firmware Volume Device Path sub_type.
 * UEFI 2.0 specification version 2.4 § 9.3.6.7.
 */
#define MEDIA_PIWG_FW_VOL_DP 7
struct media_fw_vol_device_path {
	struct efi_device_path header;
	efi_guid_t fv_name;
};

/**
 * Media relative offset range device path.
 * UEFI 2.0 specification version 2.4 § 9.3.6.8.
 */
#define MEDIA_RELATIVE_OFFSET_RANGE_DP 8
struct media_relative_offset_range_device_path {
	struct efi_device_path header;
	u32 Reserved;
	u64 starting_offset;
	u64 ending_offset;
};

/*
 * BIOS Boot Specification Device Path (UEFI 2.4 specification, version 2.4 § 9.3.7.)
 */
#define BBS_DEVICE_PATH                 0x05

#define BBS_BBS_DP                      0x01
struct bbs_bbs_device_path {
	struct efi_device_path header;
	u16 device_type;
	u16 status_flag;
	s8 String[1];
};

/* device_type definitions - from BBS specification */
#define BBS_TYPE_FLOPPY                 0x01
#define BBS_TYPE_HARDDRIVE              0x02
#define BBS_TYPE_CDROM                  0x03
#define BBS_TYPE_PCMCIA                 0x04
#define BBS_TYPE_USB                    0x05
#define BBS_TYPE_EMBEDDED_NETWORK       0x06
#define BBS_TYPE_DEV                    0x80
#define BBS_TYPE_UNKNOWN                0x_fF

#endif /* __EFI_DEVICE_PATH_H */
