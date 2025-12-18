/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __EFI_LOADER_EVENT_H_
#define __EFI_LOADER_EVENT_H_

#include <efi/types.h>
#include <efi/services.h>
#include <linux/list.h>

/**
 * struct efi_event
 *
 * @link:		Link to list of all events
 * @queue_link:		Link to the list of queued events
 * @type:		Type of event, see efi_create_event
 * @notify_tpl:		Task priority level of notifications
 * @notify_function:	Function to call when the event is triggered
 * @notify_context:	Data to be passed to the notify function
 * @group:		Event group
 * @trigger_time:	Period of the timer
 * @trigger_next:	Next time to trigger the timer
 * @trigger_type:	Type of timer, see efi_set_timer
 * @is_signaled:	The event occurred. The event is in the signaled state.
 */
struct efi_event {
	struct list_head link;
	struct list_head queue_link;
	uint32_t type;
	efi_uintn_t notify_tpl;
	void (EFIAPI *notify_function)(struct efi_event *event, void *context);
	void *notify_context;
	const efi_guid_t *group;
	u64 trigger_next;
	u64 trigger_time;
	enum efi_timer_delay trigger_type;
	bool is_signaled;
};

/* List of all events */
extern struct list_head efi_events;

/**
 * struct efi_protocol_notification - handle for notified protocol
 *
 * When a protocol interface is installed for which an event was registered with
 * the RegisterProtocolNotify() service this structure is used to hold the
 * handle on which the protocol interface was installed.
 *
 * @link:	link to list of all handles notified for this event
 * @handle:	handle on which the notified protocol interface was installed
 */
struct efi_protocol_notification {
	struct list_head link;
	efi_handle_t handle;
};

/**
 * struct efi_register_notify_event - event registered by
 *				      RegisterProtocolNotify()
 *
 * The address of this structure serves as registration value.
 *
 * @link:	link to list of all registered events
 * @event:	registered event. The same event may registered for multiple
 *		GUIDs.
 * @protocol:	protocol for which the event is registered
 * @handles:	linked list of all handles on which the notified protocol was
 *		installed
 */
struct efi_register_notify_event {
	struct list_head link;
	struct efi_event *event;
	efi_guid_t protocol;
	struct list_head handles;
};

/* Call this to create an event */
efi_status_t efi_create_event(uint32_t type, efi_uintn_t notify_tpl,
			      void (EFIAPI *notify_function) (
					struct efi_event *event,
					void *context),
			      void *notify_context, const efi_guid_t *group,
			      struct efi_event **event);
/* Call this to close an event */
efi_status_t EFIAPI efi_close_event(struct efi_event *event);
/* Call this to set a timer */
efi_status_t efi_set_timer(struct efi_event *event, enum efi_timer_delay type,
			   uint64_t trigger_time);
/* Call this to signal an event */
void efi_signal_event(struct efi_event *event);
/* Called from places to check whether a timer expired */
void efi_timer_check(void);

#endif
