/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Transport Definition
 *
 *  Copyright (C) 2005 by Latchesar Ionkov <lucho@ionkov.net>
 *  Copyright (C) 2004-2008 by Eric Van Hensbergen <ericvh@gmail.com>
 */

#ifndef NET_9P_TRANSPORT_H
#define NET_9P_TRANSPORT_H

#include <linux/module.h>
#include <linux/list.h>

#define P9_DEF_MIN_RESVPORT	(665U)
#define P9_DEF_MAX_RESVPORT	(1023U)

struct p9_client;
struct p9_req_t;
struct iov_iter;

/**
 * struct p9_trans_module - transport module interface
 * @list: used to maintain a list of currently available transports
 * @name: the human-readable name of the transport
 * @maxsize: transport provided maximum packet size
 * @pooled_rbuffers: currently only set for RDMA transport which pulls the
 *                   response buffers from a shared pool, and accordingly
 *                   we're less flexible when choosing the response message
 *                   size in this case
 * @def: set if this transport should be considered the default
 * @create: member function to create a new connection on this transport
 * @close: member function to discard a connection on this transport
 * @request: member function to issue a request to the transport
 * @poll: member function to poll for completed requests
 * @cancel: member function to cancel a request (if it hasn't been sent)
 * @cancelled: member function to notify that a cancelled request will not
 *             receive a reply
 *
 * This is the basic API for a transport module which is registered by the
 * transport module with the 9P core network module and used by the client
 * to instantiate a new connection on a transport.
 *
 * The transport module list is protected by v9fs_trans_lock.
 */

struct p9_trans_module {
	struct list_head list;
	char *name;		/* name of transport */
	int maxsize;		/* max message size of transport */
	bool pooled_rbuffers;
	int def;		/* this transport should be default */
	struct module *owner;
	int (*create)(struct p9_client *client,
		      const char *devname, char *args);
	void (*close)(struct p9_client *client);
	int (*request)(struct p9_client *client, struct p9_req_t *req);
	int (*poll)(struct p9_client *client);
	int (*cancel)(struct p9_client *client, struct p9_req_t *req);
	int (*cancelled)(struct p9_client *client, struct p9_req_t *req);
	int (*show_options)(struct p9_client *client);
};

void v9fs_register_trans(struct p9_trans_module *m);
void v9fs_unregister_trans(struct p9_trans_module *m);
struct p9_trans_module *v9fs_get_trans_by_name(const char *s);
struct p9_trans_module *v9fs_get_default_trans(void);
static inline void v9fs_put_trans(struct p9_trans_module *m) {}

#define MODULE_ALIAS_9P(transport) \
	MODULE_ALIAS("9p-" transport)

#endif /* NET_9P_TRANSPORT_H */
