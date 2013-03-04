/*
 *  linux/include/amba/bus.h
 *
 *  This device type deals with ARM PrimeCells and anything else that
 *  presents a proper CID (0xB105F00D) at the end of the I/O register
 *  region or that is derived from a PrimeCell.
 *
 *  Copyright (C) 2003 Deep Blue Solutions Ltd, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef ASMARM_AMBA_H
#define ASMARM_AMBA_H

#include <linux/clk.h>
#include <driver.h>
#include <linux/err.h>

#define AMBA_CID	0xb105f00d

/**
 * struct amba_id - identifies a device on an AMBA bus
 * @id: The significant bits if the hardware device ID
 * @mask: Bitmask specifying which bits of the id field are significant when
 *	matching.  A driver binds to a device when ((hardware device ID) & mask)
 *	== id.
 * @data: Private data used by the driver.
 */
struct amba_id {
	unsigned int		id;
	unsigned int		mask;
	void			*data;
};

struct clk;

struct amba_device {
	struct device_d		dev;
	struct resource		res;
	void __iomem		*base;
	struct clk		*pclk;
	unsigned int		periphid;
};

struct amba_driver {
	struct driver_d		drv;
	int			(*probe)(struct amba_device *, const struct amba_id *);
	int			(*remove)(struct amba_device *);
	const struct amba_id	*id_table;
};

enum amba_vendor {
	AMBA_VENDOR_ARM = 0x41,
	AMBA_VENDOR_ST = 0x80,
};

extern struct bus_type amba_bustype;

#define to_amba_device(d)	container_of(d, struct amba_device, dev)

#define amba_get_drvdata(d)	dev_get_drvdata(&d->dev)
#define amba_set_drvdata(d,p)	dev_set_drvdata(&d->dev, p)

int amba_driver_register(struct amba_driver *);
void amba_driver_unregister(struct amba_driver *);
struct amba_device *amba_device_alloc(const char *, int id, resource_size_t, size_t);
void amba_device_put(struct amba_device *);
int amba_device_add(struct amba_device *);
int amba_device_register(struct amba_device *, struct resource *);

struct amba_device *
amba_aphb_device_add(struct device_d *parent, const char *name, int id,
		     resource_size_t base, size_t size,
		     void *pdata, unsigned int periphid);

static inline struct amba_device *
amba_apb_device_add(struct device_d *parent, const char *name, int id,
		    resource_size_t base, size_t size,
		    void *pdata, unsigned int periphid)
{
	return amba_aphb_device_add(parent, name, id, base, size, pdata,
				    periphid);
}

static inline struct amba_device *
amba_ahb_device_add(struct device_d *parent, const char *name, int id,
		    resource_size_t base, size_t size,
		    void *pdata, unsigned int periphid)
{
	return amba_aphb_device_add(parent, name, id, base, size, pdata,
				    periphid);
}


void amba_device_unregister(struct amba_device *);
struct amba_device *amba_find_device(const char *, struct device_d *, unsigned int, unsigned int);
int amba_request_regions(struct amba_device *, const char *);
void amba_release_regions(struct amba_device *);

static inline void __iomem *amba_get_mem_region(struct amba_device *dev)
{
	return dev->base;
}

#define amba_pclk_enable(d)	\
	(IS_ERR((d)->pclk) ? 0 : clk_enable((d)->pclk))

#define amba_pclk_disable(d)	\
	do { if (!IS_ERR((d)->pclk)) clk_disable((d)->pclk); } while (0)

/* Some drivers don't use the struct amba_device */
#define AMBA_CONFIG_BITS(a) (((a) >> 24) & 0xff)
#define AMBA_REV_BITS(a) (((a) >> 20) & 0x0f)
#define AMBA_MANF_BITS(a) (((a) >> 12) & 0xff)
#define AMBA_PART_BITS(a) ((a) & 0xfff)

#define amba_config(d)	AMBA_CONFIG_BITS((d)->periphid)
#define amba_rev(d)	AMBA_REV_BITS((d)->periphid)
#define amba_manf(d)	AMBA_MANF_BITS((d)->periphid)
#define amba_part(d)	AMBA_PART_BITS((d)->periphid)

#define __AMBA_DEV(busid, data)				\
	{							\
		.init_name = busid,				\
		.platform_data = data,				\
	}

/*
 * APB devices do not themselves have the ability to address memory,
 * so DMA masks should be zero (much like USB peripheral devices.)
 * The DMA controller DMA masks should be used instead (much like
 * USB host controllers in conventional PCs.)
 */
#define AMBA_APB_DEVICE(name, busid, id, base, data)	\
struct amba_device name##_device = {				\
	.dev = __AMBA_DEV(busid, data),			\
	.res = DEFINE_RES_MEM(base, SZ_4K),			\
	.periphid = id,						\
}

/*
 * AHB devices are DMA capable, so set their DMA masks
 */
#define AMBA_AHB_DEVICE(name, busid, id, base, data)	\
struct amba_device name##_device = {				\
	.dev = __AMBA_DEV(busid, data),			\
	.res = DEFINE_RES_MEM(base, SZ_4K),			\
	.periphid = id,						\
}

#include <io.h>
/*
 * Read pid and cid based on size of resource
 * they are located at end of region
 */
static inline u32 amba_device_get_pid(void *base, u32 size)
{
	int i;
	u32 pid;

	for (pid = 0, i = 0; i < 4; i++)
		pid |= (readl(base + size - 0x20 + 4 * i) & 255) <<
			(i * 8);

	return pid;
}

static inline u32 amba_device_get_cid(void *base, u32 size)
{
	int i;
	u32 cid;

	for (cid = 0, i = 0; i < 4; i++)
		cid |= (readl(base + size - 0x10 + 4 * i) & 255) <<
			(i * 8);

	return cid;
}
#endif
