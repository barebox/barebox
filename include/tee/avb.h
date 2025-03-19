#ifndef TEE_AVB_H
#define TEE_AVB_H

int avb_write_persistent_value(const char *name, size_t value_size,
			       const u8 *value);
int avb_read_persistent_value(const char *name, size_t buffer_size,
			      u8 *out_buffer, size_t *out_num_bytes_read);

#endif /* TEE_AVB_H */
