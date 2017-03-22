#include <linux/types.h>
#include <linux/list.h>
#include <driver.h>

struct state;
struct mtd_info_user;

/**
 * state_backend_storage_bucket - This class describes a single backend storage
 * object copy
 *
 * @init Optional, initiates the given bucket
 * @write Required, writes the given data to the storage in any form. Returns 0
 * on success
 * @read Required, reads the last successfully written data from the backend
 * storage. Returns 0 on success and allocates a matching memory area to buf.
 * len_hint can be a hint of the storage format how large the data to be read
 * is. After the operation len_hint contains the size of the allocated buffer.
 * @free Required, Frees all internally used memory
 * @bucket_list A list element struct to attach this bucket to a list
 */
struct state_backend_storage_bucket {
	int (*init) (struct state_backend_storage_bucket * bucket);
	int (*write) (struct state_backend_storage_bucket * bucket,
		      const uint8_t * buf, ssize_t len);
	int (*read) (struct state_backend_storage_bucket * bucket,
		     uint8_t ** buf, ssize_t * len_hint);
	void (*free) (struct state_backend_storage_bucket * bucket);

	bool initialized;
	struct list_head bucket_list;
};

/**
 * state_backend_format - This class describes a data format.
 *
 * @verify Required, Verifies the validity of the given data. The buffer that is
 * passed into this function may be larger than the actual data in the buffer.
 * The magic is supplied by the state to verify that this is an expected state
 * entity. The function should return 0 on success or a negative errno otherwise.
 * @pack Required, Packs data from the given state into a newly created buffer.
 * The buffer and its length are stored in the given argument pointers. Returns
 * 0 on success, -errno otherwise.
 * @unpack Required, Unpacks the data from the given buffer into the state. Do
 * not free the buffer.
 * @free Optional, Frees all allocated memory and structures.
 * @name Name of this backend.
 */
struct state_backend_format {
	int (*verify) (struct state_backend_format * format, uint32_t magic,
		       const uint8_t * buf, ssize_t len);
	int (*pack) (struct state_backend_format * format, struct state * state,
		     uint8_t ** buf, ssize_t * len);
	int (*unpack) (struct state_backend_format * format,
		       struct state * state, const uint8_t * buf, ssize_t len);
	void (*free) (struct state_backend_format * format);
	const char *name;
};

/**
 * state_backend_storage - Storage backend of the state.
 *
 * @buckets List of storage buckets that are available
 */
struct state_backend_storage {
	struct list_head buckets;

	/* For outputs */
	struct device_d *dev;

	const char *name;

	uint32_t stridesize;

	bool readonly;
};

struct state {
	struct list_head list; /* Entry to enqueue on list of states */

	struct device_d dev;
	char *of_path;
	const char *name;
	uint32_t magic;

	struct list_head variables; /* Sorted list of variables */
	unsigned int dirty;
	unsigned int save_on_shutdown;

	struct state_backend_format *format;
	struct state_backend_storage storage;
	char *of_backend_path;
};

enum state_convert {
	STATE_CONVERT_FROM_NODE,
	STATE_CONVERT_FROM_NODE_CREATE,
	STATE_CONVERT_TO_NODE,
	STATE_CONVERT_FIXUP,
};

enum state_variable_type {
	STATE_TYPE_INVALID = 0,
	STATE_TYPE_ENUM,
	STATE_TYPE_U8,
	STATE_TYPE_U32,
	STATE_TYPE_MAC,
	STATE_TYPE_STRING,
};

struct state_variable;

/* A variable type (uint32, enum32) */
struct variable_type {
	enum state_variable_type type;
	const char *type_name;
	struct list_head list;
	int (*export) (struct state_variable *, struct device_node *,
		       enum state_convert);
	int (*import) (struct state_variable *, struct device_node *);
	struct state_variable *(*create) (struct state * state,
					  const char *name,
					  struct device_node *);
};

/* instance of a single variable */
struct state_variable {
	struct state *state;
	enum state_variable_type type;
	struct list_head list;
	const char *name;
	unsigned int start;
	unsigned int size;
	void *raw;
};

/*
 *  uint32
 */
struct state_uint32 {
	struct state_variable var;
	struct param_d *param;
	uint32_t value;
	uint32_t value_default;
};

/*
 *  enum32
 */
struct state_enum32 {
	struct state_variable var;
	struct param_d *param;
	uint32_t value;
	uint32_t value_default;
	const char **names;
	int num_names;
};

/*
 *  MAC address
 */
struct state_mac {
	struct state_variable var;
	struct param_d *param;
	uint8_t value[6];
	uint8_t value_default[6];
};

/*
 *  string
 */
struct state_string {
	struct state_variable var;
	struct param_d *param;
	char *value;
	const char *value_default;
	char raw[];
};

int state_from_node(struct state *state, struct device_node *node, bool create);
struct device_node *state_to_node(struct state *state,
				  struct device_node *parent,
				  enum state_convert conv);
int backend_format_raw_create(struct state_backend_format **format,
			      struct device_node *node, const char *secret_name,
			      struct device_d *dev);
int backend_format_dtb_create(struct state_backend_format **format,
			      struct device_d *dev);
int state_storage_init(struct state_backend_storage *storage,
		       struct device_d *dev, const char *path,
		       off_t offset, size_t max_size, uint32_t stridesize,
		       const char *storagetype);
void state_storage_set_readonly(struct state_backend_storage *storage);
void state_add_var(struct state *state, struct state_variable *var);
struct variable_type *state_find_type_by_name(const char *name);
int state_backend_bucket_circular_create(struct device_d *dev, const char *path,
					 struct state_backend_storage_bucket **bucket,
					 unsigned int eraseblock,
					 ssize_t writesize,
					 struct mtd_info_user *mtd_uinfo,
					 bool lazy_init);
int state_backend_bucket_cached_create(struct device_d *dev,
				       struct state_backend_storage_bucket *raw,
				       struct state_backend_storage_bucket **out);
struct state_variable *state_find_var(struct state *state, const char *name);
struct digest *state_backend_format_raw_get_digest(struct state_backend_format
						   *format);
void state_backend_set_readonly(struct state *state);
void state_storage_free(struct state_backend_storage *storage);
int state_backend_bucket_direct_create(struct device_d *dev, const char *path,
				       struct state_backend_storage_bucket **bucket,
				       off_t offset, ssize_t max_size);
int state_storage_write(struct state_backend_storage *storage,
			const uint8_t * buf, ssize_t len);
int state_storage_restore_consistency(struct state_backend_storage
				      *storage, const uint8_t * buf,
				      ssize_t len);
int state_storage_read(struct state_backend_storage *storage,
		       struct state_backend_format *format,
		       uint32_t magic, uint8_t **buf, ssize_t *len);

static inline struct state_uint32 *to_state_uint32(struct state_variable *s)
{
	return container_of(s, struct state_uint32, var);
}

static inline struct state_enum32 *to_state_enum32(struct state_variable *s)
{
	return container_of(s, struct state_enum32, var);
}

static inline struct state_mac *to_state_mac(struct state_variable *s)
{
	return container_of(s, struct state_mac, var);
}

static inline struct state_string *to_state_string(struct state_variable *s)
{
	return container_of(s, struct state_string, var);
}

static inline int state_string_copy_to_raw(struct state_string *string,
				    const char *src)
{
	size_t len;

	len = strlen(src);
	if (len > string->var.size)
		return -EILSEQ;

	/* copy string and clear remaining contents of buffer */
	memcpy(string->raw, src, len);
	memset(string->raw + len, 0x0, string->var.size - len);

	return 0;
}
