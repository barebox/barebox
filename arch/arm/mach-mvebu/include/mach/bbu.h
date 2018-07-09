#ifdef CONFIG_BAREBOX_UPDATE
int mvebu_bbu_flash_register_handler(const char *name,
				     char *devicefile, int version,
				     bool isdefault);
#else
int mvebu_bbu_flash_register_handler(const char *name,
                                     char *devicefile, int version,
                                     bool isdefault)
{
	return -ENOSYS;
}
#endif
