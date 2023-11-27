// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015-2021, Linaro Limited
 * Copyright (c) 2016, EPAM Systems
 */

#define pr_fmt(fmt) "optee: smc_abi: " fmt

#include <linux/arm-smccc.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <of.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/tee_drv.h>
#include <linux/types.h>
#include "optee_private.h"
#include "optee_smc.h"

/*
 * This file implement the SMC ABI used when communicating with secure world
 * OP-TEE OS via raw SMCs.
 * This file is divided into the following sections:
 * 1. Convert between struct tee_param and struct optee_msg_param
 * 2. Low level support functions to register shared memory in secure world
 * 3. Do a normal scheduled call into secure world
 * 4. Driver initialization.
 */

/*
 * 1. Convert between struct tee_param and struct optee_msg_param
 *
 * optee_from_msg_param() and optee_to_msg_param() are the main
 * functions.
 */

static int from_msg_param_tmp_mem(struct tee_param *p, u32 attr,
				  const struct optee_msg_param *mp)
{
	struct tee_shm *shm;
	phys_addr_t pa;
	int rc;

	p->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT +
		  attr - OPTEE_MSG_ATTR_TYPE_TMEM_INPUT;
	p->u.memref.size = mp->u.tmem.size;
	shm = (struct tee_shm *)(unsigned long)mp->u.tmem.shm_ref;
	if (!shm) {
		p->u.memref.shm_offs = 0;
		p->u.memref.shm = NULL;
		return 0;
	}

	rc = tee_shm_get_pa(shm, 0, &pa);
	if (rc)
		return rc;

	p->u.memref.shm_offs = mp->u.tmem.buf_ptr - pa;
	p->u.memref.shm = shm;

	return 0;
}

static void from_msg_param_reg_mem(struct tee_param *p, u32 attr,
				   const struct optee_msg_param *mp)
{
	struct tee_shm *shm;

	p->attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT +
		  attr - OPTEE_MSG_ATTR_TYPE_RMEM_INPUT;
	p->u.memref.size = mp->u.rmem.size;
	shm = (struct tee_shm *)(unsigned long)mp->u.rmem.shm_ref;

	if (shm) {
		p->u.memref.shm_offs = mp->u.rmem.offs;
		p->u.memref.shm = shm;
	} else {
		p->u.memref.shm_offs = 0;
		p->u.memref.shm = NULL;
	}
}

/**
 * optee_from_msg_param() - convert from OPTEE_MSG parameters to
 *			    struct tee_param
 * @optee:	main service struct
 * @params:	subsystem internal parameter representation
 * @num_params:	number of elements in the parameter arrays
 * @msg_params:	OPTEE_MSG parameters
 * Returns 0 on success or <0 on failure
 */
static int optee_from_msg_param(struct optee *optee, struct tee_param *params,
				size_t num_params,
				const struct optee_msg_param *msg_params)
{
	int rc;
	size_t n;

	for (n = 0; n < num_params; n++) {
		struct tee_param *p = params + n;
		const struct optee_msg_param *mp = msg_params + n;
		u32 attr = mp->attr & OPTEE_MSG_ATTR_TYPE_MASK;

		switch (attr) {
		case OPTEE_MSG_ATTR_TYPE_NONE:
			p->attr = TEE_IOCTL_PARAM_ATTR_TYPE_NONE;
			memset(&p->u, 0, sizeof(p->u));
			break;
		case OPTEE_MSG_ATTR_TYPE_VALUE_INPUT:
		case OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT:
		case OPTEE_MSG_ATTR_TYPE_VALUE_INOUT:
			optee_from_msg_param_value(p, attr, mp);
			break;
		case OPTEE_MSG_ATTR_TYPE_TMEM_INPUT:
		case OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT:
		case OPTEE_MSG_ATTR_TYPE_TMEM_INOUT:
			rc = from_msg_param_tmp_mem(p, attr, mp);
			if (rc)
				return rc;
			break;
		case OPTEE_MSG_ATTR_TYPE_RMEM_INPUT:
		case OPTEE_MSG_ATTR_TYPE_RMEM_OUTPUT:
		case OPTEE_MSG_ATTR_TYPE_RMEM_INOUT:
			from_msg_param_reg_mem(p, attr, mp);
			break;

		default:
			return -EINVAL;
		}
	}
	return 0;
}

static int to_msg_param_reg_mem(struct optee_msg_param *mp,
				const struct tee_param *p)
{
	mp->attr = OPTEE_MSG_ATTR_TYPE_RMEM_INPUT + p->attr -
		   TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;

	mp->u.rmem.shm_ref = (unsigned long)p->u.memref.shm;
	mp->u.rmem.size = p->u.memref.size;
	mp->u.rmem.offs = p->u.memref.shm_offs;
	return 0;
}

static int to_msg_param_tmp_mem(struct optee_msg_param *mp,
				const struct tee_param *p)
{
	int rc;
	phys_addr_t pa;

	mp->attr = OPTEE_MSG_ATTR_TYPE_TMEM_INPUT + p->attr -
		   TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT;

	mp->u.tmem.shm_ref = (unsigned long)p->u.memref.shm;
	mp->u.tmem.size = p->u.memref.size;

	if (!p->u.memref.shm) {
		mp->u.tmem.buf_ptr = 0;
		return 0;
	}

	rc = tee_shm_get_pa(p->u.memref.shm, p->u.memref.shm_offs, &pa);
	if (rc)
		return rc;

	mp->u.tmem.buf_ptr = pa;
	mp->attr |= OPTEE_MSG_ATTR_CACHE_PREDEFINED <<
		    OPTEE_MSG_ATTR_CACHE_SHIFT;

	return 0;
}

/**
 * optee_to_msg_param() - convert from struct tee_params to OPTEE_MSG parameters
 * @optee:	main service struct
 * @msg_params:	OPTEE_MSG parameters
 * @num_params:	number of elements in the parameter arrays
 * @params:	subsystem internal parameter representation
 * Returns 0 on success or <0 on failure
 */
static int optee_to_msg_param(struct optee *optee,
			      struct optee_msg_param *msg_params,
			      size_t num_params, const struct tee_param *params)
{
	int rc;
	size_t n;

	for (n = 0; n < num_params; n++) {
		const struct tee_param *p = params + n;
		struct optee_msg_param *mp = msg_params + n;

		switch (p->attr) {
		case TEE_IOCTL_PARAM_ATTR_TYPE_NONE:
			mp->attr = TEE_IOCTL_PARAM_ATTR_TYPE_NONE;
			memset(&mp->u, 0, sizeof(mp->u));
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT:
			optee_to_msg_param_value(mp, p);
			break;
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT:
		case TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT:
			if (tee_shm_is_dynamic(p->u.memref.shm))
				rc = to_msg_param_reg_mem(mp, p);
			else
				rc = to_msg_param_tmp_mem(mp, p);
			if (rc)
				return rc;
			break;
		default:
			return -EINVAL;
		}
	}
	return 0;
}

/*
 * 2. Low level support functions to register shared memory in secure world
 *
 * Functions to enable/disable shared memory caching in secure world, that
 * is, lazy freeing of previously allocated shared memory. Freeing is
 * performed when a request has been compled.
 */

#define PAGELIST_ENTRIES_PER_PAGE				\
	((OPTEE_MSG_NONCONTIG_PAGE_SIZE / sizeof(u64)) - 1)

/**
 * optee_alloc_and_init_page_list() - Provide page list of memory buffer
 * @buf:               Start of buffer
 * @len:               Length of buffer
 * @phys_buf_ptr       Physical pointer with coded offset to page list
 *
 * Secure world doesn't share mapping with Normal world (barebox in this case)
 * so physical pointers are needed when sharing pointers.
 *
 * Returns a pointer page list on success or NULL on failure
 */
static void *optee_alloc_and_init_page_list(void *buf, unsigned long len, u64 *phys_buf_ptr)
{
       const unsigned int page_size = OPTEE_MSG_NONCONTIG_PAGE_SIZE;
       const phys_addr_t page_mask = page_size - 1;
       u8 *buf_base;
       unsigned int page_offset;
       unsigned int num_pages;
       unsigned int list_size;
       unsigned int n;
       void *page_list;
       struct {
               u64 pages_list[PAGELIST_ENTRIES_PER_PAGE];
               u64 next_page_data;
       } *pages_data;

       /*
        * A Memory buffer is described in chunks of 4k. The list of
        * physical addresses has to be represented by a physical pointer
        * too and a single list has to start at a 4k page and fit into
        * that page. In order to be able to describe large memory buffers
        * these 4k pages carrying physical addresses are linked together
        * in a list. See OPTEE_MSG_ATTR_NONCONTIG in
        * drivers/tee/optee/optee_msg.h for more information.
        */

       page_offset = (unsigned long)buf & page_mask;
       num_pages = roundup(page_offset + len, page_size) / page_size;
       list_size = DIV_ROUND_UP(num_pages, PAGELIST_ENTRIES_PER_PAGE) *
                   page_size;
       page_list = memalign(page_size, list_size);
       if (!page_list)
               return NULL;

       pages_data = page_list;
       buf_base = (u8 *)rounddown((unsigned long)buf, page_size);
       n = 0;
       while (num_pages) {
               pages_data->pages_list[n] = virt_to_phys(buf_base);
               n++;
               buf_base += page_size;
               num_pages--;

               if (n == PAGELIST_ENTRIES_PER_PAGE) {
                       pages_data->next_page_data =
                               virt_to_phys(pages_data + 1);
                       pages_data++;
                       n = 0;
               }
       }

       *phys_buf_ptr = virt_to_phys(page_list) | page_offset;
       return page_list;
}

static int optee_shm_register(struct tee_context *ctx, struct tee_shm *shm)
{
	struct optee *optee = tee_get_drvdata(ctx->teedev);
	struct optee_msg_arg *msg_arg;
	struct tee_shm *shm_arg;
	u64 *pages_list;
	u64 ph_ptr;
	int rc = 0;

	pages_list = optee_alloc_and_init_page_list(shm->kaddr, shm->size, &ph_ptr);
	if (!pages_list)
		return -ENOMEM;

	/*
	 * We don't use a cache for shared memory allocation like Linux,
	 * so it's safe to directly call optee_get_msg_arg here.
	 */
	msg_arg = optee_get_msg_arg(ctx, 1, &shm_arg);
	if (IS_ERR(msg_arg)) {
		rc = PTR_ERR(msg_arg);
		goto free_pages_list;
	}

	msg_arg->num_params = 1;
	msg_arg->cmd = OPTEE_MSG_CMD_REGISTER_SHM;
	msg_arg->params->attr = OPTEE_MSG_ATTR_TYPE_TMEM_OUTPUT |
				OPTEE_MSG_ATTR_NONCONTIG;
	msg_arg->params->u.tmem.buf_ptr = ph_ptr;
	msg_arg->params->u.tmem.shm_ref = (unsigned long)shm;
	msg_arg->params->u.tmem.size = tee_shm_get_size(shm);

	if (optee->ops->do_call_with_arg(ctx, msg_arg) ||
	    msg_arg->ret != TEEC_SUCCESS)
		rc = -EINVAL;

	optee_free_msg_arg(ctx, shm_arg);
free_pages_list:
	free(pages_list);

	return rc;
}

static int optee_shm_unregister(struct tee_context *ctx, struct tee_shm *shm)
{
	struct optee *optee = tee_get_drvdata(ctx->teedev);
	struct optee_msg_arg *msg_arg;
	struct tee_shm *shm_arg;
	int rc = 0;

	/*
	 * We don't use a cache for shared memory allocation like Linux,
	 * so it's safe to directly call optee_get_msg_arg here.
	 */
	msg_arg = optee_get_msg_arg(ctx, 1, &shm_arg);
	if (IS_ERR(msg_arg))
		return PTR_ERR(msg_arg);

	msg_arg->num_params = 1;
	msg_arg->cmd = OPTEE_MSG_CMD_UNREGISTER_SHM;
	msg_arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_RMEM_INPUT;
	msg_arg->params[0].u.rmem.shm_ref = (unsigned long)shm;

	if (optee->ops->do_call_with_arg(ctx, msg_arg) ||
	    msg_arg->ret != TEEC_SUCCESS)
		rc = -EINVAL;

	optee_free_msg_arg(ctx, shm_arg);
	return rc;
}

/*
 * 3. Do a normal scheduled call into secure world
 *
 * The function optee_smc_do_call_with_arg() performs a normal scheduled
 * call into secure world. During this call may normal world request help
 * from normal world using RPCs, Remote Procedure Calls. This includes
 * delivery of non-secure interrupts to for instance allow rescheduling of
 * the current task.
 */


/**
 * optee_handle_rpc() - handle RPC from secure world
 * @ctx:	context doing the RPC
 * @param:	value of registers for the RPC
 * @call_ctx:	call context. Preserved during one OP-TEE invocation
 *
 * Result of RPC is written back into @param.
 */
static void optee_handle_rpc(struct tee_context *ctx, struct optee_rpc_param *param,
			     void *page_list)
{
	struct tee_device *teedev = ctx->teedev;
	struct optee *optee = tee_get_drvdata(teedev);
	struct optee_msg_arg *arg;
	struct tee_shm *shm;
	phys_addr_t pa;

	switch (OPTEE_SMC_RETURN_GET_RPC_FUNC(param->a0)) {
	case OPTEE_SMC_RPC_FUNC_ALLOC:
		shm = tee_shm_alloc_priv_buf(optee->ctx, param->a1);
		if (!IS_ERR(shm) && !tee_shm_get_pa(shm, 0, &pa)) {
			reg_pair_from_64(&param->a1, &param->a2, pa);
			/* "cookie" */
			reg_pair_from_64(&param->a4, &param->a5,
					 (unsigned long)shm);
		} else {
			param->a1 = 0;
			param->a2 = 0;
			param->a4 = 0;
			param->a5 = 0;
		}
		break;
	case OPTEE_SMC_RPC_FUNC_FREE:
		shm = reg_pair_to_ptr(param->a1, param->a2);
		tee_shm_free(shm);
		break;
	case OPTEE_SMC_RPC_FUNC_FOREIGN_INTR:
		break;
	case OPTEE_SMC_RPC_FUNC_CMD:
		shm = reg_pair_to_ptr(param->a1, param->a2);
		arg = tee_shm_get_va(shm, 0);
		if (IS_ERR(arg)) {
			pr_err("%s: tee_shm_get_va %p failed\n",
			       __func__, shm);
			break;
		}

		optee_rpc_cmd(ctx, optee, arg);
		break;
	default:
		pr_warn("Unknown RPC func 0x%x\n",
			(u32)OPTEE_SMC_RETURN_GET_RPC_FUNC(param->a0));
		break;
	}

	param->a0 = OPTEE_SMC_CALL_RETURN_FROM_RPC;
}

/**
 * optee_smc_do_call_with_arg() - Do an SMC to OP-TEE in secure world
 * @ctx:	calling context
 * @arg:	argument to pass to secure world
 *
 * Does and SMC to OP-TEE in secure world and handles eventual resulting
 * Remote Procedure Calls (RPC) from OP-TEE.
 *
 * Returns return code from secure world, 0 is OK
 */
static int optee_smc_do_call_with_arg(struct tee_context *ctx,
				      struct optee_msg_arg *arg)
{
	struct optee *optee = tee_get_drvdata(ctx->teedev);
	struct optee_rpc_param param = { .a0 = OPTEE_SMC_CALL_WITH_ARG };
	void *page_list = NULL;

	reg_pair_from_64(&param.a1, &param.a2, virt_to_phys(arg));
	while (true) {
		struct arm_smccc_res res;

		/* MMU will always be enabled at this moment and with matching caching
		 * attributes, we need not worry about flushing
		 */

		optee->smc.invoke_fn(param.a0, param.a1, param.a2, param.a3,
				     param.a4, param.a5, param.a6, param.a7, &res);

		free(page_list);
		page_list = NULL;

		if (OPTEE_SMC_RETURN_IS_RPC(res.a0)) {
			param.a0 = res.a0;
			param.a1 = res.a1;
			param.a2 = res.a2;
			param.a3 = res.a3;
			optee_handle_rpc(ctx, &param, &page_list);
		} else {
			return res.a0;
		}
	}
}

/*
 * 4. Driver initialization
 *
 * During driver initialization is secure world probed to find out which
 * features it supports so the driver can be initialized with a matching
 * configuration. This involves for instance support for dynamic shared
 * memory instead of a static memory carvout.
 */

static void optee_get_version(struct tee_device *teedev,
			      struct tee_ioctl_version_data *vers)
{
	struct tee_ioctl_version_data v = {
		.impl_id = TEE_IMPL_ID_OPTEE,
		.impl_caps = TEE_OPTEE_CAP_TZ,
		.gen_caps = TEE_GEN_CAP_GP,
	};
	struct optee *optee = tee_get_drvdata(teedev);

	if (optee->smc.sec_caps & OPTEE_SMC_SEC_CAP_DYNAMIC_SHM)
		v.gen_caps |= TEE_GEN_CAP_REG_MEM;
	if (optee->smc.sec_caps & OPTEE_SMC_SEC_CAP_MEMREF_NULL)
		v.gen_caps |= TEE_GEN_CAP_MEMREF_NULL;
	*vers = v;
}

static int optee_smc_open(struct tee_context *ctx)
{
	struct optee *optee = tee_get_drvdata(ctx->teedev);
	u32 sec_caps = optee->smc.sec_caps;

	return optee_open(ctx, sec_caps & OPTEE_SMC_SEC_CAP_MEMREF_NULL);
}

static const struct tee_driver_ops optee_clnt_ops = {
	.get_version = optee_get_version,
	.open = optee_smc_open,
	.release = optee_release,
	.open_session = optee_open_session,
	.close_session = optee_close_session,
	.invoke_func = optee_invoke_func,
	.shm_register = optee_shm_register,
	.shm_unregister = optee_shm_unregister,
};

static const struct tee_desc optee_clnt_desc = {
	.name = DRIVER_NAME "-clnt",
	.ops = &optee_clnt_ops,
	.owner = THIS_MODULE,
};

static const struct optee_ops optee_ops = {
	.do_call_with_arg = optee_smc_do_call_with_arg,
	.to_msg_param = optee_to_msg_param,
	.from_msg_param = optee_from_msg_param,
};

static bool optee_msg_api_uid_is_optee_api(optee_invoke_fn *invoke_fn)
{
	struct arm_smccc_res res;

	invoke_fn(OPTEE_SMC_CALLS_UID, 0, 0, 0, 0, 0, 0, 0, &res);

	if (res.a0 == OPTEE_MSG_UID_0 && res.a1 == OPTEE_MSG_UID_1 &&
	    res.a2 == OPTEE_MSG_UID_2 && res.a3 == OPTEE_MSG_UID_3)
		return true;
	return false;
}

static void optee_msg_get_os_revision(optee_invoke_fn *invoke_fn)
{
	union {
		struct arm_smccc_res smccc;
		struct optee_smc_call_get_os_revision_result result;
	} res = {
		.result = {
			.build_id = 0
		}
	};

	invoke_fn(OPTEE_SMC_CALL_GET_OS_REVISION, 0, 0, 0, 0, 0, 0, 0,
		  &res.smccc);

	if (res.result.build_id)
		pr_info("revision %lu.%lu (%08lx)\n", res.result.major,
			res.result.minor, res.result.build_id);
	else
		pr_info("revision %lu.%lu\n", res.result.major, res.result.minor);
}

static bool optee_msg_api_revision_is_compatible(optee_invoke_fn *invoke_fn)
{
	union {
		struct arm_smccc_res smccc;
		struct optee_smc_calls_revision_result result;
	} res;

	invoke_fn(OPTEE_SMC_CALLS_REVISION, 0, 0, 0, 0, 0, 0, 0, &res.smccc);

	if (res.result.major == OPTEE_MSG_REVISION_MAJOR &&
	    (int)res.result.minor >= OPTEE_MSG_REVISION_MINOR)
		return true;
	return false;
}

static bool optee_msg_exchange_capabilities(optee_invoke_fn *invoke_fn,
					    u32 *sec_caps)
{
	union {
		struct arm_smccc_res smccc;
		struct optee_smc_exchange_capabilities_result result;
	} res;

	invoke_fn(OPTEE_SMC_EXCHANGE_CAPABILITIES,
		  OPTEE_SMC_NSEC_CAP_UNIPROCESSOR, 0, 0, 0, 0, 0, 0,
		  &res.smccc);

	if (res.result.status != OPTEE_SMC_RETURN_OK)
		return false;

	*sec_caps = res.result.capabilities;

	return true;
}

/* Simple wrapper functions to be able to use a function pointer */
static void optee_smccc_smc(unsigned long a0, unsigned long a1,
			    unsigned long a2, unsigned long a3,
			    unsigned long a4, unsigned long a5,
			    unsigned long a6, unsigned long a7,
			    struct arm_smccc_res *res)
{
	arm_smccc_smc(a0, a1, a2, a3, a4, a5, a6, a7, res);
}

static void optee_smccc_hvc(unsigned long a0, unsigned long a1,
			    unsigned long a2, unsigned long a3,
			    unsigned long a4, unsigned long a5,
			    unsigned long a6, unsigned long a7,
			    struct arm_smccc_res *res)
{
	arm_smccc_hvc(a0, a1, a2, a3, a4, a5, a6, a7, res);
}

static optee_invoke_fn *get_invoke_func(struct device *dev)
{
	const char *method;

	pr_info("probing for conduit method.\n");

	if (of_property_read_string(dev->of_node, "method", &method)) {
		pr_warn("missing \"method\" property\n");
		return ERR_PTR(-ENXIO);
	}

	if (!strcmp("hvc", method))
		return optee_smccc_hvc;
	else if (!strcmp("smc", method))
		return optee_smccc_smc;

	pr_warn("invalid \"method\" property: %s\n", method);
	return ERR_PTR(-EINVAL);
}

static int optee_probe(struct device *dev)
{
	optee_invoke_fn *invoke_fn;
	struct optee *optee = NULL;
	struct tee_device *teedev;
	struct tee_context *ctx;
	u32 sec_caps;
	int rc;

	invoke_fn = get_invoke_func(dev);
	if (IS_ERR(invoke_fn))
		return PTR_ERR(invoke_fn);

	if (!optee_msg_api_uid_is_optee_api(invoke_fn)) {
		pr_warn("api uid mismatch\n");
		return -EINVAL;
	}

	optee_msg_get_os_revision(invoke_fn);

	if (!optee_msg_api_revision_is_compatible(invoke_fn)) {
		pr_warn("api revision mismatch\n");
		return -EINVAL;
	}

	if (!optee_msg_exchange_capabilities(invoke_fn, &sec_caps)) {
		pr_warn("capabilities mismatch\n");
		return -EINVAL;
	}

	/*
	 * OP-TEE can use both shared memory via predefined pool or as
	 * dynamic shared memory provided by normal world. To keep things
	 * simple we're only using dynamic shared memory in this driver.
	 */
	if (!(sec_caps & OPTEE_SMC_SEC_CAP_DYNAMIC_SHM)) {
		pr_err("driver requires OP-TEE dynamic shared memory support\n");
		return -ENOSYS;
	}

	optee = kzalloc(sizeof(*optee), GFP_KERNEL);
	if (!optee)
		return -ENOMEM;

	optee->ops = &optee_ops;
	optee->smc.invoke_fn = invoke_fn;
	optee->smc.sec_caps = sec_caps;

	teedev = tee_device_alloc(&optee_clnt_desc, dev, optee);
	if (IS_ERR(teedev)) {
		rc = PTR_ERR(teedev);
		goto err_free_optee;
	}
	optee->teedev = teedev;

	rc = tee_device_register(optee->teedev);
	if (rc)
		goto err_release_teedev;

	ctx = teedev_open(optee->teedev);
	if (IS_ERR(ctx)) {
		rc = PTR_ERR(ctx);
		goto err_unreg_teedev;
	}
	optee->ctx = ctx;
	if (rc)
		goto err_close_ctx;

	rc = optee_enumerate_devices(PTA_CMD_GET_DEVICES);
	if (!rc)
		rc = optee_enumerate_devices(PTA_CMD_GET_DEVICES_SUPP);
	if (rc)
		goto err_optee_unregister_devices;

	pr_debug("initialized driver with dynamic shared memory\n");
	return 0;

err_optee_unregister_devices:
	optee_unregister_devices();
err_close_ctx:
	teedev_close_context(ctx);
err_unreg_teedev:
	tee_device_unregister(optee->teedev);
err_release_teedev:
	tee_device_release(optee->teedev);
err_free_optee:
	kfree(optee);
	return rc;
}

static const struct of_device_id optee_dt_match[] = {
	{ .compatible = "linaro,optee-tz" },
	{},
};
MODULE_DEVICE_TABLE(of, optee_dt_match);

static struct driver optee_driver = {
	.probe  = optee_probe,
	.name = "optee",
	.of_match_table = optee_dt_match,
};

int optee_smc_abi_register(void)
{
	return platform_driver_register(&optee_driver);
}
