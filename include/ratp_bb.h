#ifndef __RATP_BB_H
#define __RATP_BB_H

struct ratp_bb_pkt {
	struct list_head list;

	unsigned int len;
	uint8_t data[];
};

int barebox_ratp(struct console_device *cdev);
int barebox_ratp_fs_call(struct ratp_bb_pkt *tx, struct ratp_bb_pkt **rx);
int barebox_ratp_fs_mount(const char *path);

#endif /* __RATP_BB_H */
