#ifndef __EFI_DEVICE_PATH_H
#define __EFI_DEVICE_PATH_H

#include <efi/types.h>

/*
 * Hardware Device Path (UEFI 2.4 specification, version 2.4 § 9.3.2.)
 */

#define DEVICE_PATH_TYPE_HARDWARE_DEVICE            0x01

#define DEVICE_PATH_SUB_TYPE_PCI                       0x01
struct efi_device_path_pci {
	struct efi_device_path header;
	u8 Function;
	u8 Device;
};

#define DEVICE_PATH_SUB_TYPE_PCCARD                    0x02
struct efi_device_path_pccard {
	struct efi_device_path header;
	u8 function_number;
};

#define DEVICE_PATH_SUB_TYPE_MEMORY                    0x03
struct efi_device_path_memory {
	struct efi_device_path header;
	u32 memory_type;
	efi_physical_addr_t starting_address;
	efi_physical_addr_t ending_address;
};

#define DEVICE_PATH_SUB_TYPE_VENDOR                    0x04
struct efi_device_path_vendor {
	struct efi_device_path header;
	efi_guid_t Guid;
};

struct efi_device_path_unknown_device_vendor {
	struct efi_device_path_vendor device_path;
	u8 legacy_drive_letter;
};

#define DEVICE_PATH_SUB_TYPE_CONTROLLER            0x05
struct efi_device_path_controller {
	struct efi_device_path header;
	u32 Controller;
};

/*
 * ACPI Device Path (UEFI 2.4 specification, version 2.4 § 9.3.3 and 9.3.4.)
 */
#define DEVICE_PATH_TYPE_ACPI_DEVICE                 0x02

#define DEVICE_PATH_SUB_TYPE_ACPI_DEVICE                         0x01
struct efi_device_path_acpi_hid {
	struct efi_device_path header;
	u32 HID;
	u32 UID;
};

#define DEVICE_PATH_SUB_TYPE_EXPANDED_ACPI_DEVICE		0x02
struct efi_device_path_expanded_acpi {
	struct efi_device_path header;
	u32 HID;
	u32 UID;
	u32 CID;
	u8 hid_str[];
};

#define DEVICE_PATH_SUB_TYPE_ACPI_ADR_DEVICE 3
struct efi_device_path_acpi_adr {
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
#define DEVICE_PATH_TYPE_MESSAGING_DEVICE           0x03

#define DEVICE_PATH_SUB_TYPE_MSG_ATAPI                    0x01
struct efi_device_path_atapi {
	struct efi_device_path header;
	u8 primary_secondary;
	u8 slave_master;
	u16 Lun;
};

#define DEVICE_PATH_SUB_TYPE_MSG_SCSI                     0x02
struct efi_device_path_scsi {
	struct efi_device_path header;
	u16 Pun;
	u16 Lun;
};

#define DEVICE_PATH_SUB_TYPE_MSG_FIBRECHANNEL             0x03
struct efi_device_path_fibrechannel {
	struct efi_device_path header;
	u32 Reserved;
	u64 WWN;
	u64 Lun;
};

/**
 * Fibre Channel Ex sub_type.
 * UEFI 2.0 specification version 2.4 § 9.3.5.6.
 */
#define DEVICE_PATH_SUB_TYPE_MSG_FIBRECHANNEL_EX 21
struct efi_device_path_fibrechannelex {
	struct efi_device_path header;
	u32 Reserved;
	u8 WWN[8];		/* World Wide Name */
	u8 Lun[8];		/* Logical unit, T-10 SCSI Architecture Model 4 specification */
};

#define DEVICE_PATH_SUB_TYPE_MSG_1394                     0x04
struct efi_device_path_f1394 {
	struct efi_device_path header;
	u32 Reserved;
	u64 Guid;
};

#define DEVICE_PATH_SUB_TYPE_MSG_USB                      0x05
struct efi_device_path_usb {
	struct efi_device_path header;
	u8 Port;
	u8 Endpoint;
};

/**
 * SATA Device Path sub_type.
 * UEFI 2.0 specification version 2.4 § 9.3.5.6.
 */
#define DEVICE_PATH_SUB_TYPE_MSG_SATA 18
struct efi_device_path_sata {
	struct efi_device_path header;
	u16 HBAPort_number;
	u16 port_multiplier_port_number;
	u16 Lun;		/* Logical Unit Number */
};

/**
 * USB WWID Device Path sub_type.
 * UEFI 2.0 specification version 2.4 § 9.3.5.7.
 */
#define DEVICE_PATH_SUB_TYPE_MSG_USB_WWID 16
struct efi_device_path_usb_wwid {
	struct efi_device_path header;
	u16 interface_number;
	u16 vendor_id;
	u16 product_id;
	s16 serial_number[];	/* UTF-16 characters of the USB serial number */
};

/**
 * Device Logical Unit sub_type.
 * UEFI 2.0 specification version 2.4 § 9.3.5.8.
 */
#define DEVICE_PATH_SUB_TYPE_MSG_DEVICE_LOGICAL_UNIT 17
struct efi_device_path_logical_unit {
	struct efi_device_path header;
	u8 Lun;			/* Logical Unit Number */
};

#define DEVICE_PATH_SUB_TYPE_MSG_USB_CLASS                0x0f
struct efi_device_path_usb_class {
	struct efi_device_path header;
	u16 vendor_id;
	u16 product_id;
	u8 device_class;
	u8 device_subclass;
	u8 device_protocol;
};

#define DEVICE_PATH_SUB_TYPE_MSG_I2_o                      0x06
struct efi_device_path_i2_o {
	struct efi_device_path header;
	u32 Tid;
};

#define DEVICE_PATH_SUB_TYPE_MSG_MAC_ADDR                 0x0b
struct efi_device_path_mac_addr {
	struct efi_device_path header;
	struct efi_mac_address mac_address;
	u8 if_type;
};

#define DEVICE_PATH_SUB_TYPE_MSG_IPv4                     0x0c
struct efi_device_path_ipv4 {
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

#define DEVICE_PATH_SUB_TYPE_MSG_IPv6                     0x0d
struct efi_device_path_ipv6 {
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
#define DEVICE_PATH_SUB_TYPE_MSG_VLAN 20
struct efi_device_path_vlan {
	struct efi_device_path header;
	u16 vlan_id;
};

#define DEVICE_PATH_SUB_TYPE_MSG_INFINIBAND               0x09
struct efi_device_path_infiniband {
	struct efi_device_path header;
	u32 resource_flags;
	efi_guid_t port_gid;
	u64 service_id;
	u64 target_port_id;
	u64 device_id;
};

#define DEVICE_PATH_SUB_TYPE_MSG_UART                     0x0e
struct efi_device_path_uart {
	struct efi_device_path header;
	u32 Reserved;
	u64 baud_rate;
	u8 data_bits;
	u8 Parity;
	u8 stop_bits;
};

#define DEVICE_PATH_SUB_TYPE_MSG_VENDOR                   0x0a
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
#define DEVICE_PATH_TYPE_MEDIA_DEVICE               0x04

#define DEVICE_PATH_SUB_TYPE_HARD_DRIVE_PATH              0x01
struct efi_device_path_hard_drive_path {
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

#define DEVICE_PATH_SUB_TYPE_CDROM_PATH                  0x02
struct efi_device_path_cdrom_path {
	struct efi_device_path header;
	u32 boot_entry;
	u64 partition_start;
	u64 partition_size;
};

#define DEVICE_PATH_SUB_TYPE_VENDOR_PATH                 0x03
/* Use VENDOR_DEVICE_PATH struct */

#define DEVICE_PATH_SUB_TYPE_FILE_PATH               0x04
struct efi_device_path_file_path {
	struct efi_device_path header;
	s16 path_name[];
};

#define SIZE_OF_FILEPATH_DEVICE_PATH offsetof(FILEPATH_DEVICE_PATH,path_name)

#define DEVICE_PATH_SUB_TYPE_MEDIA_PROTOCOL               0x05
struct efi_device_path_media_protocol {
	struct efi_device_path header;
	efi_guid_t Protocol;
};

/**
 * PIWG Firmware File sub_type.
 * UEFI 2.0 specification version 2.4 § 9.3.6.6.
 */
#define DEVICE_PATH_SUB_TYPE_PIWG_FW_FILE 6
struct efi_device_path_media_fw_vol_file_path {
	struct efi_device_path header;
	efi_guid_t fv_file_name;
};

/**
 * PIWG Firmware Volume Device Path sub_type.
 * UEFI 2.0 specification version 2.4 § 9.3.6.7.
 */
#define DEVICE_PATH_SUB_TYPE_PIWG_FW_VOL 7
struct efi_device_media_piwg_fw_vol {
	struct efi_device_path header;
	efi_guid_t fv_name;
};

/**
 * Media relative offset range device path.
 * UEFI 2.0 specification version 2.4 § 9.3.6.8.
 */
#define DEVICE_PATH_SUB_TYPE_MEDIA_RELATIVE_OFFSET_RANGE 8
struct efi_device_media_relative_offset_range {
	struct efi_device_path header;
	u32 Reserved;
	u64 starting_offset;
	u64 ending_offset;
};

/*
 * BIOS Boot Specification Device Path (UEFI 2.4 specification, version 2.4 § 9.3.7.)
 */
#define DEVICE_PATH_TYPE_BBS_DEVICE                 0x05

#define DEVICE_PATH_SUB_TYPE_BBS_BBS                      0x01
struct efi_device_path_bbs_bbs {
	struct efi_device_path header;
	u16 device_type;
	u16 status_flag;
	s8 String[];
};

/* device_type definitions - from BBS specification */
#define BBS_TYPE_FLOPPY                 0x01
#define BBS_TYPE_HARDDRIVE              0x02
#define BBS_TYPE_CDROM                  0x03
#define BBS_TYPE_PCMCIA                 0x04
#define BBS_TYPE_USB                    0x05
#define BBS_TYPE_EMBEDDED_NETWORK       0x06
#define BBS_TYPE_DEV                    0x80
#define BBS_TYPE_UNKNOWN                0xff


#define DEVICE_PATH_TYPE_MASK			0x7f
#define DEVICE_PATH_TYPE_UNPACKED		0x80

#define DEVICE_PATH_TYPE_END			0x7f

#define DEVICE_PATH_SUB_TYPE_END		0xff
#define DEVICE_PATH_SUB_TYPE_INSTANCE_END	0x01
#define DEVICE_PATH_END_LENGTH			(sizeof(struct efi_device_path))

#endif /* __EFI_DEVICE_PATH_H */
