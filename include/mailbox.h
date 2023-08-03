#ifndef __MAILBOX_H
#define __MAILBOX_H

struct mbox_chan {
	struct mbox_controller *mbox;
	struct device *dev;
	void *con_priv;
};

/**
 * struct mbox_ops - The functions that a mailbox driver must implement.
 */
struct mbox_chan_ops {
	int (*request)(struct mbox_chan *chan);
	int (*rfree)(struct mbox_chan *chan);
	int (*send)(struct mbox_chan *chan, const void *data);
	int (*recv)(struct mbox_chan *chan, void *data);
};

struct mbox_controller {
	struct device *dev;
	const struct mbox_chan_ops *ops;
	struct mbox_chan *chans;
	int num_chans;
	struct mbox_chan *(*of_xlate)(struct mbox_controller *mbox,
				      const struct of_phandle_args *sp);
	struct list_head node;
};

int mbox_controller_register(struct mbox_controller *mbox);
struct mbox_chan *mbox_request_channel(struct device *dev, int index);
struct mbox_chan *mbox_request_channel_byname(struct device *dev, const char *name);
int mbox_send(struct mbox_chan *chan, const void *data);
int mbox_recv(struct mbox_chan *chan, void *data, unsigned long timeout_us);

#endif /* __MAILBOX_H */
