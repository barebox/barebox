// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <net.h>
#include <tlv/tlv.h>

int tlv_handle_serial(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val)
{
	const char *str;

	str = __tlv_format_str(dev, map, len, val);
	if (!str)
		return -ENOMEM;

	barebox_set_serial_number(str);
	return 0;
}

int tlv_handle_eth_address(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val)
{
	int i;

	if (len % ETH_ALEN != 0)
		return -EINVAL;

	for (i = 0; i < len / ETH_ALEN; i++)
		eth_register_ethaddr(i, val + i * ETH_ALEN);

	return tlv_format_mac(dev, map, len, val);
}

int tlv_handle_eth_address_seq(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val)
{
	u8 eth_addr[ETH_ALEN];
	int eth_count;

	eth_count = *val;

	if (len != 1 + ETH_ALEN)
		return -EINVAL;

	memcpy(eth_addr, val + 1, ETH_ALEN);

	for (int i = 0; i < eth_count; i++, eth_addr_inc(eth_addr)) {
		eth_register_ethaddr(i, eth_addr);
		tlv_format_mac(dev, map, ETH_ALEN, eth_addr);
	}

	return 0;
}

static const char *__tlv_format(struct tlv_device *dev, struct tlv_mapping *map, char *buf)
{
	struct param_d *param;
	int ret;

	if (!buf)
		return NULL;

	param = dev_add_param_fixed(&dev->dev, map->prop, NULL);
	if (!IS_ERR(param))
		param->value = buf; /* pass ownership */

	ret = of_append_property(tlv_of_node(dev), map->prop, buf, strlen(buf) + 1);
	if (ret)
		return NULL;

	return buf;
}

#define tlv_format(tlvdev, map, ...) ({ __tlv_format(tlvdev, map, basprintf(__VA_ARGS__)) ? 0 : -ENOMEM; })

const char * __tlv_format_str(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val)
{
	return __tlv_format(dev, map, basprintf("%.*s", len, val));
}

int tlv_format_str(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val)
{
	return __tlv_format_str(dev, map, len, val) ? 0 : -ENOMEM;
}

int tlv_format_hex(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val)
{
	return tlv_format(dev, map, "%*ph", len, val);
}

int tlv_format_blob(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val)
{
	struct param_d *param;

	param = dev_add_param_fixed(&dev->dev, map->prop, NULL);
	if (!IS_ERR(param))
		param->value = basprintf("%*phN", len, val);

	return of_append_property(tlv_of_node(dev), map->prop, val, len);
}

static struct device_node *of_append_node(struct device_node *root, const char *name)
{
	struct device_node *np;

	np = of_get_child_by_name(root, name);
	if (np)
		return np;

	return of_new_node(root, name);
}

int tlv_format_mac(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val)
{
	struct device_node *np = tlv_of_node(dev);
	struct property *pp;
	char propname[sizeof("address-4294967295")];
	int base = 0, i, ret;

	if (len % 6 != 0)
		return -EINVAL;

	np = of_append_node(np, map->prop);
	if (!np)
		return -ENOMEM;

	for_each_property_of_node(np, pp)
		base++;

	for (i = base; i < base + len / 6; i++) {
		snprintf(propname, sizeof(propname), "address-%u", i);
		ret = of_property_sprintf(np, propname, "%*phC", 6, val);
		if (ret)
			return ret;
	}

	return 0;
}

int tlv_format_dec(struct tlv_device *dev, struct tlv_mapping *map, u16 len, const u8 *val)
{
	switch (len) {
	case 1:
		return tlv_format(dev, map, "%u", *(u8 *)val);
	case 2:
		return tlv_format(dev, map, "%u", get_unaligned_be16(val));
	case 4:
		return tlv_format(dev, map, "%u", get_unaligned_be32(val));
	case 8:
		return tlv_format(dev, map, "%llu", get_unaligned_be64(val));
	default:
		return tlv_format_hex(dev, map, len, val);
	}
}

struct tlv_mapping barebox_tlv_v1_mappings[] = {
	/* Detailed release information string for the device */
	{ 0x0002, tlv_format_str, "device-hardware-release" },
	/* UNIX timestamp of fabrication */
	{ 0x0003, tlv_format_timestamp, "factory-timestamp" },
	/* Device serial number string */
	{ 0x0004, tlv_handle_serial, "device-serial-number"},
	/* Modification: 0: Device unmodified; 1: undocumented modifications */
	{ 0x0005, tlv_format_dec, "modification" },
	/* a comma separated list of features */
	{ 0x0006, tlv_format_str, "featureset" },
	/* Printed Circuit Board Assembly serial number string */
	{ 0x0007, tlv_format_str, "pcba-serial-number"},
	/* Printed Circuit Board Assembly hardware release */
	{ 0x0008, tlv_format_str, "pcba-hardware-release"},
	/* A single Ethernet address */
	{ 0x0011, tlv_handle_eth_address, "ethernet-address" },
	/* A sequence of multiple Ethernet addresses */
	{ 0x0012, tlv_handle_eth_address_seq, "ethernet-address" },
	{ /* sentintel */ },
};

static struct tlv_mapping *mappings[] = { barebox_tlv_v1_mappings, NULL };
static struct of_device_id of_matches[] = {
	{ .compatible = "barebox,tlv-v1" },
	{ /* sentinel */}
};

static struct tlv_decoder barebox_tlv_v1 = {
	.magic = TLV_MAGIC_BAREBOX_V1,
	.driver.name = "barebox-tlv-v1",
	.driver.of_compatible = of_matches,
	.mappings = mappings,
};

static int tlv_register_default(void)
{
	return tlv_register_decoder(&barebox_tlv_v1);
}
device_initcall(tlv_register_default);
