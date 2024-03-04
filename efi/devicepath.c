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
#define next_device_path_node(a)       ( (const struct efi_device_path *) ( ((u8 *) (a)) + (a)->length))
#define is_device_path_end_type(a)      ( device_path_type(a) == END_DEVICE_PATH_TYPE )
#define is_device_path_end_sub_type(a)   ( (a)->sub_type == END_ENTIRE_DEVICE_PATH_SUBTYPE )
#define is_device_path_end(a)          ( is_device_path_end_type(a) && is_device_path_end_sub_type(a) )
#define is_device_path_unpacked(a)     ( (a)->type & EFI_DP_TYPE_UNPACKED )

#define set_device_path_end_node(a)  {                      \
            (a)->type = END_DEVICE_PATH_TYPE;           \
            (a)->sub_type = END_ENTIRE_DEVICE_PATH_SUBTYPE;     \
            (a)->length = sizeof(struct efi_device_path);   \
            }

const struct efi_device_path end_device_path = {
	.type = END_DEVICE_PATH_TYPE,
	.sub_type = END_ENTIRE_DEVICE_PATH_SUBTYPE,
	.length = END_DEVICE_PATH_LENGTH,
};

const struct efi_device_path end_instance_device_path = {
	.type = END_DEVICE_PATH_TYPE,
	.sub_type = END_INSTANCE_DEVICE_PATH_SUBTYPE,
	.length = END_DEVICE_PATH_LENGTH,
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

static struct efi_device_path *
unpack_device_path(const struct efi_device_path *dev_path)
{
	const struct efi_device_path *Src;
	struct efi_device_path *Dest, *new_path;
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
dev_path_pci(struct string *str, const void *dev_path)
{
	const struct pci_device_path *Pci;

	Pci = dev_path;
	cprintf(str, "Pci(0x%x,0x%x)", Pci->Device, Pci->Function);
}

static void
dev_path_pccard(struct string *str, const void *dev_path)
{
	const struct pccard_device_path *Pccard;

	Pccard = dev_path;
	cprintf(str, "Pccard(0x%x)", Pccard->function_number);
}

static void
dev_path_mem_map(struct string *str, const void *dev_path)
{
	const struct memmap_device_path *mem_map;

	mem_map = dev_path;
	cprintf(str, "mem_map(%d,0x%llx,0x%llx)",
		mem_map->memory_type,
		mem_map->starting_address, mem_map->ending_address);
}

static void
dev_path_controller(struct string *str, const void *dev_path)
{
	const struct controller_device_path *Controller;

	Controller = dev_path;
	cprintf(str, "Ctrl(%d)", Controller->Controller);
}

static void
dev_path_vendor(struct string *str, const void *dev_path)
{
	const struct vendor_device_path *Vendor;
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
dev_path_acpi(struct string *str, const void *dev_path)
{
	const struct acpi_hid_device_path *Acpi;

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
	const struct atapi_device_path *Atapi;

	Atapi = dev_path;
	cprintf(str, "Ata(%s,%s)",
		Atapi->primary_secondary ? "Secondary" : "Primary",
		Atapi->slave_master ? "Slave" : "Master");
}

static void
dev_path_scsi(struct string *str, const void *dev_path)
{
	const struct scsi_device_path *Scsi;

	Scsi = dev_path;
	cprintf(str, "Scsi(%d,%d)", Scsi->Pun, Scsi->Lun);
}

static void
dev_path_fibre(struct string *str, const void *dev_path)
{
	const struct fibrechannel_device_path *Fibre;

	Fibre = dev_path;
	cprintf(str, "Fibre%s(0x%016llx,0x%016llx)",
		device_path_type(&Fibre->header) ==
		MSG_FIBRECHANNEL_DP ? "" : "Ex", Fibre->WWN, Fibre->Lun);
}

static void
dev_path1394(struct string *str, const void *dev_path)
{
	const struct f1394_device_path *F1394;

	F1394 = dev_path;
	cprintf(str, "1394(%pU)", &F1394->Guid);
}

static void
dev_path_usb(struct string *str, const void *dev_path)
{
	const struct usb_device_path *Usb;

	Usb = dev_path;
	cprintf(str, "Usb(0x%x,0x%x)", Usb->Port, Usb->Endpoint);
}

static void
dev_path_i2_o(struct string *str, const void *dev_path)
{
	const struct i2_o_device_path *i2_o;

	i2_o = dev_path;
	cprintf(str, "i2_o(0x%X)", i2_o->Tid);
}

static void
dev_path_mac_addr(struct string *str, const void *dev_path)
{
	const struct mac_addr_device_path *MAC;
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
	const struct ipv4_device_path *ip;
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
	const struct ipv6_device_path *ip;

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
dev_path_infini_band(struct string *str, const void *dev_path)
{
	const struct infiniband_device_path *infini_band;

	infini_band = dev_path;
	cprintf(str, "Infiniband(0x%x,%pU,0x%llx,0x%llx,0x%llx)",
		infini_band->resource_flags, &infini_band->port_gid,
		infini_band->service_id, infini_band->target_port_id,
		infini_band->device_id);
}

static void
dev_path_uart(struct string *str, const void *dev_path)
{
	const struct uart_device_path *Uart;
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
	const struct sata_device_path *sata;

	sata = dev_path;
	cprintf(str, "Sata(0x%x,0x%x,0x%x)", sata->HBAPort_number,
		sata->port_multiplier_port_number, sata->Lun);
}

static void
dev_path_hard_drive(struct string *str, const void *dev_path)
{
	const struct harddrive_device_path *hd;

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
dev_path_cdrom(struct string *str, const void *dev_path)
{
	const struct cdrom_device_path *cd;

	cd = dev_path;
	cprintf(str, "CDROM(0x%x)", cd->boot_entry);
}

static void
dev_path_file_path(struct string *str, const void *dev_path)
{
	const struct filepath_device_path *Fp;
	char *dst;

	Fp = dev_path;

	dst = strdup_wchar_to_char(Fp->path_name);

	cprintf(str, "%s", dst);

	free(dst);
}

static void
dev_path_media_protocol(struct string *str, const void *dev_path)
{
	const struct media_protocol_device_path *media_prot;

	media_prot = dev_path;
	cprintf(str, "%pU", &media_prot->Protocol);
}

static void
dev_path_bss_bss(struct string *str, const void *dev_path)
{
	const struct bbs_bbs_device_path *Bss;
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
	void (*Function) (struct string *, const void *);
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

static void __device_path_to_str(struct string *str,
				 const struct efi_device_path *dev_path)
{
	const struct efi_device_path *dev_path_node;
	void (*dump_node) (struct string *, const void *);
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

	dev_path = unpack_device_path(dev_path);
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

	dev_path = unpack_device_path(dev_path);
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
	dev_path = unpack_device_path(dev_path);

	while ((dev_path = device_path_next_compatible_node(dev_path,
				 MEDIA_DEVICE_PATH, MEDIA_HARDDRIVE_DP))) {
		struct harddrive_device_path *hd =
			(struct harddrive_device_path *)dev_path;

		if (hd->signature_type != SIGNATURE_TYPE_GUID)
			continue;

		return xasprintf("%pUl", (efi_guid_t *)&(hd->signature[0]));
	}

	return NULL;
}

char *device_path_to_filepath(const struct efi_device_path *dev_path)
{
	struct filepath_device_path *fp = NULL;
	char *path;

	dev_path = unpack_device_path(dev_path);

	while ((dev_path = device_path_next_compatible_node(dev_path,
				 MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP))) {
		fp = container_of(dev_path, struct filepath_device_path, header);
		dev_path = next_device_path_node(&fp->header);
	}

	path = strdup_wchar_to_char(fp->path_name);
	if (!path)
		return NULL;

	return strreplace(path, '\\', '/');
}
