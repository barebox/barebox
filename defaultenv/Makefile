bbenv-$(CONFIG_DEFAULT_ENVIRONMENT_GENERIC_NEW) += defaultenv-2-base
bbenv-$(CONFIG_DEFAULT_ENVIRONMENT_GENERIC_NEW_MENU) += defaultenv-2-menu
bbenv-$(CONFIG_DEFAULT_ENVIRONMENT_GENERIC_NEW_DFU) += defaultenv-2-dfu
bbenv-$(CONFIG_DEFAULT_ENVIRONMENT_GENERIC) += defaultenv-1
obj-$(CONFIG_DEFAULT_ENVIRONMENT) += defaultenv.o
extra-y += barebox_default_env barebox_default_env.h barebox_default_env$(DEFAULT_COMPRESSION_SUFFIX) barebox_zero_env

$(obj)/defaultenv.o: $(obj)/barebox_default_env.h

#
# Generate a barebox envfs image.
#
quiet_cmd_env_default = ENV     $@
cmd_env_default = ($(srctree)/scripts/genenv $(srctree) $(objtree) $@ $(CONFIG_DEFAULT_ENVIRONMENT_PATH))

$(obj)/barebox_default_env: FORCE
	$(call if_changed,env_default)

quiet_cmd_env_h = ENVH    $@
cmd_env_h = cat $< | (cd $(obj) && $(objtree)/scripts/bin2c "__aligned(4) default_environment") > $@; \
	echo "static const int default_environment_uncompress_size=`stat -c%s $(obj)/barebox_default_env`;" >> $@

$(obj)/barebox_default_env.h: $(obj)/barebox_default_env$(DEFAULT_COMPRESSION_SUFFIX) FORCE
	$(call if_changed,env_h)

quiet_cmd_env_zero = ENVZ    $@
cmd_env_zero = ($(objtree)/scripts/bareboxenv -z $(CONFIG_DEFAULT_ENVIRONMENT_PATH) $@)

$(obj)/barebox_zero_env: FORCE
	$(call if_changed,env_zero)
