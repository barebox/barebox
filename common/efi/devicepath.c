#include <common.h>
#include <efi.h>
#include <malloc.h>
#include <string.h>
#include <wchar.h>

struct string {
	char *str;
	int len;
};

char *cprintf(struct string *str, const char *fmt, ...)
    __attribute__ ((format(__printf__, 2, 3)));

char *cprintf(struct string *str, const char *fmt, ...)
{
	va_list args;
	int len;

	va_start(args, fmt);
	if (str->str)
		len = vsprintf(str->str + str->len, fmt, args);
	else
		len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	str->len += len;

	return NULL;
}

#define MIN_ALIGNMENT_SIZE  8	/* FIXME: X86_64 specific */
#define ALIGN_SIZE(a)   ((a % MIN_ALIGNMENT_SIZE) ? MIN_ALIGNMENT_SIZE - (a % MIN_ALIGNMENT_SIZE) : 0)

#define EFI_DP_TYPE_MASK			0x7f
#define EFI_DP_TYPE_UNPACKED			0x80

#define END_DEVICE_PATH_TYPE			0x7f

#define END_ENTIRE_DEVICE_PATH_SUBTYPE		0xff
#define END_INSTANCE_DEVICE_PATH_SUBTYPE	0x01
#define END_DEVICE_PATH_LENGTH			(sizeof(struct efi_device_path))

#define DP_IS_END_TYPE(a)
#define DP_IS_END_SUBTYPE(a)        ( ((a)->sub_type == END_ENTIRE_DEVICE_PATH_SUBTYPE )

#define device_path_type(a)           ( ((a)->type) & EFI_DP_TYPE_MASK )
#define next_device_path_node(a)       ( (struct efi_device_path *) ( ((u8 *) (a)) + (a)->length))
#define is_device_path_end_type(a)      ( device_path_type(a) == END_DEVICE_PATH_TYPE )
#define is_device_path_end_sub_type(a)   ( (a)->sub_type == END_ENTIRE_DEVICE_PATH_SUBTYPE )
#define is_device_path_end(a)          ( is_device_path_end_type(a) && is_device_path_end_sub_type(a) )
#define is_device_path_unpacked(a)     ( (a)->type & EFI_DP_TYPE_UNPACKED )

#define set_device_path_end_node(a)  {                      \
            (a)->type = END_DEVICE_PATH_TYPE;           \
            (a)->sub_type = END_ENTIRE_DEVICE_PATH_SUBTYPE;     \
            (a)->length = sizeof(struct efi_device_path);   \
            }

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
	efi_mac_address mac_address;
	u8 if_type;
};

#define MSG_IPv4_DP                     0x0c
struct ipv4_device_path {
	struct efi_device_path header;
	efi_ipv4_address local_ip_address;
	efi_ipv4_address remote_ip_address;
	u16 local_port;
	u16 remote_port;
	u16 Protocol;
	bool static_ip_address;
	/* new from UEFI version 2, code must check length field in header */
	efi_ipv4_address gateway_ip_address;
	efi_ipv4_address subnet_mask;
};

#define MSG_IPv6_DP                     0x0d
struct ipv6_device_path {
	struct efi_device_path header;
	efi_ipv6_address local_ip_address;
	efi_ipv6_address remote_ip_address;
	u16 local_port;
	u16 remote_port;
	u16 Protocol;
	bool IPAddress_origin;
	/* new from UEFI version 2, code must check length field in header */
	u8 prefix_length;
	efi_ipv6_address gateway_ip_address;
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

struct efi_device_path end_device_path = {
	.type = END_DEVICE_PATH_TYPE,
	.sub_type = END_ENTIRE_DEVICE_PATH_SUBTYPE,
	.length = END_DEVICE_PATH_LENGTH,
};

struct efi_device_path end_instance_device_path = {
	.type = END_DEVICE_PATH_TYPE,
	.sub_type = END_INSTANCE_DEVICE_PATH_SUBTYPE,
	.length = END_DEVICE_PATH_LENGTH,
};

struct efi_device_path *
device_path_from_handle(efi_handle_t Handle)
{
	efi_status_t Status;
	struct efi_device_path *device_path;

	Status = BS->handle_protocol(Handle, &efi_device_path_protocol_guid,
				(void *) &device_path);
	if (EFI_ERROR(Status))
		device_path = NULL;

	return device_path;
}

static struct efi_device_path *
unpack_device_path(struct efi_device_path *dev_path)
{
	struct efi_device_path *Src, *Dest, *new_path;
	unsigned long Size;

	/* Walk device path and round sizes to valid boundaries */

	Src = dev_path;
	Size = 0;
	for (;;) {
		Size += Src->length;
		Size += ALIGN_SIZE(Size);

		if (is_device_path_end(Src)) {
			break;
		}

		Src = next_device_path_node(Src);
	}

	new_path = xzalloc(Size);

	Src = dev_path;
	Dest = new_path;
	for (;;) {
		Size = Src->length;
		memcpy(Dest, Src, Size);
		Size += ALIGN_SIZE(Size);
		Dest->length = Size;
		Dest->type |= EFI_DP_TYPE_UNPACKED;
		Dest =
		    (struct efi_device_path *) (((u8 *) Dest) + Size);

		if (is_device_path_end(Src))
			break;

		Src = next_device_path_node(Src);
	}

	return new_path;
}

static void
dev_path_pci(struct string *str, void *dev_path)
{
	struct pci_device_path *Pci;

	Pci = dev_path;
	cprintf(str, "Pci(0x%x,0x%x)", Pci->Device, Pci->Function);
}

static void
dev_path_pccard(struct string *str, void *dev_path)
{
	struct pccard_device_path *Pccard;

	Pccard = dev_path;
	cprintf(str, "Pccard(0x%x)", Pccard->function_number);
}

static void
dev_path_mem_map(struct string *str, void *dev_path)
{
	struct memmap_device_path *mem_map;

	mem_map = dev_path;
	cprintf(str, "mem_map(%d,0x%llx,0x%llx)",
		mem_map->memory_type,
		mem_map->starting_address, mem_map->ending_address);
}

static void
dev_path_controller(struct string *str, void *dev_path)
{
	struct controller_device_path *Controller;

	Controller = dev_path;
	cprintf(str, "Ctrl(%d)", Controller->Controller);
}

static void
dev_path_vendor(struct string *str, void *dev_path)
{
	struct vendor_device_path *Vendor;
	char *type;
	struct unknown_device_vendor_device_path *unknown_dev_path;

	Vendor = dev_path;
	switch (device_path_type(&Vendor->header)) {
	case HARDWARE_DEVICE_PATH:
		type = "Hw";
		break;
	case MESSAGING_DEVICE_PATH:
		type = "Msg";
		break;
	case MEDIA_DEVICE_PATH:
		type = "Media";
		break;
	default:
		type = "?";
		break;
	}

	cprintf(str, "Ven%s(%pU", type, &Vendor->Guid);
	if (efi_guidcmp(Vendor->Guid, efi_unknown_device_guid) == 0) {
		/* GUID used by EFI to enumerate an EDD 1.1 device */
		unknown_dev_path =
		    (struct unknown_device_vendor_device_path *) Vendor;
		cprintf(str, ":%02x)", unknown_dev_path->legacy_drive_letter);
	} else {
		cprintf(str, ")");
	}
}

/*
  type: 2 (ACPI Device Path) sub_type: 1 (ACPI Device Path)
 */
static void
dev_path_acpi(struct string *str, void *dev_path)
{
	struct acpi_hid_device_path *Acpi;

	Acpi = dev_path;
	if ((Acpi->HID & PNP_EISA_ID_MASK) == PNP_EISA_ID_CONST) {
		switch (EISA_ID_TO_NUM(Acpi->HID)) {
		case 0x301:
			cprintf(str, "Keyboard(%d)", Acpi->UID);
			break;

		case 0x401:
			cprintf(str, "parallel_port(%d)", Acpi->UID);
			break;
		case 0x501:
			cprintf(str, "Serial(%d)", Acpi->UID);
			break;
		case 0x604:
			cprintf(str, "Floppy(%d)", Acpi->UID);
			break;
		case 0xa03:
			cprintf(str, "pci_root(%d)", Acpi->UID);
			break;
		case 0xa08:
			cprintf(str, "pcie_root(%d)", Acpi->UID);
			break;
		default:
			cprintf(str, "Acpi(PNP%04x",
				EISA_ID_TO_NUM(Acpi->HID));
			if (Acpi->UID)
				cprintf(str, ",%d", Acpi->UID);
			cprintf(str, ")");
			break;
		}
	} else {
		cprintf(str, "Acpi(0x%X", Acpi->HID);
		if (Acpi->UID)
			cprintf(str, ",%d", Acpi->UID);
		cprintf(str, ")");
	}
}

static void
dev_path_atapi(struct string *str, void *dev_path)
{
	struct atapi_device_path *Atapi;

	Atapi = dev_path;
	cprintf(str, "Ata(%s,%s)",
		Atapi->primary_secondary ? "Secondary" : "Primary",
		Atapi->slave_master ? "Slave" : "Master");
}

static void
dev_path_scsi(struct string *str, void *dev_path)
{
	struct scsi_device_path *Scsi;

	Scsi = dev_path;
	cprintf(str, "Scsi(%d,%d)", Scsi->Pun, Scsi->Lun);
}

static void
dev_path_fibre(struct string *str, void *dev_path)
{
	struct fibrechannel_device_path *Fibre;

	Fibre = dev_path;
	cprintf(str, "Fibre%s(0x%016llx,0x%016llx)",
		device_path_type(&Fibre->header) ==
		MSG_FIBRECHANNEL_DP ? "" : "Ex", Fibre->WWN, Fibre->Lun);
}

static void
dev_path1394(struct string *str, void *dev_path)
{
	struct f1394_device_path *F1394;

	F1394 = dev_path;
	cprintf(str, "1394(%pU)", &F1394->Guid);
}

static void
dev_path_usb(struct string *str, void *dev_path)
{
	struct usb_device_path *Usb;

	Usb = dev_path;
	cprintf(str, "Usb(0x%x,0x%x)", Usb->Port, Usb->Endpoint);
}

static void
dev_path_i2_o(struct string *str, void *dev_path)
{
	struct i2_o_device_path *i2_o;

	i2_o = dev_path;
	cprintf(str, "i2_o(0x%X)", i2_o->Tid);
}

static void
dev_path_mac_addr(struct string *str, void *dev_path)
{
	struct mac_addr_device_path *MAC;
	unsigned long hw_address_size;
	unsigned long Index;

	MAC = dev_path;

	/* hw_address_size = sizeof(EFI_MAC_ADDRESS); */
	hw_address_size = MAC->header.length;
	hw_address_size -= sizeof (MAC->header);
	hw_address_size -= sizeof (MAC->if_type);
	if (MAC->if_type == 0x01 || MAC->if_type == 0x00)
		hw_address_size = 6;

	cprintf(str, "Mac(");

	for (Index = 0; Index < hw_address_size; Index++)
		cprintf(str, "%02x", MAC->mac_address.Addr[Index]);

	if (MAC->if_type != 0)
		cprintf(str, ",%d", MAC->if_type);

	cprintf(str, ")");
}

static void
cat_print_iPv4(struct string *str, efi_ipv4_address * address)
{
	cprintf(str, "%d.%d.%d.%d", address->Addr[0], address->Addr[1],
		address->Addr[2], address->Addr[3]);
}

static bool
is_not_null_iPv4(efi_ipv4_address * address)
{
	u8 val;

	val = address->Addr[0] | address->Addr[1];
	val |= address->Addr[2] | address->Addr[3];

	return val != 0;
}

static void
cat_print_network_protocol(struct string *str, u16 Proto)
{
	if (Proto == 6)
		cprintf(str, "TCP");
	else if (Proto == 17)
		cprintf(str, "UDP");
	else
		cprintf(str, "%d", Proto);
}

static void
dev_path_iPv4(struct string *str, void *dev_path)
{
	struct ipv4_device_path *ip;
	bool show;

	ip = dev_path;
	cprintf(str, "IPv4(");
	cat_print_iPv4(str, &ip->remote_ip_address);
	cprintf(str, ",");
	cat_print_network_protocol(str, ip->Protocol);
	cprintf(str, ",%s", ip->static_ip_address ? "Static" : "DHCP");
	show = is_not_null_iPv4(&ip->local_ip_address);
	if (!show
	    && ip->header.length ==
	    sizeof (struct ipv4_device_path)) {
		/* only version 2 includes gateway and netmask */
		show |= is_not_null_iPv4(&ip->gateway_ip_address);
		show |= is_not_null_iPv4(&ip->subnet_mask);
	}
	if (show) {
		cprintf(str, ",");
		cat_print_iPv4(str, &ip->local_ip_address);
		if (ip->header.length ==
		    sizeof (struct ipv4_device_path)) {
			/* only version 2 includes gateway and netmask */
			show = is_not_null_iPv4(&ip->gateway_ip_address);
			show |= is_not_null_iPv4(&ip->subnet_mask);
			if (show) {
				cprintf(str, ",");
				cat_print_iPv4(str, &ip->gateway_ip_address);
				if (is_not_null_iPv4(&ip->subnet_mask)) {
					cprintf(str, ",");
					cat_print_iPv4(str, &ip->subnet_mask);
				}
			}
		}
	}
	cprintf(str, ")");
}

#define cat_print_iPv6_ADD( x , y ) ( ( (u16) ( x ) ) << 8 | ( y ) )
static void
cat_print_ipv6(struct string *str, efi_ipv6_address * address)
{
	cprintf(str, "%x:%x:%x:%x:%x:%x:%x:%x",
		cat_print_iPv6_ADD(address->Addr[0], address->Addr[1]),
		cat_print_iPv6_ADD(address->Addr[2], address->Addr[3]),
		cat_print_iPv6_ADD(address->Addr[4], address->Addr[5]),
		cat_print_iPv6_ADD(address->Addr[6], address->Addr[7]),
		cat_print_iPv6_ADD(address->Addr[8], address->Addr[9]),
		cat_print_iPv6_ADD(address->Addr[10], address->Addr[11]),
		cat_print_iPv6_ADD(address->Addr[12], address->Addr[13]),
		cat_print_iPv6_ADD(address->Addr[14], address->Addr[15]));
}

static void
dev_path_iPv6(struct string *str, void *dev_path)
{
	struct ipv6_device_path *ip;

	ip = dev_path;
	cprintf(str, "IPv6(");
	cat_print_ipv6(str, &ip->remote_ip_address);
	cprintf(str, ",");
	cat_print_network_protocol(str, ip->Protocol);
	cprintf(str, ",%s,", ip->IPAddress_origin ?
		(ip->IPAddress_origin == 1 ? "stateless_auto_configure" :
		 "stateful_auto_configure") : "Static");
	cat_print_ipv6(str, &ip->local_ip_address);
	if (ip->header.length ==
	    sizeof (struct ipv6_device_path)) {
		cprintf(str, ",");
		cat_print_ipv6(str, &ip->gateway_ip_address);
		cprintf(str, ",");
		cprintf(str, "%d", ip->prefix_length);
	}
	cprintf(str, ")");
}

static void
dev_path_infini_band(struct string *str, void *dev_path)
{
	struct infiniband_device_path *infini_band;

	infini_band = dev_path;
	cprintf(str, "Infiniband(0x%x,%pU,0x%llx,0x%llx,0x%llx)",
		infini_band->resource_flags, &infini_band->port_gid,
		infini_band->service_id, infini_band->target_port_id,
		infini_band->device_id);
}

static void
dev_path_uart(struct string *str, void *dev_path)
{
	struct uart_device_path *Uart;
	s8 Parity;

	Uart = dev_path;
	switch (Uart->Parity) {
	case 0:
		Parity = 'D';
		break;
	case 1:
		Parity = 'N';
		break;
	case 2:
		Parity = 'E';
		break;
	case 3:
		Parity = 'O';
		break;
	case 4:
		Parity = 'M';
		break;
	case 5:
		Parity = 'S';
		break;
	default:
		Parity = 'x';
		break;
	}

	if (Uart->baud_rate == 0)
		cprintf(str, "Uart(DEFAULT %c", Parity);
	else
		cprintf(str, "Uart(%lld %c", Uart->baud_rate, Parity);

	if (Uart->data_bits == 0)
		cprintf(str, "D");
	else
		cprintf(str, "%d", Uart->data_bits);

	switch (Uart->stop_bits) {
	case 0:
		cprintf(str, "D)");
		break;
	case 1:
		cprintf(str, "1)");
		break;
	case 2:
		cprintf(str, "1.5)");
		break;
	case 3:
		cprintf(str, "2)");
		break;
	default:
		cprintf(str, "x)");
		break;
	}
}

static void
dev_path_sata(struct string *str, void *dev_path)
{
	struct sata_device_path *sata;

	sata = dev_path;
	cprintf(str, "Sata(0x%x,0x%x,0x%x)", sata->HBAPort_number,
		sata->port_multiplier_port_number, sata->Lun);
}

static void
dev_path_hard_drive(struct string *str, void *dev_path)
{
	struct harddrive_device_path *hd;

	hd = dev_path;
	switch (hd->signature_type) {
	case SIGNATURE_TYPE_MBR:
		cprintf(str, "HD(Part%d,Sig%08x)",
			hd->partition_number, *((u32 *) (&(hd->signature[0])))
		    );
		break;
	case SIGNATURE_TYPE_GUID:
		cprintf(str, "HD(Part%d,Sig%pU)",
			hd->partition_number,
			(efi_guid_t *) & (hd->signature[0])
		    );
		break;
	default:
		cprintf(str, "HD(Part%d,mbr_type=%02x,sig_type=%02x)",
			hd->partition_number, hd->mbr_type, hd->signature_type);
		break;
	}
}

static void
dev_path_cdrom(struct string *str, void *dev_path)
{
	struct cdrom_device_path *cd;

	cd = dev_path;
	cprintf(str, "CDROM(0x%x)", cd->boot_entry);
}

static void
dev_path_file_path(struct string *str, void *dev_path)
{
	struct filepath_device_path *Fp;
	char *dst;

	Fp = dev_path;

	dst = strdup_wchar_to_char(Fp->path_name);

	cprintf(str, "%s", dst);

	free(dst);
}

static void
dev_path_media_protocol(struct string *str, void *dev_path)
{
	struct media_protocol_device_path *media_prot;

	media_prot = dev_path;
	cprintf(str, "%pU", &media_prot->Protocol);
}

static void
dev_path_bss_bss(struct string *str, void *dev_path)
{
	struct bbs_bbs_device_path *Bss;
	char *type;

	Bss = dev_path;
	switch (Bss->device_type) {
	case BBS_TYPE_FLOPPY:
		type = "Floppy";
		break;
	case BBS_TYPE_HARDDRIVE:
		type = "Harddrive";
		break;
	case BBS_TYPE_CDROM:
		type = "CDROM";
		break;
	case BBS_TYPE_PCMCIA:
		type = "PCMCIA";
		break;
	case BBS_TYPE_USB:
		type = "Usb";
		break;
	case BBS_TYPE_EMBEDDED_NETWORK:
		type = "Net";
		break;
	default:
		type = "?";
		break;
	}

	cprintf(str, "Bss-%s(%s)", type, Bss->String);
}

static void
dev_path_end_instance(struct string *str, void *dev_path)
{
	cprintf(str, ",");
}

/**
 * Print unknown device node.
 * UEFI 2.4 § 9.6.1.6 table 89.
 */

static void
dev_path_node_unknown(struct string *str, void *dev_path)
{
	struct efi_device_path *Path;
	u8 *value;
	int length, index;
	Path = dev_path;
	value = dev_path;
	value += 4;
	switch (Path->type) {
	case HARDWARE_DEVICE_PATH:{
			/* Unknown Hardware Device Path */
			cprintf(str, "hardware_path(%d", Path->sub_type);
			break;
		}
	case ACPI_DEVICE_PATH:{/* Unknown ACPI Device Path */
			cprintf(str, "acpi_path(%d", Path->sub_type);
			break;
		}
	case MESSAGING_DEVICE_PATH:{
			/* Unknown Messaging Device Path */
			cprintf(str, "Msg(%d", Path->sub_type);
			break;
		}
	case MEDIA_DEVICE_PATH:{
			/* Unknown Media Device Path */
			cprintf(str, "media_path(%d", Path->sub_type);
			break;
		}
	case BBS_DEVICE_PATH:{	/* Unknown BIOS Boot Specification Device Path */
			cprintf(str, "bbs_path(%d", Path->sub_type);
			break;
		}
	default:{		/* Unknown Device Path */
			cprintf(str, "Path(%d,%d", Path->type, Path->sub_type);
			break;
		}
	}
	length = Path->length;
	for (index = 0; index < length; index++) {
		if (index == 0)
			cprintf(str, ",0x");
		cprintf(str, "%02x", *value);
		value++;
	}
	cprintf(str, ")");
}

/*
 * Table to convert "type" and "sub_type" to a "convert to text" function/
 * Entries hold "type" and "sub_type" for know values.
 * Special "sub_type" 0 is used as default for known type with unknown subtype.
 */
struct {
	u8 type;
	u8 sub_type;
	void (*Function) (struct string *, void *);
} dev_path_table[] = {
	{
	HARDWARE_DEVICE_PATH, HW_PCI_DP, dev_path_pci}, {
	HARDWARE_DEVICE_PATH, HW_PCCARD_DP, dev_path_pccard}, {
	HARDWARE_DEVICE_PATH, HW_MEMMAP_DP, dev_path_mem_map}, {
	HARDWARE_DEVICE_PATH, HW_VENDOR_DP, dev_path_vendor}, {
	HARDWARE_DEVICE_PATH, HW_CONTROLLER_DP, dev_path_controller}, {
	ACPI_DEVICE_PATH, ACPI_DP, dev_path_acpi}, {
	MESSAGING_DEVICE_PATH, MSG_ATAPI_DP, dev_path_atapi}, {
	MESSAGING_DEVICE_PATH, MSG_SCSI_DP, dev_path_scsi}, {
	MESSAGING_DEVICE_PATH, MSG_FIBRECHANNEL_DP, dev_path_fibre}, {
	MESSAGING_DEVICE_PATH, MSG_1394_DP, dev_path1394}, {
	MESSAGING_DEVICE_PATH, MSG_USB_DP, dev_path_usb}, {
	MESSAGING_DEVICE_PATH, MSG_I2_o_DP, dev_path_i2_o}, {
	MESSAGING_DEVICE_PATH, MSG_MAC_ADDR_DP, dev_path_mac_addr}, {
	MESSAGING_DEVICE_PATH, MSG_IPv4_DP, dev_path_iPv4}, {
	MESSAGING_DEVICE_PATH, MSG_IPv6_DP, dev_path_iPv6}, {
	MESSAGING_DEVICE_PATH, MSG_INFINIBAND_DP, dev_path_infini_band}, {
	MESSAGING_DEVICE_PATH, MSG_UART_DP, dev_path_uart}, {
	MESSAGING_DEVICE_PATH, MSG_SATA_DP, dev_path_sata}, {
	MESSAGING_DEVICE_PATH, MSG_VENDOR_DP, dev_path_vendor}, {
	MEDIA_DEVICE_PATH, MEDIA_HARDDRIVE_DP, dev_path_hard_drive}, {
	MEDIA_DEVICE_PATH, MEDIA_CDROM_DP, dev_path_cdrom}, {
	MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP, dev_path_vendor}, {
	MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP, dev_path_file_path}, {
	MEDIA_DEVICE_PATH, MEDIA_PROTOCOL_DP, dev_path_media_protocol}, {
	BBS_DEVICE_PATH, BBS_BBS_DP, dev_path_bss_bss}, {
	END_DEVICE_PATH_TYPE, END_INSTANCE_DEVICE_PATH_SUBTYPE,
		    dev_path_end_instance}, {
	0, 0, NULL}
};

static int __device_path_to_str(struct string *str, struct efi_device_path *dev_path)
{
	struct efi_device_path *dev_path_node;
	void (*dump_node) (struct string *, void *);
	int i;

	dev_path = unpack_device_path(dev_path);

	dev_path_node = dev_path;
	while (!is_device_path_end(dev_path_node)) {
		dump_node = NULL;
		for (i = 0; dev_path_table[i].Function; i += 1) {

			if (device_path_type(dev_path_node) ==
			    dev_path_table[i].type
			    && dev_path_node->sub_type ==
			    dev_path_table[i].sub_type) {
				dump_node = dev_path_table[i].Function;
				break;
			}
		}

		if (!dump_node)
			dump_node = dev_path_node_unknown;

		if (str->len && dump_node != dev_path_end_instance)
			cprintf(str, "/");

		dump_node(str, dev_path_node);

		dev_path_node = next_device_path_node(dev_path_node);
	}

	return 0;
}

char *device_path_to_str(struct efi_device_path *dev_path)
{
	struct string str = {};

	__device_path_to_str(&str, dev_path);

	str.str = malloc(str.len + 1);
	if (!str.str)
		return NULL;

	str.len = 0;

	__device_path_to_str(&str, dev_path);

	return str.str;
}

u8 device_path_to_type(struct efi_device_path *dev_path)
{
	struct efi_device_path *dev_path_next;

	dev_path = unpack_device_path(dev_path);
	dev_path_next = next_device_path_node(dev_path);

	while (!is_device_path_end(dev_path_next)) {
		dev_path = dev_path_next;
		dev_path_next = next_device_path_node(dev_path);
	}

	return device_path_type(dev_path);
}

char *device_path_to_partuuid(struct efi_device_path *dev_path)
{
	struct efi_device_path *dev_path_node;
	struct harddrive_device_path *hd;
	char *str = NULL;;

	dev_path = unpack_device_path(dev_path);

	for (dev_path_node = dev_path; !is_device_path_end(dev_path_node);
	     dev_path_node = next_device_path_node(dev_path_node)) {

		if (device_path_type(dev_path_node) != MEDIA_DEVICE_PATH)
			continue;

		if (dev_path_node->sub_type != MEDIA_HARDDRIVE_DP)
			continue;

		hd = (struct harddrive_device_path *)dev_path_node;

		if (hd->signature_type != SIGNATURE_TYPE_GUID)
			continue;

		str = xasprintf("%pUl", (efi_guid_t *)&(hd->signature[0]));
		break;
	}

	return str;
}

