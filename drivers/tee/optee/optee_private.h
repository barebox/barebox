/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2015-2021, Linaro Limited
 */

#ifndef OPTEE_PRIVATE_H
#define OPTEE_PRIVATE_H

#include <linux/arm-smccc.h>
#include <linux/tee_drv.h>
#include <linux/types.h>
#include "optee_msg.h"

#define DRIVER_NAME "optee"

#define OPTEE_MAX_ARG_SIZE	1024

/* Some Global Platform error codes used in this driver */
#define TEEC_SUCCESS			0x00000000
#define TEEC_ERROR_BAD_PARAMETERS	0xFFFF0006
#define TEEC_ERROR_NOT_SUPPORTED	0xFFFF000A
#define TEEC_ERROR_COMMUNICATION	0xFFFF000E
#define TEEC_ERROR_OUT_OF_MEMORY	0xFFFF000C
#define TEEC_ERROR_BUSY			0xFFFF000D
#define TEEC_ERROR_SHORT_BUFFER		0xFFFF0010

#define TEEC_ORIGIN_COMMS		0x00000002

/*
 * This value should be larger than the number threads in secure world to
 * meet the need from secure world. The number of threads in secure world
 * are usually not even close to 255 so we should be safe for now.
 */
#define OPTEE_DEFAULT_MAX_NOTIF_VALUE	255

typedef void (optee_invoke_fn)(unsigned long, unsigned long, unsigned long,
				unsigned long, unsigned long, unsigned long,
				unsigned long, unsigned long,
				struct arm_smccc_res *);


/*
 * struct optee_smc - optee smc communication struct
 * @invoke_fn		handler function to invoke secure monitor
 * @sec_caps:		secure world capabilities defined by
 *			OPTEE_SMC_SEC_CAP_* in optee_smc.h
 */
struct optee_smc {
	optee_invoke_fn *invoke_fn;
	u32 sec_caps;
};

struct optee;

/**
 * struct optee_ops - OP-TEE driver internal operations
 * @do_call_with_arg:	enters OP-TEE in secure world
 * @to_msg_param:	converts from struct tee_param to OPTEE_MSG parameters
 * @from_msg_param:	converts from OPTEE_MSG parameters to struct tee_param
 *
 * These OPs are only supposed to be used internally in the OP-TEE driver
 * as a way of abstracting the different methogs of entering OP-TEE in
 * secure world.
 */
struct optee_ops {
	int (*do_call_with_arg)(struct tee_context *ctx,
				struct optee_msg_arg *arg);
	int (*to_msg_param)(struct optee *optee,
			    struct optee_msg_param *msg_params,
			    size_t num_params, const struct tee_param *params);
	int (*from_msg_param)(struct optee *optee, struct tee_param *params,
			      size_t num_params,
			      const struct optee_msg_param *msg_params);
};

/**
 * struct optee - main service struct
 * @teedev:		client device
 * @ops:		internal callbacks for different ways to reach secure
 *			world
 * @ctx:		driver internal TEE context
 * @smc:		specific to SMC ABI
 */
struct optee {
	struct tee_device *teedev;
	const struct optee_ops *ops;
	struct tee_context *ctx;
	union {
		struct optee_smc smc;
	};
};

struct optee_session {
	struct list_head list_node;
	u32 session_id;
};

struct optee_context_data {
	/* Serializes access to this struct */
	struct list_head sess_list;
};

struct optee_rpc_param {
	u32	a0;
	u32	a1;
	u32	a2;
	u32	a3;
	u32	a4;
	u32	a5;
	u32	a6;
	u32	a7;
};

int optee_open_session(struct tee_context *ctx,
		       struct tee_ioctl_open_session_arg *arg,
		       struct tee_param *param);
int optee_close_session_helper(struct tee_context *ctx, u32 session);
int optee_close_session(struct tee_context *ctx, u32 session);
int optee_invoke_func(struct tee_context *ctx, struct tee_ioctl_invoke_arg *arg,
		      struct tee_param *param);

#define PTA_CMD_GET_DEVICES		0x0
#define PTA_CMD_GET_DEVICES_SUPP	0x1
int optee_enumerate_devices(u32 func);
void optee_unregister_devices(void);

int optee_open(struct tee_context *ctx, bool cap_memref_null);
void optee_release(struct tee_context *ctx);

static inline void optee_from_msg_param_value(struct tee_param *p, u32 attr,
					      const struct optee_msg_param *mp)
{
	p->attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT +
		  attr - OPTEE_MSG_ATTR_TYPE_VALUE_INPUT;
	p->u.value.a = mp->u.value.a;
	p->u.value.b = mp->u.value.b;
	p->u.value.c = mp->u.value.c;
}

static inline void optee_to_msg_param_value(struct optee_msg_param *mp,
					    const struct tee_param *p)
{
	mp->attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT + p->attr -
		   TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	mp->u.value.a = p->u.value.a;
	mp->u.value.b = p->u.value.b;
	mp->u.value.c = p->u.value.c;
}

struct optee_msg_arg *optee_get_msg_arg(struct tee_context *ctx,
					size_t num_params,
					struct tee_shm **shm_ret);
void optee_free_msg_arg(struct tee_context *ctx,
			struct tee_shm *shm);
size_t optee_msg_arg_size(void);


void optee_rpc_cmd(struct tee_context *ctx, struct optee *optee,
		   struct optee_msg_arg *arg);

/*
 * Small helpers
 */

static inline void *reg_pair_to_ptr(u32 reg0, u32 reg1)
{
	return (void *)(unsigned long)(((u64)reg0 << 32) | reg1);
}

static inline void reg_pair_from_64(u32 *reg0, u32 *reg1, u64 val)
{
	*reg0 = val >> 32;
	*reg1 = val;
}

/* Registration of the ABIs */
int optee_smc_abi_register(void);

#endif /*OPTEE_PRIVATE_H*/
