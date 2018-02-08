#ifndef __RATP_H
#define __RATP_H

struct ratp {
	struct ratp_internal *internal;
	int (*send)(struct ratp *, void *pkt, int len);
	int (*recv)(struct ratp *, uint8_t *data);
};

int ratp_establish(struct ratp *ratp, bool active, int timeout_ms);
void ratp_close(struct ratp *ratp);
int ratp_recv(struct ratp *ratp, void **data, size_t *len);
int ratp_send(struct ratp *ratp, const void *data, size_t len);
int ratp_send_complete(struct ratp *ratp, const void *data, size_t len,
		   void (*complete)(void *ctx, int status), void *complete_ctx);
int ratp_poll(struct ratp *ratp);
bool ratp_closed(struct ratp *ratp);
bool ratp_busy(struct ratp *ratp);

#endif /* __RATP_H */