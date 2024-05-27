/*
 * RAM Oops/Panic logger
 *
 * Copyright (C) 2010 Marco Stornelli <marco.stornelli@gmail.com>
 * Copyright (C) 2011 Kees Cook <keescook@chromium.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/pstore.h>
#include <linux/time.h>
#include <linux/ioport.h>
#include <linux/compiler.h>
#include <linux/pstore_ram.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/log2.h>
#include <linux/spinlock.h>
#include <malloc.h>
#include <linux/printk.h>
#include <stdio.h>
#include <globalvar.h>
#include <init.h>
#include <common.h>
#include <of.h>
#include <of_address.h>

#define RAMOOPS_KERNMSG_HDR "===="
#define MIN_MEM_SIZE 4096UL

static const ulong record_size = CONFIG_FS_PSTORE_RAMOOPS_RECORD_SIZE;

static const ulong ramoops_console_size = CONFIG_FS_PSTORE_RAMOOPS_CONSOLE_SIZE;

static const ulong ramoops_ftrace_size = CONFIG_FS_PSTORE_RAMOOPS_FTRACE_SIZE;

static const ulong ramoops_pmsg_size = CONFIG_FS_PSTORE_RAMOOPS_PMSG_SIZE;

static const ulong mem_size = CONFIG_FS_PSTORE_RAMOOPS_SIZE;

static const int dump_oops = 1;

static const int ramoops_ecc = CONFIG_FS_PSTORE_ECC_SIZE;

struct ramoops_context {
	struct persistent_ram_zone **przs;
	struct persistent_ram_zone *cprz;
	struct persistent_ram_zone *fprz;
	struct persistent_ram_zone *mprz;
	phys_addr_t phys_addr;
	unsigned long size;
	unsigned int memtype;
	size_t record_size;
	size_t console_size;
	size_t ftrace_size;
	size_t pmsg_size;
	int dump_oops;
	struct persistent_ram_ecc_info ecc_info;
	unsigned int max_dump_cnt;
	unsigned int dump_write_cnt;
	/* _read_cnt need clear on ramoops_pstore_open */
	unsigned int dump_read_cnt;
	unsigned int console_read_cnt;
	unsigned int ftrace_read_cnt;
	unsigned int pmsg_read_cnt;
	struct pstore_info pstore;
};

static struct ramoops_platform_data *dummy_data;

static int ramoops_pstore_open(struct pstore_info *psi)
{
	struct ramoops_context *cxt = psi->data;

	cxt->dump_read_cnt = 0;
	cxt->console_read_cnt = 0;
	cxt->ftrace_read_cnt = 0;
	cxt->pmsg_read_cnt = 0;
	return 0;
}

static struct persistent_ram_zone *
ramoops_get_next_prz(struct persistent_ram_zone *przs[], uint *c, uint max,
		     u64 *id,
		     enum pstore_type_id *typep, enum pstore_type_id type,
		     bool update)
{
	struct persistent_ram_zone *prz;
	int i = (*c)++;

	if (i >= max)
		return NULL;

	prz = przs[i];
	if (!prz)
		return NULL;

	/* Update old/shadowed buffer. */
	if (update)
		persistent_ram_save_old(prz);

	if (!persistent_ram_old_size(prz))
		return NULL;

	*typep = type;
	*id = i;

	return prz;
}

static bool prz_ok(struct persistent_ram_zone *prz)
{
	return !!prz && !!(persistent_ram_old_size(prz) +
			   persistent_ram_ecc_string(prz, NULL, 0));
}

static ssize_t ramoops_pstore_read(struct pstore_record *record)
{
	ssize_t size;
	ssize_t ecc_notice_size;
	struct ramoops_context *cxt = record->psi->data;
	struct persistent_ram_zone *prz;

	record->compressed = false;

	prz = ramoops_get_next_prz(cxt->przs, &cxt->dump_read_cnt,
				   cxt->max_dump_cnt, &record->id,
				   &record->type,
				   PSTORE_TYPE_DMESG, 0);
	if (!prz_ok(prz))
		prz = ramoops_get_next_prz(&cxt->cprz, &cxt->console_read_cnt,
					   1, &record->id, &record->type,
					   PSTORE_TYPE_CONSOLE, 0);

	if (!prz_ok(prz))
		prz = ramoops_get_next_prz(&cxt->fprz, &cxt->ftrace_read_cnt,
					   1, &record->id, &record->type,
					   PSTORE_TYPE_FTRACE, 0);
	if (!prz_ok(prz))
		prz = ramoops_get_next_prz(&cxt->mprz, &cxt->pmsg_read_cnt,
					   1, &record->id, &record->type,
					   PSTORE_TYPE_PMSG, 0);
	if (!prz_ok(prz))
		return 0;

	if (!persistent_ram_old(prz))
		return 0;

	size = persistent_ram_old_size(prz);

	/* ECC correction notice */
	ecc_notice_size = persistent_ram_ecc_string(prz, NULL, 0);

	record->buf = kmalloc(size + ecc_notice_size + 1, GFP_KERNEL);
	if (record->buf == NULL)
		return -ENOMEM;

	memcpy(record->buf, (char *)persistent_ram_old(prz), size);
	persistent_ram_ecc_string(prz, record->buf + size, ecc_notice_size + 1);

	return size + ecc_notice_size;
}

static int notrace ramoops_pstore_write_buf(enum pstore_type_id type,
					    enum kmsg_dump_reason reason,
					    u64 *id, unsigned int part,
					    const char *buf,
					    bool compressed, size_t size,
					    struct pstore_info *psi)
{
	struct ramoops_context *cxt = psi->data;
	struct persistent_ram_zone *prz;

	if (type == PSTORE_TYPE_CONSOLE) {
		if (!cxt->cprz)
			return -ENOMEM;
		persistent_ram_write(cxt->cprz, buf, size);
		return 0;
	} else if (type == PSTORE_TYPE_FTRACE) {
		if (!cxt->fprz)
			return -ENOMEM;
		persistent_ram_write(cxt->fprz, buf, size);
		return 0;
	} else if (type == PSTORE_TYPE_PMSG) {
		if (!cxt->mprz)
			return -ENOMEM;
		persistent_ram_write(cxt->mprz, buf, size);
		return 0;
	}

	if (type != PSTORE_TYPE_DMESG)
		return -EINVAL;

	/* Explicitly only take the first part of any new crash.
	 * If our buffer is larger than kmsg_bytes, this can never happen,
	 * and if our buffer is smaller than kmsg_bytes, we don't want the
	 * report split across multiple records.
	 */
	if (part != 1)
		return -ENOSPC;

	if (!cxt->przs)
		return -ENOSPC;

	prz = cxt->przs[cxt->dump_write_cnt];

	persistent_ram_write(prz, buf, size);

	cxt->dump_write_cnt = (cxt->dump_write_cnt + 1) % cxt->max_dump_cnt;

	return 0;
}

static int ramoops_pstore_erase(enum pstore_type_id type, u64 id, int count,
				struct pstore_info *psi)
{
	struct ramoops_context *cxt = psi->data;
	struct persistent_ram_zone *prz;

	switch (type) {
	case PSTORE_TYPE_DMESG:
		if (id >= cxt->max_dump_cnt)
			return -EINVAL;
		prz = cxt->przs[id];
		break;
	case PSTORE_TYPE_CONSOLE:
		prz = cxt->cprz;
		break;
	case PSTORE_TYPE_FTRACE:
		prz = cxt->fprz;
		break;
	case PSTORE_TYPE_PMSG:
		prz = cxt->mprz;
		break;
	default:
		return -EINVAL;
	}

	persistent_ram_free_old(prz);
	persistent_ram_zap(prz);

	return 0;
}

static struct ramoops_context oops_cxt = {
	.pstore = {
		.name	= "ramoops",
		.open	= ramoops_pstore_open,
		.read	= ramoops_pstore_read,
		.write_buf	= ramoops_pstore_write_buf,
		.erase	= ramoops_pstore_erase,
	},
};

static void ramoops_free_przs(struct ramoops_context *cxt)
{
	int i;

	cxt->max_dump_cnt = 0;
	if (!cxt->przs)
		return;

	for (i = 0; !IS_ERR_OR_NULL(cxt->przs[i]); i++)
		persistent_ram_free(cxt->przs[i]);
	kfree(cxt->przs);
}

static int ramoops_init_przs(const char *name,
			     struct ramoops_context *cxt,
			     struct persistent_ram_zone ***przs,
			     phys_addr_t *paddr, size_t mem_sz,
			     ssize_t record_size,
			     unsigned int *cnt, u32 sig, u32 flags)
{
	int err = -ENOMEM;
	int i;
	size_t zone_sz;
	struct persistent_ram_zone **prz_ar;

	/* Allocate nothing for 0 mem_sz or 0 record_size. */
	if (mem_sz == 0 || record_size == 0) {
		*cnt = 0;
		return 0;
	}

	/*
	 * If we have a negative record size, calculate it based on
	 * mem_sz / *cnt. If we have a positive record size, calculate
	 * cnt from mem_sz / record_size.
	 */
	if (record_size < 0) {
		if (*cnt == 0)
			return 0;
		record_size = mem_sz / *cnt;
		if (record_size == 0) {
			pr_err("%s record size == 0 (%zu / %u)\n",
				name, mem_sz, *cnt);
			goto fail;
		}
	} else {
		*cnt = mem_sz / record_size;
		if (*cnt == 0) {
			pr_err("%s record count == 0 (%zu / %zu)\n",
				name, mem_sz, record_size);
			goto fail;
		}
	}

	if (*paddr + mem_sz - cxt->phys_addr > cxt->size) {
		pr_err("no room for %s mem region (0x%zx@0x%llx) in (0x%lx@0x%llx)\n",
			name,
			mem_sz, (unsigned long long)*paddr,
			cxt->size, (unsigned long long)cxt->phys_addr);
		goto fail;
	}

	zone_sz = mem_sz / *cnt;
	if (!zone_sz) {
		pr_err("%s zone size == 0\n", name);
		goto fail;
	}

	prz_ar = kcalloc(*cnt, sizeof(**przs), GFP_KERNEL);
	if (!prz_ar)
		goto fail;

	for (i = 0; i < *cnt; i++) {
		char *label;

		if (*cnt == 1)
			label = basprintf("ramoops:%s", name);
		else
			label = basprintf("ramoops:%s(%d/%d)",
					  name, i, *cnt - 1);
		prz_ar[i] = persistent_ram_new(*paddr, zone_sz, sig,
					       &cxt->ecc_info,
					       cxt->memtype, label);
		if (IS_ERR(prz_ar[i])) {
			err = PTR_ERR(prz_ar[i]);
			pr_err("failed to request %s mem region (0x%zx@0x%llx): %d\n",
				name, record_size,
				(unsigned long long)*paddr, err);

			while (i > 0) {
				i--;
				persistent_ram_free(prz_ar[i]);
			}
			kfree(prz_ar);
			goto fail;
		}
		*paddr += zone_sz;
	}

	*przs = prz_ar;
	return 0;

fail:
	*cnt = 0;
	return err;
}

static int ramoops_init_prz(const char *name,
			    struct ramoops_context *cxt,
			    struct persistent_ram_zone **prz,
			    phys_addr_t *paddr, size_t sz, u32 sig)
{
	char *label;

	if (!sz)
		return 0;

	if (*paddr + sz - cxt->phys_addr > cxt->size) {
		pr_err("no room for %s mem region (0x%zx@0x%llx) in (0x%lx@0x%llx)\n",
			name, sz, (unsigned long long)*paddr,
			cxt->size, (unsigned long long)cxt->phys_addr);
		return -ENOMEM;
	}

	label = basprintf("ramoops:%s", name);
	*prz = persistent_ram_new(*paddr, sz, sig, &cxt->ecc_info,
				  cxt->memtype, label);
	if (IS_ERR(*prz)) {
		int err = PTR_ERR(*prz);

		pr_err("failed to request %s mem region (0x%zx@0x%llx): %d\n",
			name, sz, (unsigned long long)*paddr, err);
		return err;
	}

#ifndef CONFIG_FS_PSTORE_RAMOOPS_RO
	persistent_ram_zap(*prz);
#endif /* CONFIG_FS_PSTORE_RAMOOPS_RO */

	*paddr += sz;

	return 0;
}

static int ramoops_parse_dt_size(struct device *dev,
				 const char *propname, u32 *value)
{
	u32 val32 = 0;
	int ret;

	ret = of_property_read_u32(dev->of_node, propname, &val32);
	if (ret < 0 && ret != -EINVAL) {
		dev_err(dev, "failed to parse property %s: %d\n",
			propname, ret);
		return ret;
	}

	if (val32 > INT_MAX) {
		dev_err(dev, "%s %u > INT_MAX\n", propname, val32);
		return -EOVERFLOW;
	}

	*value = val32;
	return 0;
}

static int ramoops_parse_dt(struct device *dev,
			    struct ramoops_platform_data *pdata)
{
	struct device_node *of_node = dev->of_node;
	struct resource *res;
	u32 value;
	int ret;

	res = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev,
			"failed to locate DT /reserved-memory resource\n");
		return -EINVAL;
	}

	pdata->mem_size = resource_size(res);
	pdata->mem_address = res->start;
	pdata->mem_type = of_property_read_bool(of_node, "unbuffered");
	pdata->dump_oops = !of_property_read_bool(of_node, "no-dump-oops");

#define parse_size(name, field) {					\
		ret = ramoops_parse_dt_size(dev, name, &value);		\
		if (ret < 0)						\
		return ret;						\
		field = value;						\
	}

       parse_size("record-size", pdata->record_size);
       parse_size("console-size", pdata->console_size);
       parse_size("ftrace-size", pdata->ftrace_size);
       parse_size("pmsg-size", pdata->pmsg_size);
       parse_size("ecc-size", pdata->ecc_info.ecc_size);

#undef parse_size

       return 0;
}

static int ramoops_of_fixup(struct device_node *root, void *data)
{
	struct ramoops_platform_data *pdata = data;
	struct device_node *node;
	struct resource res = {};
	int ret;

	res.start = pdata->mem_address;
	res.end = res.start + pdata->mem_size - 1;
	res.name = "ramoops";

	ret = of_fixup_reserved_memory(root, &res);
	if (ret)
		return ret;

	node = of_find_node_by_path_from(root, "/reserved-memory/ramoops");
	if (!node)
		return -ENOMEM;

	ret = of_property_write_string(node, "compatible", "ramoops");
	if (ret)
		return ret;

	ret = of_property_write_bool(node, "unbuffered", pdata->mem_type);
	if (ret)
		return ret;
	ret = of_property_write_bool(node, "no-dump-oops", !pdata->dump_oops);
	if (ret)
		return ret;

#define store_size(name, field) {					\
		ret = of_property_write_u32(node, name, field);		\
		if (ret < 0)						\
			return ret;					\
	}

	store_size("record-size", pdata->record_size);
	store_size("console-size", pdata->console_size);
	store_size("ftrace-size", pdata->ftrace_size);
	store_size("pmsg-size", pdata->pmsg_size);
	store_size("ecc-size", pdata->ecc_info.ecc_size);

#undef store_size

	return 0;
}

static int ramoops_probe(struct device *dev)
{
	struct ramoops_platform_data *pdata = dummy_data;
	struct ramoops_context *cxt = &oops_cxt;
	size_t dump_mem_sz;
	phys_addr_t paddr;
	int err = -EINVAL;
	char kernelargs[512];

	if (IS_ENABLED(CONFIG_OFTREE) && !pdata) {
		pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			err = -ENOMEM;
			goto fail_out;
		}

		err = ramoops_parse_dt(dev, pdata);
		if (err < 0)
			goto fail_out;
	}

	/* Only a single ramoops area allowed at a time, so fail extra
	 * probes.
	 */
	if (cxt->max_dump_cnt)
		goto fail_out;

	if (!pdata->mem_size || (!pdata->record_size && !pdata->console_size &&
			!pdata->ftrace_size && !pdata->pmsg_size)) {
		pr_err("The memory size and the record/console size must be "
			"non-zero\n");
		goto fail_out;
	}

	if (pdata->record_size && !is_power_of_2(pdata->record_size))
		pdata->record_size = rounddown_pow_of_two(pdata->record_size);
	if (pdata->console_size && !is_power_of_2(pdata->console_size))
		pdata->console_size = rounddown_pow_of_two(pdata->console_size);
	if (pdata->ftrace_size && !is_power_of_2(pdata->ftrace_size))
		pdata->ftrace_size = rounddown_pow_of_two(pdata->ftrace_size);
	if (pdata->pmsg_size && !is_power_of_2(pdata->pmsg_size))
		pdata->pmsg_size = rounddown_pow_of_two(pdata->pmsg_size);

	cxt->size = pdata->mem_size;
	cxt->phys_addr = pdata->mem_address;
	cxt->memtype = pdata->mem_type;
	cxt->record_size = pdata->record_size;
	cxt->console_size = pdata->console_size;
	cxt->ftrace_size = pdata->ftrace_size;
	cxt->pmsg_size = pdata->pmsg_size;
	cxt->dump_oops = pdata->dump_oops;
	cxt->ecc_info = pdata->ecc_info;

	paddr = cxt->phys_addr;

	dump_mem_sz = cxt->size - cxt->console_size - cxt->ftrace_size
			- cxt->pmsg_size;
	err = ramoops_init_przs("dump", cxt, &cxt->przs, &paddr,
				dump_mem_sz, cxt->record_size,
				&cxt->max_dump_cnt, 0, 0);
	if (err)
		goto fail_out;

	err = ramoops_init_prz("console", cxt, &cxt->cprz, &paddr,
			       cxt->console_size, 0);
	if (err)
		goto fail_init_cprz;

	err = ramoops_init_prz("ftrace", cxt, &cxt->fprz, &paddr,
			       cxt->ftrace_size, 0);
	if (err)
		goto fail_init_fprz;

	err = ramoops_init_prz("pmsg", cxt, &cxt->mprz, &paddr,
			       cxt->pmsg_size, 0);
	if (err)
		goto fail_init_mprz;

	cxt->pstore.data = cxt;
	spin_lock_init(&cxt->pstore.buf_lock);

	err = pstore_register(&cxt->pstore);
	if (err) {
		pr_err("registering with pstore failed\n");
		goto fail_buf;
	}

	pr_info("attached 0x%lx@0x%llx, ecc: %d/%d\n",
		cxt->size, (unsigned long long)cxt->phys_addr,
		cxt->ecc_info.ecc_size, cxt->ecc_info.block_size);

	if (!IS_ENABLED(CONFIG_OFTREE)) {
		scnprintf(kernelargs, sizeof(kernelargs),
			  "ramoops.record_size=0x%zx "
			  "ramoops.console_size=0x%zx "
			  "ramoops.ftrace_size=0x%zx "
			  "ramoops.pmsg_size=0x%zx "
			  "ramoops.mem_address=0x%llx "
			  "ramoops.mem_size=0x%lx "
			  "ramoops.ecc=%d",
			  cxt->record_size,
			  cxt->console_size,
			  cxt->ftrace_size,
			  cxt->pmsg_size,
			  (unsigned long long)cxt->phys_addr,
			  mem_size,
			  ramoops_ecc);
		globalvar_add_simple("linux.bootargs.ramoops", kernelargs);
	} else {
		of_register_fixup(ramoops_of_fixup, pdata);
	}

	return 0;

fail_buf:
	kfree(cxt->mprz);
fail_init_mprz:
	kfree(cxt->fprz);
fail_init_fprz:
	kfree(cxt->cprz);
fail_init_cprz:
	ramoops_free_przs(cxt);
fail_out:
	kfree(pdata);
	return err;
}
unsigned long arm_mem_ramoops_get(void);

static void ramoops_register_dummy(void)
{
	dummy_data = kzalloc(sizeof(*dummy_data), GFP_KERNEL);
	if (!dummy_data) {
		pr_info("could not allocate pdata\n");
		return;
	}

	dummy_data->mem_size = mem_size;
	dummy_data->mem_address = arm_mem_ramoops_get();
	dummy_data->mem_type = 0;
	dummy_data->record_size = record_size;
	dummy_data->console_size = ramoops_console_size;
	dummy_data->ftrace_size = ramoops_ftrace_size;
	dummy_data->pmsg_size = ramoops_pmsg_size;
	dummy_data->dump_oops = dump_oops;
	/*
	 * For backwards compatibility ramoops.ecc=1 means 16 bytes ECC
	 * (using 1 byte for ECC isn't much of use anyway).
	 */
	dummy_data->ecc_info.ecc_size = ramoops_ecc == 1 ? 16 : ramoops_ecc;

	if (!IS_ENABLED(CONFIG_OFTREE))
		ramoops_probe(NULL);
}

static const struct of_device_id ramoops_dt_ids[] = {
	{ .compatible = "ramoops" },
	{ },
};
MODULE_DEVICE_TABLE(of, ramoops_dt_ids);

static struct driver ramoops_driver = {
	.name = "ramoops",
	.probe = ramoops_probe,
	.of_compatible = DRV_OF_COMPAT(ramoops_dt_ids),
};

static int __init ramoops_init(void)
{
	ramoops_register_dummy();
	return platform_driver_register(&ramoops_driver);
}
device_initcall(ramoops_init);
