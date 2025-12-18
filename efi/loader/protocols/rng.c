// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/58da850c87240c397242ebd47341c69aa6278119/lib/efi_loader/efi_rng.c
/*
 * Copyright (c) 2019, Linaro Limited
 */

#include <linux/hw_random.h>
#include <stdlib.h>
#include <efi/loader.h>
#include <efi/protocol/rng.h>
#include <efi/devicepath.h>
#include <efi/loader/devicepath.h>
#include <efi/loader/object.h>
#include <efi/loader/trace.h>
#include <efi/guid.h>
#include <efi/error.h>
#include <xfuncs.h>

struct efi_rng_obj {
	struct efi_object header;
	struct hwrng *hwrng;
	struct efi_rng_protocol ops;
};

#define to_efi_rng_obj(this) container_of(this, struct efi_rng_obj, ops)

/**
 * rng_getinfo() - get information about random number generation
 *
 * This function implement the GetInfo() service of the EFI random number
 * generator protocol. See the UEFI spec for details.
 *
 * @this:			random number generator protocol instance
 * @rng_algorithm_list_size:	number of random number generation algorithms
 * @rng_algorithm_list:		descriptions of random number generation
 *				algorithms
 * Return:			status code
 */
static efi_status_t EFIAPI rng_getinfo(struct efi_rng_protocol *this,
				       efi_uintn_t *rng_algorithm_list_size,
				       efi_guid_t *rng_algorithm_list)
{
	efi_status_t ret = EFI_SUCCESS;
	efi_guid_t rng_algo_guid = EFI_RNG_ALGORITHM_RAW;

	EFI_ENTRY("%p, %p, %p", this, rng_algorithm_list_size,
		  rng_algorithm_list);

	if (!this || !rng_algorithm_list_size) {
		ret = EFI_INVALID_PARAMETER;
		goto back;
	}

	if (!rng_algorithm_list ||
	    *rng_algorithm_list_size < sizeof(*rng_algorithm_list)) {
		*rng_algorithm_list_size = sizeof(*rng_algorithm_list);
		ret = EFI_BUFFER_TOO_SMALL;
		goto back;
	}

	/*
	 * For now, use EFI_RNG_ALGORITHM_RAW as the default
	 * algorithm. If a new algorithm gets added in the
	 * future through a Kconfig, rng_algo_guid will be set
	 * based on that Kconfig option
	 */
	*rng_algorithm_list_size = sizeof(*rng_algorithm_list);
	*rng_algorithm_list = rng_algo_guid;

back:
	return EFI_EXIT(ret);
}

/**
 * rng_getval() - get random value
 *
 * This function implement the GetRng() service of the EFI random number
 * generator protocol. See the UEFI spec for details.
 *
 * @this:		random number generator protocol instance
 * @rng_algorithm:	random number generation algorithm
 * @rng_value_length:	number of random bytes to generate, buffer length
 * @rng_value:		buffer to receive random bytes
 * Return:		status code
 */
static efi_status_t EFIAPI rng_getval(struct efi_rng_protocol *this,
				  efi_guid_t *rng_algorithm,
				  efi_uintn_t rng_value_length,
				  uint8_t *rng_value)
{
	int ret;
	efi_status_t status = EFI_SUCCESS;
	const efi_guid_t rng_raw_guid = EFI_RNG_ALGORITHM_RAW;
	struct efi_rng_obj *rngobj = to_efi_rng_obj(this);

	EFI_ENTRY("%p, %p, %zu, %p", this, rng_algorithm, rng_value_length,
		  rng_value);

	if (!this || !rng_value || !rng_value_length) {
		status = EFI_INVALID_PARAMETER;
		goto back;
	}

	if (rng_algorithm) {
		EFI_PRINT("RNG algorithm %pUl\n", rng_algorithm);
		if (efi_guidcmp(*rng_algorithm, rng_raw_guid)) {
			status = EFI_UNSUPPORTED;
			goto back;
		}
	}

	ret = hwrng_get_crypto_bytes(rngobj->hwrng, rng_value, rng_value_length);
	if (ret < 0) {
		EFI_PRINT("Rng device read failed\n");
		status = EFI_DEVICE_ERROR;
		goto back;
	}

back:
	return EFI_EXIT(status);
}

/**
 * efi_rng_register() - register EFI_RNG_PROTOCOL
 *
 * If a HWRNG device is available, the Random Number Generator Protocol is
 * registered.
 *
 * Return:	An EFI error status
 */
static efi_status_t efi_rng_register(void *data)
{
	struct efi_rng_obj *rngobj;
	struct hwrng *rng;
	struct efi_device_path *dp;
	efi_handle_t handle;

	rng = hwrng_get_first();
	if (IS_ERR(rng))
		return EFI_UNSUPPORTED;

	rngobj = xzalloc(sizeof(*rngobj));

	rngobj->hwrng = rng;
	rngobj->ops.get_info = rng_getinfo;
	rngobj->ops.get_rng = rng_getval;

	dp = efi_dp_from_cdev(&rng->cdev, true);

	efi_add_handle(&rngobj->header);

	handle = &rngobj->header;

	return efi_install_multiple_protocol_interfaces(&handle,
							&efi_device_path_protocol_guid, dp,
							&efi_rng_protocol_guid, &rngobj->ops,
							NULL);
}

static int efi_rng_init(void)
{
	efi_register_deferred_init(efi_rng_register, NULL);
	return 0;
}
device_initcall(efi_rng_init);
