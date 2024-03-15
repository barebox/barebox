// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <efi.h>
#include <efi/efi-util.h>
#include <malloc.h>
#include <string.h>
#include <wchar.h>
#include <linux/overflow.h>
#include <efi/device-path.h>

struct string {
	char *str;
	unsigned allocated;
	unsigned used;
};

char *cprintf(struct string *str, const char *fmt, ...)
    __attribute__ ((format(__printf__, 2, 3)));

char *cprintf(struct string *str, const char *fmt, ...)
{
	void *buf = str->str;
	unsigned bufsize = 0;
	va_list args;
	int len;

	if (str->str) {
		buf += str->used;
		if (check_sub_overflow(str->allocated, str->used, &bufsize))
			bufsize = 0;
	}

	va_start(args, fmt);
	len = vsnprintf(buf, bufsize, fmt, args);
	va_end(args);

	str->used += len;

	return NULL;
}

#define device_path_type(a)           ( ((a)->type) & DEVICE_PATH_TYPE_MASK )
#define next_device_path_node(a)       ( (const struct efi_device_path *) ( ((u8 *) (a)) + (a)->length))
#define is_device_path_end_type(a)      ( device_path_type(a) == DEVICE_PATH_TYPE_END )
#define is_device_path_end_sub_type(a)   ( (a)->sub_type == DEVICE_PATH_SUB_TYPE_END )
#define is_device_path_end(a)          ( is_device_path_end_type(a) && is_device_path_end_sub_type(a) )

#define set_device_path_end_node(a)  {                      \
            (a)->type = DEVICE_PATH_TYPE_END;           \
            (a)->sub_type = DEVICE_PATH_SUB_TYPE_END;     \
            (a)->length = sizeof(struct efi_device_path);   \
            }

const struct efi_device_path end_device_path = {
	.type = DEVICE_PATH_TYPE_END,
	.sub_type = DEVICE_PATH_SUB_TYPE_END,
	.length = DEVICE_PATH_END_LENGTH,
};

const struct efi_device_path end_instance_device_path = {
	.type = DEVICE_PATH_TYPE_END,
	.sub_type = DEVICE_PATH_SUB_TYPE_INSTANCE_END,
	.length = DEVICE_PATH_END_LENGTH,
};

const struct efi_device_path *
device_path_from_handle(efi_handle_t Handle)
{
	const efi_guid_t *const protocols[] = {
		&efi_loaded_image_device_path_guid,
		&efi_device_path_protocol_guid,
		NULL
	};
	const efi_guid_t * const*proto;
	efi_status_t Status;

	for (proto = protocols; *proto; proto++) {
		const struct efi_device_path *device_path;

		Status = BS->handle_protocol(Handle, *proto, (void *) &device_path);
		if (!EFI_ERROR(Status) && device_path)
			return device_path;
	}

	return NULL;
}

static void
dev_path_pci(struct string *str, const void *dev_path)
{
	const struct efi_device_path_pci *Pci;

	Pci = dev_path;
	cprintf(str, "Pci(0x%x,0x%x)", Pci->Device, Pci->Function);
}

static void
dev_path_pccard(struct string *str, const void *dev_path)
{
	const struct efi_device_path_pccard *Pccard;

	Pccard = dev_path;
	cprintf(str, "Pccard(0x%x)", Pccard->function_number);
}

static void
dev_path_mem_map(struct string *str, const void *dev_path)
{
	const struct efi_device_path_memory *mem_map;

	mem_map = dev_path;
	cprintf(str, "MemoryMapped(0x%x,0x%llx,0x%llx)",
		mem_map->memory_type,
		mem_map->starting_address, mem_map->ending_address);
}

static void
dev_path_controller(struct string *str, const void *dev_path)
{
	const struct efi_device_path_controller *Controller;

	Controller = dev_path;
	cprintf(str, "Ctrl(%d)", Controller->Controller);
}

static void
dev_path_vendor(struct string *str, const void *dev_path)
{
	const struct efi_device_path_vendor *Vendor;
	char *type;
	struct efi_device_path_unknown_device_vendor *unknown_dev_path;

	Vendor = dev_path;
	switch (device_path_type(&Vendor->header)) {
	case DEVICE_PATH_TYPE_HARDWARE_DEVICE:
		type = "Hw";
		break;
	case DEVICE_PATH_TYPE_MESSAGING_DEVICE:
		type = "Msg";
		break;
	case DEVICE_PATH_TYPE_MEDIA_DEVICE:
		type = "Media";
		break;
	default:
		type = "?";
		break;
	}

	cprintf(str, "Ven%s(%pUl", type, &Vendor->Guid);
	if (efi_guidcmp(Vendor->Guid, efi_unknown_device_guid) == 0) {
		/* GUID used by EFI to enumerate an EDD 1.1 device */
		unknown_dev_path =
		    (struct efi_device_path_unknown_device_vendor *) Vendor;
		cprintf(str, ":%02x)", unknown_dev_path->legacy_drive_letter);
	} else {
		cprintf(str, ")");
	}
}

/*
  type: 2 (ACPI Device Path) sub_type: 1 (ACPI Device Path)
 */
static void
dev_path_acpi(struct string *str, const void *dev_path)
{
	const struct efi_device_path_acpi_hid *Acpi;

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
dev_path_atapi(struct string *str, const void *dev_path)
{
	const struct efi_device_path_atapi *Atapi;

	Atapi = dev_path;
	cprintf(str, "Ata(%s,%s)",
		Atapi->primary_secondary ? "Secondary" : "Primary",
		Atapi->slave_master ? "Slave" : "Master");
}

static void
dev_path_scsi(struct string *str, const void *dev_path)
{
	const struct efi_device_path_scsi *Scsi;

	Scsi = dev_path;
	cprintf(str, "Scsi(%d,%d)", Scsi->Pun, Scsi->Lun);
}

static void
dev_path_fibre(struct string *str, const void *dev_path)
{
	const struct efi_device_path_fibrechannel *Fibre;

	Fibre = dev_path;
	cprintf(str, "Fibre%s(0x%016llx,0x%016llx)",
		device_path_type(&Fibre->header) ==
		DEVICE_PATH_SUB_TYPE_MSG_FIBRECHANNEL ? "" : "Ex", Fibre->WWN, Fibre->Lun);
}

static void
dev_path1394(struct string *str, const void *dev_path)
{
	const struct efi_device_path_f1394 *F1394;

	F1394 = dev_path;
	cprintf(str, "1394(%pUl)", &F1394->Guid);
}

static void
dev_path_usb(struct string *str, const void *dev_path)
{
	const struct efi_device_path_usb *Usb;

	Usb = dev_path;
	cprintf(str, "Usb(0x%x,0x%x)", Usb->Port, Usb->Endpoint);
}

static void
dev_path_i2_o(struct string *str, const void *dev_path)
{
	const struct efi_device_path_i2_o *i2_o;

	i2_o = dev_path;
	cprintf(str, "i2_o(0x%X)", i2_o->Tid);
}

static void
dev_path_mac_addr(struct string *str, const void *dev_path)
{
	const struct efi_device_path_mac_addr *MAC;
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
cat_print_iPv4(struct string *str, const struct efi_ipv4_address * address)
{
	cprintf(str, "%d.%d.%d.%d", address->Addr[0], address->Addr[1],
		address->Addr[2], address->Addr[3]);
}

static bool
is_not_null_iPv4(const struct efi_ipv4_address * address)
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
dev_path_iPv4(struct string *str, const void *dev_path)
{
	const struct efi_device_path_ipv4 *ip;
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
	    sizeof (struct efi_device_path_ipv4)) {
		/* only version 2 includes gateway and netmask */
		show |= is_not_null_iPv4(&ip->gateway_ip_address);
		show |= is_not_null_iPv4(&ip->subnet_mask);
	}
	if (show) {
		cprintf(str, ",");
		cat_print_iPv4(str, &ip->local_ip_address);
		if (ip->header.length ==
		    sizeof (struct efi_device_path_ipv4)) {
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
cat_print_ipv6(struct string *str, const struct efi_ipv6_address * address)
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
dev_path_iPv6(struct string *str, const void *dev_path)
{
	const struct efi_device_path_ipv6 *ip;

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
	    sizeof (struct efi_device_path_ipv6)) {
		cprintf(str, ",");
		cat_print_ipv6(str, &ip->gateway_ip_address);
		cprintf(str, ",");
		cprintf(str, "%d", ip->prefix_length);
	}
	cprintf(str, ")");
}

static void
dev_path_infini_band(struct string *str, const void *dev_path)
{
	const struct efi_device_path_infiniband *infini_band;

	infini_band = dev_path;
	cprintf(str, "Infiniband(0x%x,%pUl,0x%llx,0x%llx,0x%llx)",
		infini_band->resource_flags, &infini_band->port_gid,
		infini_band->service_id, infini_band->target_port_id,
		infini_band->device_id);
}

static void
dev_path_uart(struct string *str, const void *dev_path)
{
	const struct efi_device_path_uart *Uart;
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
dev_path_sata(struct string *str, const void *dev_path)
{
	const struct efi_device_path_sata *sata;

	sata = dev_path;
	cprintf(str, "Sata(0x%x,0x%x,0x%x)", sata->HBAPort_number,
		sata->port_multiplier_port_number, sata->Lun);
}

static void
dev_path_hard_drive(struct string *str, const void *dev_path)
{
	const struct efi_device_path_hard_drive_path *hd;

	hd = dev_path;
	switch (hd->signature_type) {
	case SIGNATURE_TYPE_MBR:
		cprintf(str, "HD(Part%d,Sig%08x)",
			hd->partition_number, *((u32 *) (&(hd->signature[0])))
		    );
		break;
	case SIGNATURE_TYPE_GUID:
		cprintf(str, "HD(Part%d,Sig%pUl)", hd->partition_number, (guid_t *)hd->signature);
		break;
	default:
		cprintf(str, "HD(Part%d,mbr_type=%02x,sig_type=%02x)",
			hd->partition_number, hd->mbr_type, hd->signature_type);
		break;
	}
}

static void
dev_path_cdrom(struct string *str, const void *dev_path)
{
	const struct efi_device_path_cdrom_path *cd;

	cd = dev_path;
	cprintf(str, "CDROM(0x%x)", cd->boot_entry);
}

static void
dev_path_file_path(struct string *str, const void *dev_path)
{
	const struct efi_device_path_file_path *Fp;
	char *dst;

	Fp = dev_path;

	dst = strdup_wchar_to_char(Fp->path_name);

	cprintf(str, "%s", dst);

	free(dst);
}

static void
dev_path_media_protocol(struct string *str, const void *dev_path)
{
	const struct efi_device_path_media_protocol *media_prot;

	media_prot = dev_path;
	cprintf(str, "%pUl", &media_prot->Protocol);
}

static void
dev_path_bbs_bbs(struct string *str, const void *dev_path)
{
	const struct efi_device_path_bbs_bbs *bbs;
	char *type;

	bbs = dev_path;
	switch (bbs->device_type) {
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
	case BBS_TYPE_DEV:
		type = "Dev";
		break;
	default:
		type = "?";
		break;
	}

	cprintf(str, "BBS(%s,%s)", type, bbs->String);
}

static void
dev_path_end_instance(struct string *str, const void *dev_path)
{
	cprintf(str, ",");
}

/**
 * Print unknown device node.
 * UEFI 2.4 § 9.6.1.6 table 89.
 */

static void
dev_path_node_unknown(struct string *str, const void *dev_path)
{
	const struct efi_device_path *Path;
	const u8 *value;
	int length, index;
	Path = dev_path;
	value = dev_path;
	value += 4;
	switch (Path->type) {
	case DEVICE_PATH_TYPE_HARDWARE_DEVICE:{
			/* Unknown Hardware Device Path */
			cprintf(str, "hardware_path(%d", Path->sub_type);
			break;
		}
	case DEVICE_PATH_TYPE_ACPI_DEVICE:{/* Unknown ACPI Device Path */
			cprintf(str, "acpi_path(%d", Path->sub_type);
			break;
		}
	case DEVICE_PATH_TYPE_MESSAGING_DEVICE:{
			/* Unknown Messaging Device Path */
			cprintf(str, "Msg(%d", Path->sub_type);
			break;
		}
	case DEVICE_PATH_TYPE_MEDIA_DEVICE:{
			/* Unknown Media Device Path */
			cprintf(str, "media_path(%d", Path->sub_type);
			break;
		}
	case DEVICE_PATH_TYPE_BBS_DEVICE:{	/* Unknown BIOS Boot Specification Device Path */
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
	void (*Function) (struct string *, const void *);
} dev_path_table[] = {
	{
	DEVICE_PATH_TYPE_HARDWARE_DEVICE, DEVICE_PATH_SUB_TYPE_PCI, dev_path_pci}, {
	DEVICE_PATH_TYPE_HARDWARE_DEVICE, DEVICE_PATH_SUB_TYPE_PCCARD, dev_path_pccard}, {
	DEVICE_PATH_TYPE_HARDWARE_DEVICE, DEVICE_PATH_SUB_TYPE_MEMORY, dev_path_mem_map}, {
	DEVICE_PATH_TYPE_HARDWARE_DEVICE, DEVICE_PATH_SUB_TYPE_VENDOR, dev_path_vendor}, {
	DEVICE_PATH_TYPE_HARDWARE_DEVICE, DEVICE_PATH_SUB_TYPE_CONTROLLER, dev_path_controller}, {
	DEVICE_PATH_TYPE_ACPI_DEVICE, DEVICE_PATH_SUB_TYPE_ACPI_DEVICE, dev_path_acpi}, {
	DEVICE_PATH_TYPE_MESSAGING_DEVICE, DEVICE_PATH_SUB_TYPE_MSG_ATAPI, dev_path_atapi}, {
	DEVICE_PATH_TYPE_MESSAGING_DEVICE, DEVICE_PATH_SUB_TYPE_MSG_SCSI, dev_path_scsi}, {
	DEVICE_PATH_TYPE_MESSAGING_DEVICE, DEVICE_PATH_SUB_TYPE_MSG_FIBRECHANNEL, dev_path_fibre}, {
	DEVICE_PATH_TYPE_MESSAGING_DEVICE, DEVICE_PATH_SUB_TYPE_MSG_1394, dev_path1394}, {
	DEVICE_PATH_TYPE_MESSAGING_DEVICE, DEVICE_PATH_SUB_TYPE_MSG_USB, dev_path_usb}, {
	DEVICE_PATH_TYPE_MESSAGING_DEVICE, DEVICE_PATH_SUB_TYPE_MSG_I2_o, dev_path_i2_o}, {
	DEVICE_PATH_TYPE_MESSAGING_DEVICE, DEVICE_PATH_SUB_TYPE_MSG_MAC_ADDR, dev_path_mac_addr}, {
	DEVICE_PATH_TYPE_MESSAGING_DEVICE, DEVICE_PATH_SUB_TYPE_MSG_IPv4, dev_path_iPv4}, {
	DEVICE_PATH_TYPE_MESSAGING_DEVICE, DEVICE_PATH_SUB_TYPE_MSG_IPv6, dev_path_iPv6}, {
	DEVICE_PATH_TYPE_MESSAGING_DEVICE, DEVICE_PATH_SUB_TYPE_MSG_INFINIBAND, dev_path_infini_band}, {
	DEVICE_PATH_TYPE_MESSAGING_DEVICE, DEVICE_PATH_SUB_TYPE_MSG_UART, dev_path_uart}, {
	DEVICE_PATH_TYPE_MESSAGING_DEVICE, DEVICE_PATH_SUB_TYPE_MSG_SATA, dev_path_sata}, {
	DEVICE_PATH_TYPE_MESSAGING_DEVICE, DEVICE_PATH_SUB_TYPE_MSG_VENDOR, dev_path_vendor}, {
	DEVICE_PATH_TYPE_MEDIA_DEVICE, DEVICE_PATH_SUB_TYPE_HARD_DRIVE_PATH, dev_path_hard_drive}, {
	DEVICE_PATH_TYPE_MEDIA_DEVICE, DEVICE_PATH_SUB_TYPE_CDROM_PATH, dev_path_cdrom}, {
	DEVICE_PATH_TYPE_MEDIA_DEVICE, DEVICE_PATH_SUB_TYPE_VENDOR_PATH, dev_path_vendor}, {
	DEVICE_PATH_TYPE_MEDIA_DEVICE, DEVICE_PATH_SUB_TYPE_FILE_PATH, dev_path_file_path}, {
	DEVICE_PATH_TYPE_MEDIA_DEVICE, DEVICE_PATH_SUB_TYPE_MEDIA_PROTOCOL, dev_path_media_protocol}, {
	DEVICE_PATH_TYPE_BBS_DEVICE, DEVICE_PATH_SUB_TYPE_BBS_BBS, dev_path_bbs_bbs}, {
	DEVICE_PATH_TYPE_END, DEVICE_PATH_SUB_TYPE_INSTANCE_END,
		    dev_path_end_instance}, {
	0, 0, NULL}
};

static void __device_path_to_str(struct string *str,
				 const struct efi_device_path *dev_path)
{
	const struct efi_device_path *dev_path_node;
	void (*dump_node) (struct string *, const void *);
	int i;

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

		if (str->used && dump_node != dev_path_end_instance)
			cprintf(str, "/");

		dump_node(str, dev_path_node);

		dev_path_node = next_device_path_node(dev_path_node);
	}
}

/**
 * device_path_to_str_buf() - formats a device path into a preallocated buffer
 *
 * @dev_path:	The EFI device path to format
 * @buf:	The buffer to format into or optionally NULL if @len is zero
 * @len:	The number of bytes that may be written into @buf
 * Return:	total number of bytes that are required to store the formatted
 *		result, excluding the terminating NUL byte, which is always
 *		written.
 */
size_t device_path_to_str_buf(const struct efi_device_path *dev_path,
			      char *buf, size_t len)
{
	struct string str = {
		.str = buf,
		.allocated = len,
	};

	__device_path_to_str(&str, dev_path);

	return str.used;
}

/**
 * device_path_to_str() - formats a device path into a newly allocated buffer
 *
 * @dev_path:	The EFI device path to format
 * Return:	A pointer to the nul-terminated formatted device path.
 */
char *device_path_to_str(const struct efi_device_path *dev_path)
{
	void *buf;
	size_t size;

	size = device_path_to_str_buf(dev_path, NULL, 0);

	buf = malloc(size + 1);
	if (!buf)
		return NULL;

	device_path_to_str_buf(dev_path, buf, size + 1);

	return buf;
}

u8 device_path_to_type(const struct efi_device_path *dev_path)
{
	const struct efi_device_path *dev_path_next;

	dev_path_next = next_device_path_node(dev_path);

	while (!is_device_path_end(dev_path_next)) {
		dev_path = dev_path_next;
		dev_path_next = next_device_path_node(dev_path);
	}

	return device_path_type(dev_path);
}

u8 device_path_to_subtype(const struct efi_device_path *dev_path)
{
	const struct efi_device_path *dev_path_next;

	dev_path_next = next_device_path_node(dev_path);

	while (!is_device_path_end(dev_path_next)) {
		dev_path = dev_path_next;
		dev_path_next = next_device_path_node(dev_path);
	}

	return dev_path->sub_type;
}

static const struct efi_device_path *
device_path_next_compatible_node(const struct efi_device_path *dev_path,
				 u8 type, u8 subtype)
{
	for (; !is_device_path_end(dev_path);
	     dev_path = next_device_path_node(dev_path)) {

		if (device_path_type(dev_path) != type)
			continue;

		if (dev_path->sub_type != subtype)
			continue;

		return dev_path;
	}

	return NULL;
}

char *device_path_to_partuuid(const struct efi_device_path *dev_path)
{
	while ((dev_path = device_path_next_compatible_node(dev_path,
				 DEVICE_PATH_TYPE_MEDIA_DEVICE, DEVICE_PATH_SUB_TYPE_HARD_DRIVE_PATH))) {
		struct efi_device_path_hard_drive_path *hd =
			(struct efi_device_path_hard_drive_path *)dev_path;

		if (hd->signature_type != SIGNATURE_TYPE_GUID)
			continue;

		return xasprintf("%pUl", (efi_guid_t *)&(hd->signature[0]));
	}

	return NULL;
}

char *device_path_to_filepath(const struct efi_device_path *dev_path)
{
	struct efi_device_path_file_path *fp = NULL;
	char *path;

	while ((dev_path = device_path_next_compatible_node(dev_path,
				 DEVICE_PATH_TYPE_MEDIA_DEVICE, DEVICE_PATH_SUB_TYPE_FILE_PATH))) {
		fp = container_of(dev_path, struct efi_device_path_file_path, header);
		dev_path = next_device_path_node(&fp->header);
	}

	path = strdup_wchar_to_char(fp->path_name);
	if (!path)
		return NULL;

	return strreplace(path, '\\', '/');
}
