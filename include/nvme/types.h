// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libnvme.
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors: Keith Busch <keith.busch@wdc.com>
 *          Chaitanya Kulkarni <chaitanya.kulkarni@wdc.com>
 */
#ifndef _LIBNVME_H
#define _LIBNVME_H

enum nvme_sanitize_sanact {
	NVME_SANITIZE_SANACT_EXIT_FAILURE                       = 1,
	NVME_SANITIZE_SANACT_START_BLOCK_ERASE                  = 2,
	NVME_SANITIZE_SANACT_START_OVERWRITE                    = 3,
	NVME_SANITIZE_SANACT_START_CRYPTO_ERASE                 = 4,
	NVME_SANITIZE_SANACT_EXIT_MEDIA_VERIF                   = 5,
};

struct nvme_sanitize_log_page {
	__le16  sprog;
	__le16  sstat;
	__le32  scdw10;
	__le32  eto;
	__le32  etbe;
	__le32  etce;
	__le32  etond;
	__le32  etbend;
	__le32  etcend;
	__le32  etpvds;
	__u8    ssi;
	__u8    rsvd37[475];
};

enum nvme_cmd_get_log_lid {
	NVME_LOG_LID_SANITIZE					= 0x81,
};
#endif /* _LIBNVME_TYPES_H */
