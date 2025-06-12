/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __TLV_H_
#define __TLV_H_

#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/err.h>
#include <asm/unaligned.h>
#include <unistd.h>
#include <driver.h>

#include <tlv/format.h>

struct tlv *tlv_next(const struct tlv_header *header, const struct tlv *tlv);

struct tlv_header *tlv_read(const char *filename, size_t *nread);

struct device_node;
struct tlv_device;
struct tlv_mapping;

struct tlv_mapping {
	u16 tag;
	int (*handle)(struct tlv_device *dev, struct tlv_mapping *map,
		      u16 len, const u8 *val);
	const char *prop;
};

extern struct tlv_mapping barebox_tlv_v1_mappings[];

extern const char *__tlv_format_str(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val);
extern int tlv_format_str(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val);
extern int tlv_format_dec(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val);
#define tlv_format_timestamp tlv_format_dec /* For timestamps since the UNIX epoch */
extern int tlv_format_hex(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val);
extern int tlv_format_mac(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val);
extern int tlv_format_blob(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val);
extern int tlv_handle_serial(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val);
extern int tlv_handle_eth_address(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val);
extern int tlv_handle_eth_address_seq(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val);

struct tlv_decoder {
	u32 magic;
	void *driverata;
	struct tlv_mapping **mappings;
	struct driver driver;
	/* private members */
	struct driver _platform_driver;
	struct list_head list;
};

struct tlv_device {
	struct device dev;
	u32 magic;
};

static inline struct device_node *tlv_of_node(struct tlv_device *tlvdev)
{
	return tlvdev->dev.device_node;
}

struct tlv_device *tlv_register_device(struct tlv_header *header, struct device *parent);
static inline struct tlv_header *tlv_device_header(struct tlv_device *tlvdev)
{
	return tlvdev->dev.platform_data;
}

void tlv_free_device(struct tlv_device *tlvdev);

int tlv_register_decoder(struct tlv_decoder *decoder);

int tlv_parse(struct tlv_device *tlvdev,
	      const struct tlv_decoder *decoder);

int of_tlv_fixup(struct device_node *root, void *ctx);

int tlv_of_register_fixup(struct tlv_device *tlvdev);
void tlv_of_unregister_fixup(struct tlv_device *tlvdev);

extern struct bus_type tlv_bus;

#define to_tlv_decoder(drv) ( \
	drv->bus == &tlv_bus ? container_of_const(drv, struct tlv_decoder, driver) : \
	drv->bus == &platform_bus ? container_of_const(drv, struct tlv_decoder, _platform_driver) : \
	NULL \
)

static inline struct tlv_device *to_tlv_device(struct device *dev)
{
	return container_of(dev, struct tlv_device, dev);
}

struct tlv_device *tlv_register_device_by_path(const char *path, struct device *parent);
struct tlv_device *tlv_ensure_probed_by_alias(const char *alias);

#endif
