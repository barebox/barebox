/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * ioport.h	Definitions of routines for detecting, reserving and
 *		allocating system resources.
 *
 * Authors:	Linus Torvalds
 */

#ifndef _LINUX_IOPORT_H
#define _LINUX_IOPORT_H

#ifndef __ASSEMBLY__
#include <linux/list.h>
#include <linux/compiler.h>
#include <linux/types.h>
/*
 * Resources are tree-like, allowing
 * nesting etc..
 */
struct resource {
	resource_size_t start;
	resource_size_t end;
	const char *name;
	unsigned int flags;
	unsigned int type:4;
	unsigned int attrs:27;
	unsigned int runtime:1;
	struct resource *parent;
	struct list_head children;
	struct list_head sibling;
};

/*
 * IO resources have these defined flags.
 */
#define IORESOURCE_BITS		0x000000ff	/* Bus-specific bits */

#define IORESOURCE_TYPE_BITS	0x00001f00	/* Resource type */
#define IORESOURCE_IO		0x00000100
#define IORESOURCE_MEM		0x00000200
#define IORESOURCE_IRQ		0x00000400
#define IORESOURCE_DMA		0x00000800
#define IORESOURCE_BUS		0x00001000

#define IORESOURCE_PREFETCH	0x00002000	/* No side effects */
#define IORESOURCE_READONLY	0x00004000
#define IORESOURCE_CACHEABLE	0x00008000
#define IORESOURCE_RANGELENGTH	0x00010000
#define IORESOURCE_SHADOWABLE	0x00020000

#define IORESOURCE_SIZEALIGN	0x00040000	/* size indicates alignment */
#define IORESOURCE_STARTALIGN	0x00080000	/* start field is alignment */

#define IORESOURCE_MEM_64	0x00100000
#define IORESOURCE_WINDOW	0x00200000	/* forwarded by bridge */
#define IORESOURCE_MUXED	0x00400000	/* Resource is software muxed */

#define IORESOURCE_TYPE_VALID	0x00800000	/* type & attrs are valid */

#define IORESOURCE_EXCLUSIVE	0x08000000	/* Userland may not map this resource */
#define IORESOURCE_DISABLED	0x10000000
#define IORESOURCE_UNSET	0x20000000
#define IORESOURCE_AUTO		0x40000000
#define IORESOURCE_BUSY		0x80000000	/* Driver has marked this resource busy */

/* PnP IRQ specific bits (IORESOURCE_BITS) */
#define IORESOURCE_IRQ_HIGHEDGE		(1<<0)
#define IORESOURCE_IRQ_LOWEDGE		(1<<1)
#define IORESOURCE_IRQ_HIGHLEVEL	(1<<2)
#define IORESOURCE_IRQ_LOWLEVEL		(1<<3)
#define IORESOURCE_IRQ_SHAREABLE	(1<<4)
#define IORESOURCE_IRQ_OPTIONAL		(1<<5)

/* PnP DMA specific bits (IORESOURCE_BITS) */
#define IORESOURCE_DMA_TYPE_MASK	(3<<0)
#define IORESOURCE_DMA_8BIT		(0<<0)
#define IORESOURCE_DMA_8AND16BIT	(1<<0)
#define IORESOURCE_DMA_16BIT		(2<<0)

#define IORESOURCE_DMA_MASTER		(1<<2)
#define IORESOURCE_DMA_BYTE		(1<<3)
#define IORESOURCE_DMA_WORD		(1<<4)

#define IORESOURCE_DMA_SPEED_MASK	(3<<6)
#define IORESOURCE_DMA_COMPATIBLE	(0<<6)
#define IORESOURCE_DMA_TYPEA		(1<<6)
#define IORESOURCE_DMA_TYPEB		(2<<6)
#define IORESOURCE_DMA_TYPEF		(3<<6)

/* PnP memory I/O specific bits (IORESOURCE_BITS) */
#define IORESOURCE_MEM_WRITEABLE	(1<<0)	/* dup: IORESOURCE_READONLY */
#define IORESOURCE_MEM_CACHEABLE	(1<<1)	/* dup: IORESOURCE_CACHEABLE */
#define IORESOURCE_MEM_RANGELENGTH	(1<<2)	/* dup: IORESOURCE_RANGELENGTH */
#define IORESOURCE_MEM_TYPE_MASK	(3<<3)
#define IORESOURCE_MEM_8BIT		(0<<3)
#define IORESOURCE_MEM_16BIT		(1<<3)
#define IORESOURCE_MEM_8AND16BIT	(2<<3)
#define IORESOURCE_MEM_32BIT		(3<<3)
#define IORESOURCE_MEM_SHADOWABLE	(1<<5)	/* dup: IORESOURCE_SHADOWABLE */
#define IORESOURCE_MEM_EXPANSIONROM	(1<<6)

enum resource_memtype {
        MEMTYPE_RESERVED,
        MEMTYPE_LOADER_CODE,
        MEMTYPE_LOADER_DATA,
        MEMTYPE_BOOT_SERVICES_CODE,
        MEMTYPE_BOOT_SERVICES_DATA,
        MEMTYPE_RUNTIME_SERVICES_CODE,
        MEMTYPE_RUNTIME_SERVICES_DATA,
        MEMTYPE_CONVENTIONAL,
        MEMTYPE_UNUSABLE,
        MEMTYPE_ACPI_RECLAIM,
        MEMTYPE_ACPI_NVS,
        MEMTYPE_MMIO,
        MEMTYPE_MMIO_PORT,
        MEMTYPE_PAL_CODE,
        MEMTYPE_PERSISTENT,
        MEMTYPE_UNACCEPTED,
        MEMTYPE_MAX,
};

#define MEMATTR_UC	0x00000001	/* uncached */
#define MEMATTR_WC	0x00000002	/* write-coalescing */
#define MEMATTR_WT	0x00000004	/* write-through */
#define MEMATTR_WB	0x00000008	/* write-back */
#define MEMATTR_UCE	0x00000010	/* uncached, exported */
#define MEMATTR_WP	0x00001000	/* write-protect */
#define MEMATTR_RP	0x00002000	/* read-protect */
#define MEMATTR_XP	0x00004000	/* execute-protect */
#define MEMATTR_NV	0x00008000	/* non-volatile */
#define MEMATTR_MORE_RELIABLE 0x00010000  /* higher reliability */
#define MEMATTR_RO	0x00020000	/* read-only */
#define MEMATTR_SP	0x00040000	/* specific-purpose */

#define MEMATTRS_CACHEABLE	(MEMATTR_WT | MEMATTR_WC | MEMATTR_WB)
#define MEMATTRS_RW		(MEMATTRS_CACHEABLE | MEMATTR_XP)
#define MEMATTRS_RO		(MEMATTRS_CACHEABLE | MEMATTR_XP | MEMATTR_RO)
#define MEMATTRS_RX		(MEMATTRS_CACHEABLE | MEMATTR_RO)
#define MEMATTRS_RWX		(MEMATTRS_CACHEABLE)	/* TODO: remove all */
#define MEMATTRS_RW_DEVICE	(MEMATTR_UC | MEMATTR_XP)
#define MEMATTRS_FAULT		(MEMATTR_UC | MEMATTR_XP | MEMATTR_RP | MEMATTR_RO)

/* PnP I/O specific bits (IORESOURCE_BITS) */
#define IORESOURCE_IO_16BIT_ADDR	(1<<0)
#define IORESOURCE_IO_FIXED		(1<<1)

/* PCI ROM control bits (IORESOURCE_BITS) */
#define IORESOURCE_ROM_ENABLE		(1<<0)	/* ROM is enabled, same as PCI_ROM_ADDRESS_ENABLE */
#define IORESOURCE_ROM_SHADOW		(1<<1)	/* ROM is copy at C000:0 */
#define IORESOURCE_ROM_COPY		(1<<2)	/* ROM is alloc'd copy, resource field overlaid */
#define IORESOURCE_ROM_BIOS_COPY	(1<<3)	/* ROM is BIOS copy, resource field overlaid */

/* PCI control bits.  Shares IORESOURCE_BITS with above PCI ROM.  */
#define IORESOURCE_PCI_FIXED		(1<<4)	/* Do not move resource */

/* Helpers to define resources */
#define DEFINE_RES_NAMED(_start, _size, _name, _flags)	\
	{						\
		.start	= (_start),			\
		.end	= (_start) + (_size) - 1,	\
		.name	= (_name),			\
		.flags	= (_flags),			\
	}

#define DEFINE_RES_MEM_NAMED(_start, _size, _name)	\
	DEFINE_RES_NAMED((_start), (_size), (_name), IORESOURCE_MEM)
#define DEFINE_RES_MEM(_start, _size)			\
	DEFINE_RES_MEM_NAMED((_start), (_size), NULL)

static inline resource_size_t resource_size(const struct resource *res)
{
	return res->end - res->start + 1;
}

static inline unsigned long resource_type(const struct resource *res)
{
	return res->flags & IORESOURCE_TYPE_BITS;
}

/* True iff r1 completely contains r2 */
static inline bool resource_contains(struct resource *r1, struct resource *r2)
{
	if (resource_type(r1) != resource_type(r2))
		return false;
	if (r1->flags & IORESOURCE_UNSET || r2->flags & IORESOURCE_UNSET)
		return false;
	return r1->start <= r2->start && r1->end >= r2->end;
}

/* True if r1 and r2 are adjacent to each other */
static inline bool resource_adjacent(struct resource *r1, struct resource *r2)
{
	return (r1->end != ~(resource_size_t)0 && r1->end + 1 == r2->start) ||
		(r2->end != ~(resource_size_t)0 && r2->end + 1 == r1->start);
}

struct resource *request_iomem_region(const char *name,
		resource_size_t start, resource_size_t end);
struct resource *request_ioport_region(const char *name,
		resource_size_t start, resource_size_t end);

struct resource *__request_region(struct resource *parent,
				  resource_size_t start, resource_size_t end,
				  const char *name, unsigned flags);

int __merge_regions(const char *name,
		struct resource *resa, struct resource *resb);

int release_region(struct resource *res);

extern struct resource iomem_resource;
extern struct resource ioport_resource;

static inline void reserve_resource(struct resource *res)
{
	res->type = MEMTYPE_RESERVED;
	/* Reserved memory is used for secure memory that should
	 * be hardware-protected independently of MMU flags.
	 * We map it as device memory, so we can still test
	 * if it's indeed inaccessible
	 */
	res->attrs = MEMATTRS_RW_DEVICE;
	res->flags |= IORESOURCE_TYPE_VALID;
}

static inline bool is_reserved_resource(const struct resource *res)
{
	if (res->flags & IORESOURCE_TYPE_VALID)
		return res->type == MEMTYPE_RESERVED;
	return false;
}

#endif /* __ASSEMBLY__ */
#endif	/* _LINUX_IOPORT_H */
