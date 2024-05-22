// SPDX-License-Identifier: GPL-2.0
#include <common.h>
#include <pbl/handoff-data.h>
#include <init.h>
#include <linux/list.h>
#include <memory.h>

static struct handoff_data *handoff_data = (void *)-1;

static struct handoff_data *handoff_data_get(void)
{
	static struct handoff_data __handoff_data;

	/*
	 * Sometimes the PBL copies itself to some other location and is
	 * re-entered at that location. For example on some i.MX SoCs we have
	 * to move the PBL out of the SRAM (which will be occupied by the TF-A
	 * later). We force the handoff_data variable into the data segment.
	 * When moving the PBL somewhere else with handoff_data set we move the
	 * content of the variable with it and thus find it to have the correct
	 * value in the new PBL.
	 */
	if (handoff_data == (void *)-1) {
		handoff_data = &__handoff_data;
		INIT_LIST_HEAD(&handoff_data->entries);
	}

	return handoff_data;
}

/**
 * handoff_data_set - set the handoff data to be at a specified pointer
 * @handoff: the place where the handoff data is
 *
 * This sets the handoff data to @handoff. To be used by barebox proper
 * to pass the place where the handoff data has been placed by the PBL.
 */
void handoff_data_set(struct handoff_data *handoff)
{
	handoff_data = handoff;
}

/**
 * handoff_data_add_entry - add a new handoff data entry
 * @hde: the new entry
 *
 * This adds a new handoff data entry.
 */
void handoff_data_add_entry(struct handoff_data_entry *hde)
{
	struct handoff_data *hd = handoff_data_get();

	list_add_tail(&hde->list, &hd->entries);
}

/**
 * handoff_data_size - calculate the handoff data size
 *
 * This calculates the size needed for the current handoff data
 * when put to a contiguous memory regions. Can be used to get the
 * size needed in preparation for a handoff_data_move()
 */
size_t __handoff_data_size(const struct handoff_data *hd)
{
	struct handoff_data_entry *hde;
	size_t size = 0;
	size_t dsize = 0;

	if (!hd)
		hd = handoff_data_get();

	dsize += sizeof(*hd);

	list_for_each_entry(hde, &hd->entries, list) {
		dsize += sizeof(*hde);
		size += ALIGN(hde->size, 8);
	}

	return dsize + size;
}

/**
 * handoff_data_move - move handoff data to specified destination
 * @dest: The place where to move the handoff data to
 *
 * This moves the handoff data to @dest and also sets the new location
 * to @dest. This can be used to move the handoff data to a contiguous
 * region outside the binary. Note once moved no data should be added,
 * as that would make the handoff_data discontigoous again.
 */
void handoff_data_move(void *dest)
{
	struct handoff_data *hd = handoff_data_get();
	struct handoff_data *hdnew = dest;
	struct handoff_data_entry *hde;

	INIT_LIST_HEAD(&hdnew->entries);

	dest = hdnew + 1;

	list_for_each_entry(hde, &hd->entries, list) {
		struct handoff_data_entry *newde = dest;

		dest = newde + 1;

		if (hde->flags & HANDOFF_DATA_FLAG_NO_COPY) {
			newde->data = hde->data;
		} else {
			memcpy(dest, hde->data, hde->size);
			newde->data = dest;
			dest += ALIGN(hde->size, 8);
		}

		newde->size = hde->size;
		newde->cookie = hde->cookie;
		list_add_tail(&newde->list, &hdnew->entries);
	}

	handoff_data_set(hdnew);
}

/**
 * handoff_data_get_entry - get the memory associated to a cookie
 * @cookie: the cookie the data is identified with
 * @size: size of the memory returned
 *
 * This returns the memory associated with @cookie.
 */
void *handoff_data_get_entry(unsigned int cookie, size_t *size)
{
	struct handoff_data *hd = handoff_data_get();
	struct handoff_data_entry *hde;

	list_for_each_entry(hde, &hd->entries, list) {
		if (hde->cookie == cookie) {
			*size = hde->size;
			return hde->data;
		}
	}

	return NULL;
}

/**
 * handoff_data_show - show current handoff data entries
 *
 * This prints the current handoff data entries to the console for debugging
 * purposes.
 */
int handoff_data_show(void)
{
	struct handoff_data *hd = handoff_data_get();
	struct handoff_data_entry *hde;

	list_for_each_entry(hde, &hd->entries, list) {
		printf("handoff 0x%08x at 0x%p (size %zu)\n",
			hde->cookie, hde->data, hde->size);
	}

	return 0;
}

static const char *handoff_data_entry_name(struct handoff_data_entry *hde)
{
	static char name[sizeof("handoff 12345678")];

	switch (hde->cookie) {
	case HANDOFF_DATA_INTERNAL_DT:
		return "handoff FDT (internal)";
	case HANDOFF_DATA_INTERNAL_DT_Z:
		return "handoff FDT (internal, compressed)";
	case HANDOFF_DATA_EXTERNAL_DT:
		return "handoff FDT (external)";
	case HANDOFF_DATA_BOARDDATA:
		return "handoff boarddata";
	default:
		sprintf(name, "handoff %08x", hde->cookie);
		return name;
	}
}

static int handoff_data_reserve(void)
{
	struct handoff_data *hd = handoff_data_get();
	struct handoff_data_entry *hde;

	list_for_each_entry(hde, &hd->entries, list) {
		const char *name = handoff_data_entry_name(hde);
		request_barebox_region(name, (resource_size_t)hde->data, hde->size);
	}

	return 0;
}
late_initcall(handoff_data_reserve);
