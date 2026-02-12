/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2025 Pengutronix */

#ifndef HGS_EFI_H
#define HGS_EFI_H

#include <errno.h>
#include <linux/types.h>

enum hgs_sep_msg_type {
	HGS_SEP_MSG_TYPE_COMMAND,
	HGS_SEP_MSG_TYPE_EVENT,
	HGS_SEP_MSG_TYPE_REPLY,
};

struct hgs_sep_cmd {
	enum hgs_sep_msg_type type;
	uint16_t msg_id;
	void *payload;
	size_t payload_size;
	void *reply_data;
	size_t reply_data_size;
};

struct hgs_efi;

#if defined(CONFIG_MFD_HGS_EFI)

int hgs_efi_exec(struct hgs_efi *efi, struct hgs_sep_cmd *cmd);
char *hgs_efi_extract_str_response(u8 *buf);

#else

static inline int hgs_efi_exec(struct hgs_efi *efi, struct hgs_sep_cmd *cmd)
{
	return -ENOTSUPP;
}

static inline char *hgs_efi_extract_str_response(u8 *buf)
{
	return ERR_PTR(-ENOTSUPP);
}

#endif /* CONFIG_MFD_HGS_EFI */

#endif /* HGS_EFI_H */
