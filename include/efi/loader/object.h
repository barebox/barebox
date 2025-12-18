/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *  EFI application loader
 *
 *  Copyright (c) 2016 Alexander Graf
 */

#ifndef _EFI_LOADER_OBJECT_H
#define _EFI_LOADER_OBJECT_H

#include <efi/types.h>
#include <efi/services.h>

/**
 * enum efi_object_type - type of EFI object
 *
 * In UnloadImage we must be able to identify if the handle relates to a
 * started image.
 */
enum efi_object_type {
	/** @EFI_OBJECT_TYPE_UNDEFINED: undefined image type */
	EFI_OBJECT_TYPE_UNDEFINED = 0,
	/** @EFI_OBJECT_TYPE_BAREBOX_FIRMWARE: barebox firmware */
	EFI_OBJECT_TYPE_BAREBOX_FIRMWARE,
	/** @EFI_OBJECT_TYPE_LOADED_IMAGE: loaded image (not started) */
	EFI_OBJECT_TYPE_LOADED_IMAGE,
	/** @EFI_OBJECT_TYPE_STARTED_IMAGE: started image */
	EFI_OBJECT_TYPE_STARTED_IMAGE,
};

/**
 * struct efi_object - dereferenced EFI handle
 *
 * @link:	pointers to put the handle into a linked list
 * @protocols:	linked list with the protocol interfaces installed on this
 *		handle
 * @type:	image type if the handle relates to an image
 *
 * UEFI offers a flexible and expandable object model. The objects in the UEFI
 * API are devices, drivers, and loaded images. struct efi_object is our storage
 * structure for these objects.
 *
 * When including this structure into a larger structure always put it first so
 * that when deleting a handle the whole encompassing structure can be freed.
 *
 * A pointer to this structure is referred to as a handle. Typedef efi_handle_t
 * has been created for such pointers.
 */
struct efi_object {
	/* Every UEFI object is part of a global object list */
	struct list_head link;
	/* The list of protocols */
	struct list_head protocols;
	enum efi_object_type type;
};

/**
 * struct efi_open_protocol_info_item - open protocol info item
 *
 * When a protocol is opened a open protocol info entry is created.
 * These are maintained in a list.
 *
 * @link:	link to the list of open protocol info entries of a protocol
 * @info:	information about the opening of a protocol
 */
struct efi_open_protocol_info_item {
	struct list_head link;
	struct efi_open_protocol_information_entry info;
};

/**
 * struct efi_handler - single protocol interface of a handle
 *
 * When the UEFI payload wants to open a protocol on an object to get its
 * interface (usually a struct with callback functions), this struct maps the
 * protocol GUID to the respective protocol interface
 *
 * @link:		link to the list of protocols of a handle
 * @guid:		GUID of the protocol
 * @protocol_interface:	protocol interface
 * @open_infos:		link to the list of open protocol info items
 */
struct efi_handler {
	struct list_head link;
	efi_guid_t guid;
	void *protocol_interface;
	struct list_head open_infos;
};

/* This list contains all UEFI objects we know of */
extern struct list_head efi_obj_list;

/* Root node */
extern efi_handle_t efi_root;

/* Initialize efi execution environment */
efi_status_t efi_init_obj_list(void);

/* Add a new object to the object list. */
void efi_add_handle(efi_handle_t obj);
/* Create handle */
efi_status_t efi_create_handle(efi_handle_t *handle);
/* Delete handle */
efi_status_t efi_delete_handle(efi_handle_t obj);
/* Call this to validate a handle and find the EFI object for it */
struct efi_object *efi_search_obj(const efi_handle_t handle);

/* Find a protocol on a handle */
efi_status_t efi_search_protocol(const efi_handle_t handle,
				 const efi_guid_t *protocol_guid,
				 struct efi_handler **handler);
/* Install new protocol on a handle */
efi_status_t efi_add_protocol(const efi_handle_t handle,
			      const efi_guid_t *protocol,
			      const void *protocol_interface);
/* Open protocol */
efi_status_t efi_protocol_open(struct efi_handler *handler,
			       void **protocol_interface, void *agent_handle,
			       void *controller_handle, uint32_t attributes);

/* Install multiple protocol interfaces */
efi_status_t EFIAPI
efi_install_multiple_protocol_interfaces(efi_handle_t *handle, ...) __attribute__((sentinel));
efi_status_t EFIAPI
efi_uninstall_multiple_protocol_interfaces(efi_handle_t handle, ...)  __attribute__((sentinel));
/* Get handles that support a given protocol */
efi_status_t EFIAPI efi_locate_handle_buffer(
			enum efi_locate_search_type search_type,
			const efi_guid_t *protocol, void *search_key,
			size_t *no_handles, efi_handle_t **buffer);
/* Close an previously opened protocol interface */
efi_status_t EFIAPI efi_close_protocol(efi_handle_t handle,
				       const efi_guid_t *protocol,
				       efi_handle_t agent_handle,
				       efi_handle_t controller_handle);

#endif /* _EFI_LOADER_OBJECT_H */
