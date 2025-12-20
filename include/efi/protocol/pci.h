/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_PROTOCOL_PCI_H_
#define __EFI_PROTOCOL_PCI_H_

#include <efi/types.h>
#include <efi/memory.h>

struct efi_pci_root_bridge_io_protocol;
struct efi_pci_io_protocol;

enum efi_pci_protocol_width {
	EFI_PCI_WIDTH_U8,
	EFI_PCI_WIDTH_U16,
	EFI_PCI_WIDTH_U32,
	EFI_PCI_WIDTH_U64,
	EFI_PCI_WIDTH_FIFO_U8,
	EFI_PCI_WIDTH_FIFO_U16,
	EFI_PCI_WIDTH_FIFO_U32,
	EFI_PCI_WIDTH_FIFO_U64,
	EFI_PCI_WIDTH_FILL_U8,
	EFI_PCI_WIDTH_FILL_U16,
	EFI_PCI_WIDTH_FILL_U32,
	EFI_PCI_WIDTH_FILL_U64,
	EFI_PCI_WIDTH_MAX
};

#define EFI_PCI_IO_PASS_THROUGH_BAR	0xff

typedef efi_status_t (EFIAPI *efi_pci_root_bridge_io_protocol_io_mem) (
	struct efi_pci_root_bridge_io_protocol *this,
	enum efi_pci_protocol_width width,
	u64 addr,
	size_t count,
	void *buf
);

typedef efi_status_t (EFIAPI *efi_pci_io_protocol_io_mem) (
	struct efi_pci_io_protocol *this,
	enum efi_pci_protocol_width width,
	u8 bar,
	u64 offset,
	size_t count,
	void *buf
);

struct efi_pci_root_bridge_io_protocol_access {
	efi_pci_root_bridge_io_protocol_io_mem read;
	efi_pci_root_bridge_io_protocol_io_mem write;
};

struct efi_pci_io_protocol_access {
	efi_pci_io_protocol_io_mem read;
	efi_pci_io_protocol_io_mem write;
};

#define EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO   0x0001
#define EFI_PCI_ATTRIBUTE_ISA_IO               0x0002
#define EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO       0x0004
#define EFI_PCI_ATTRIBUTE_VGA_MEMORY           0x0008
#define EFI_PCI_ATTRIBUTE_VGA_IO               0x0010
#define EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO       0x0020
#define EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO     0x0040
#define EFI_PCI_ATTRIBUTE_MEMORY_WRITE_COMBINE 0x0080
#define EFI_PCI_ATTRIBUTE_IO                   0x0100
#define EFI_PCI_ATTRIBUTE_MEMORY               0x0200
#define EFI_PCI_ATTRIBUTE_BUS_MASTER           0x0400
#define EFI_PCI_ATTRIBUTE_MEMORY_CACHED        0x0800
#define EFI_PCI_ATTRIBUTE_MEMORY_DISABLE       0x1000
#define EFI_PCI_ATTRIBUTE_EMBEDDED_DEVICE      0x2000
#define EFI_PCI_ATTRIBUTE_EMBEDDED_ROM         0x4000
#define EFI_PCI_ATTRIBUTE_DUAL_ADDRESS_CYCLE   0x8000
#define EFI_PCI_ATTRIBUTE_ISA_IO_16            0x10000
#define EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16    0x20000
#define EFI_PCI_ATTRIBUTE_VGA_IO_16            0x40000

typedef efi_status_t (EFIAPI *efi_pci_root_bridge_io_protocol_poll_io_mem) (
	struct efi_pci_root_bridge_io_protocol	*this,
	enum efi_pci_protocol_width width,
	u64 address,
	u64 mask,
	u64 value,
	u64 delay,
	u64 *result
);

typedef efi_status_t (EFIAPI *efi_pci_io_protocol_poll_io_mem) (
	struct efi_pci_io_protocol	*this,
	enum efi_pci_protocol_width width,
	u8 bar,
	u64 offset,
	u64 mask,
	u64 value,
	u64 delay,
	u64 *result
);

enum efi_pci_root_bridge_io_protocol_operation {
	EFI_PCI_ROOT_BRIGE_IO_OP_BUS_MASTER_READ,
	EFI_PCI_ROOT_BRIGE_IO_OP_BUS_MASTER_WRITE,
	EFI_PCI_ROOT_BRIGE_IO_OP_BUS_MASTER_COMMON_BUFFER,
	EFI_PCI_ROOT_BRIGE_IO_OP_BUS_MASTER_READ64,
	EFI_PCI_ROOT_BRIGE_IO_OP_BUS_MASTER_WRITE64,
	EFI_PCI_ROOT_BRIGE_IO_OP_BUS_MASTER_COMMON_BUFFER64,
	EFI_PCI_ROOT_BRIGE_IO_OP_MAX
};

enum efi_pci_io_protocol_operation {
	EFI_PCI_IO_OP_BUS_MASTER_READ,
	EFI_PCI_IO_OP_BUS_MASTER_WRITE,
	EFI_PCI_IO_OP_BUS_MASTER_COMMON_BUFFER,
	EFI_PCI_IO_OP_MAX
};

typedef efi_status_t (EFIAPI *efi_pci_root_bridge_io_protocol_copy_mem) (
	struct efi_pci_root_bridge_io_protocol *this,
	enum efi_pci_protocol_width width,
	u64 dst,
	u64 src,
	size_t count
);

typedef efi_status_t (EFIAPI *efi_pci_io_protocol_copy_mem) (
	struct efi_pci_io_protocol *this,
	enum efi_pci_protocol_width width,
	u8 dst_bar,
	u64 dst_offset,
	u8 src_bar,
	u64 src_offset,
	size_t count
);

typedef efi_status_t (EFIAPI *efi_pci_root_bridge_io_protocol_map) (
	struct efi_pci_root_bridge_io_protocol *this,
	enum efi_pci_root_bridge_io_protocol_operation operation,
	void *hostaddr,
	size_t *nbytes,
	efi_physical_addr_t *devaddr,
	void **mapping
);

typedef efi_status_t (EFIAPI *efi_pci_io_protocol_map) (
	struct efi_pci_io_protocol *this,
	enum efi_pci_io_protocol_operation operation,
	void *hostaddr,
	size_t *nbytes,
	efi_physical_addr_t *devaddr,
	void **mapping
);

typedef efi_status_t (EFIAPI *efi_pci_root_bridge_io_protocol_unmap) (
	struct efi_pci_root_bridge_io_protocol *this,
	void *mapping
);

typedef efi_status_t (EFIAPI *efi_pci_io_protocol_unmap) (
	struct efi_pci_io_protocol *this,
	void *mapping
);

typedef efi_status_t (EFIAPI *efi_pci_root_bridge_io_protocol_allocate_buffer) (
	struct efi_pci_root_bridge_io_protocol *this,
	enum efi_allocate_type alloctype,
	enum efi_memory_type memtype,
	size_t npages,
	void **hostaddr,
	u64 attrs
);

typedef efi_status_t (EFIAPI *efi_pci_io_protocol_allocate_buffer) (
	struct efi_pci_io_protocol *this,
	enum efi_allocate_type alloctype,
	enum efi_memory_type memtype,
	size_t npages,
	void **hostaddr,
	u64 attrs
);

typedef efi_status_t (EFIAPI *efi_pci_root_bridge_io_protocol_free_buffer) (
	struct efi_pci_root_bridge_io_protocol *this,
	size_t npages,
	void *hostaddr
);

typedef efi_status_t (EFIAPI *efi_pci_io_protocol_free_buffer) (
	struct efi_pci_io_protocol *this,
	size_t npages,
	void *hostaddr
);

typedef efi_status_t (EFIAPI *efi_pci_root_bridge_io_protocol_flush) (
	struct efi_pci_root_bridge_io_protocol *this
);

typedef efi_status_t (EFIAPI *efi_pci_io_protocol_flush) (
	struct efi_pci_io_protocol *this
);

typedef efi_status_t (EFIAPI *efi_pci_io_protocol_get_location) (
	struct efi_pci_io_protocol *this,
	size_t *segmentno,
	size_t *busno,
	size_t *deveno,
	size_t *funcno
);

enum efi_io_protocol_attribute_operation {
	PCI_IO_ATTR_OP_GET,
	PCI_IO_ATTR_OP_SET,
	PCI_IO_ATTR_OP_ENABLE,
	PCI_IO_ATTR_OP_DISABLE,
	PCI_IO_ATTR_OP_SUPPORTED,
	PCI_IO_ATTR_OP_MAX,
};

typedef efi_status_t (EFIAPI *efi_pci_io_protocol_attributes) (
	struct efi_pci_io_protocol *this,
	enum efi_io_protocol_attribute_operation operation,
	u64 attrs,
	u64 *result
);

typedef efi_status_t (EFIAPI *efi_pci_root_bridge_io_protocol_get_attributes) (
	struct efi_pci_root_bridge_io_protocol *this,
	u64 *supports,
	u64 *attrs
);

typedef efi_status_t (EFIAPI *efi_pci_io_protocol_get_bar_attributes) (
	struct efi_pci_io_protocol *this,
	u8 bar,
	u64 *supports,
	void **resources
);

typedef efi_status_t (EFIAPI *efi_pci_root_bridge_io_protocol_set_attributes) (
	struct efi_pci_root_bridge_io_protocol *this,
	u64 attrs,
	u64 *resource_base,
	u64 *resource_len
);

typedef efi_status_t (EFIAPI *efi_pci_io_protocol_set_bar_attributes) (
	struct efi_pci_io_protocol *this,
	u64 attrs,
	u8 bar,
	u64 *offset,
	u64 *len
);

typedef efi_status_t (EFIAPI *efi_pci_root_bridge_io_protocol_configuration) (
	struct efi_pci_root_bridge_io_protocol *this,
	void **resources
);

typedef efi_status_t (EFIAPI *efi_pci_io_protocol_config) (
	struct efi_pci_io_protocol *this,
	enum efi_pci_protocol_width width,
	u32 offset,
	size_t count,
	void *buffer
);

struct efi_pci_io_protocol_config_access {
	efi_pci_io_protocol_config read;
	efi_pci_io_protocol_config write;
};

struct __packed efi_acpi_resource {
#define	ACPI_RESOURCE_DESC_TAG	0x8A
#define	ACPI_RESOURCE_END_TAG	0x79
	u8 asd; /* 0x8A */
	u16 len; /* 0x2B */
#define	ACPI_RESOURCE_TYPE_MEM		0
#define	ACPI_RESOURCE_TYPE_IO		1
#define	ACPI_RESOURCE_TYPE_BUSNO	2
	u8 restype;
	u8 genflags;
#define	ACPI_RESOURCE_TYPFLAG_TTP_MASK	0b00100000
#define	ACPI_RESOURCE_TYPFLAG_MTP_MASK	0b00011000
#define		ACPI_RESOURCE_TYPFLAG_MTP_MEM		0
#define		ACPI_RESOURCE_TYPFLAG_MTP_RESERVED	1
#define		ACPI_RESOURCE_TYPFLAG_MTP_ACPI		2
#define		ACPI_RESOURCE_TYPFLAG_MTP_NVS		3
#define	ACPI_RESOURCE_TYPFLAG_MEM_MASK	0b00000110
#define		ACPI_RESOURCE_TYPFLAG_MEM_NONCACHEABLE	0
#define		ACPI_RESOURCE_TYPFLAG_MEM_CACHEABLE	1
#define		ACPI_RESOURCE_TYPFLAG_MEM_WC		2
#define		ACPI_RESOURCE_TYPFLAG_MEM_PREF		3
#define	ACPI_RESOURCE_TYPFLAG_RW_MASK	0b00000001
	u8 typflags;
	u64 addr_granularity;
	u64 addr_min;
	u64 addr_max;
	u64 addr_xlate_off;
	u64 addr_len;
};

struct efi_pci_root_bridge_io_protocol {
	efi_handle_t					parent_handle;
	efi_pci_root_bridge_io_protocol_poll_io_mem	poll_mem;
	efi_pci_root_bridge_io_protocol_poll_io_mem	poll_io;
	struct efi_pci_root_bridge_io_protocol_access	mem;
	struct efi_pci_root_bridge_io_protocol_access	io;
	struct efi_pci_root_bridge_io_protocol_access	pci;
	efi_pci_root_bridge_io_protocol_copy_mem	copy_mem;
	efi_pci_root_bridge_io_protocol_map		map;
	efi_pci_root_bridge_io_protocol_unmap		unmap;
	efi_pci_root_bridge_io_protocol_allocate_buffer	allocate_buffer;
	efi_pci_root_bridge_io_protocol_free_buffer	free_buffer;
	efi_pci_root_bridge_io_protocol_flush		flush;
	efi_pci_root_bridge_io_protocol_get_attributes	get_attributes;
	efi_pci_root_bridge_io_protocol_set_attributes	set_attributes;
	efi_pci_root_bridge_io_protocol_configuration	configuration;
	u32						segmentno;
};

struct efi_pci_io_protocol {
	efi_pci_io_protocol_poll_io_mem		poll_mem;
	efi_pci_io_protocol_poll_io_mem		poll_io;
	struct efi_pci_io_protocol_access	mem;
	struct efi_pci_io_protocol_access	io;
	struct efi_pci_io_protocol_access	pci;
	efi_pci_io_protocol_copy_mem		copy_mem;
	efi_pci_io_protocol_map			map;
	efi_pci_io_protocol_unmap		unmap;
	efi_pci_io_protocol_allocate_buffer	allocate_buffer;
	efi_pci_io_protocol_free_buffer		free_buffer;
	efi_pci_io_protocol_flush		flush;
	efi_pci_io_protocol_get_location	get_location;
	efi_pci_io_protocol_attributes		attributes;
	efi_pci_io_protocol_get_bar_attributes	get_bar_attributes;
	efi_pci_io_protocol_set_bar_attributes	set_bar_attributes;
	u64					rom_size;
	void					*rom_image;
};

#define EFI_PCI_ADDRESS(bus, dev, func, reg) \
  (u64) ( \
  (((size_t) bus) << 24) | \
  (((size_t) dev) << 16) | \
  (((size_t) func) << 8) | \
  (((size_t) (reg)) < 256 ? ((size_t) (reg)) : ((u64)(reg)) << 32))

#endif
