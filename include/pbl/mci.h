/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __PBL_MCI_H__
#define __PBL_MCI_H__

#include <linux/types.h>
#include <mci.h>

struct mci_cmd;
struct mci_data;
struct pbl_bio;

enum pbl_mci_capacity {
	PBL_MCI_UNKNOWN_CAPACITY,
	PBL_MCI_STANDARD_CAPACITY,
	PBL_MCI_HIGH_CAPACITY, /* and extended */
	PBL_MCI_ULTRA_CAPACITY,
	PBL_MCI_RESERVED_CAPACITY,
};

struct pbl_mci {
	void *priv;
	enum pbl_mci_capacity capacity;
	unsigned max_blocks_per_read;
	int (*send_cmd)(struct pbl_mci *mci, struct mci_cmd *cmd,
			struct mci_data *data);
};

static inline int pbl_mci_send_cmd(struct pbl_mci *mci,
				   struct mci_cmd *cmd,
				   struct mci_data *data)
{
	return mci->send_cmd(mci, cmd, data);
}

int pbl_mci_bio_init(struct pbl_mci *mci, struct pbl_bio *bio);

#endif /* __PBL_MCI_H__ */
