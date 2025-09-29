# SPDX-License-Identifier: GPL-2.0-only

obj-$(CONFIG_SECURITY_POLICY)		+= policy.o
obj-$(CONFIG_SECURITY_POLICY_NAMES)	+= sconfig_names.o
obj-$(CONFIG_CRYPTO_KEYSTORE)	+= keystore.o
obj-$(CONFIG_JWT)		+= jwt.o
obj-pbl-$(CONFIG_HAVE_OPTEE)	+= optee.o
obj-$(CONFIG_BLOBGEN)		+= blobgen.o
obj-$(CONFIG_PASSWORD)		+= password.o

# Default password handling
# ---------------------------------------------------------------------------
#
ifdef CONFIG_PASSWORD

ifeq ($(CONFIG_PASSWORD_DEFAULT),"")
define filechk_passwd
	echo "static const char default_passwd[] = \"\";"
endef
else
define filechk_passwd
(							\
echo -n "static const char default_passwd[] = \"";	\
cat $(shell find $(srctree) -name $(CONFIG_PASSWORD_DEFAULT) -type f) | tr -d '\n';\
echo "\";"						\
)
endef
endif

include/generated/passwd.h: FORCE
	$(call filechk,passwd)

$(obj)/password.o: include/generated/passwd.h
endif # CONFIG_PASSWORD

# External security policy handling
# ---------------------------------------------------------------------------

external-policy := $(foreach p, \
	$(call remove_quotes,$(CONFIG_SECURITY_POLICY_PATH)), \
		$(p:security/%=%))

external-policy-tmp := $(addsuffix .tmp,$(external-policy))
real-external-policy-tmp := $(addprefix $(obj)/,$(external-policy-tmp))

ifneq ($(external-policy),)
obj-y	+= default.sconfig.o
extra-y	+= default.sconfig.c
always-y += policy-list
$(foreach p, $(external-policy), \
	$(if $(findstring /,$p),$(error \
	CONFIG_SECURITY_POLICY_PATH contains path separators.\
	$(newline)"$p" must start with security/)))
$(foreach p, $(external-policy), \
	$(if $(wildcard $(srctree)/$(src)/$p),,$(error \
	CONFIG_SECURITY_POLICY_PATH contains non-existent files.\
	$(newline)"$p" does not exist in $$(srctree)/security)))
endif

$(obj)/policy-list: $(addprefix $(src)/,$(external-policy)) FORCE
	$(call if_changed,gen_order_src)

targets += $(external-policy-tmp)

$(obj)/default.sconfig.c: $(real-external-policy-tmp) FORCE
	+$(Q)$(foreach p, $(real-external-policy-tmp), \
		$(call noop_cmd,security_checkconfig,$p) ;)
	$(call if_changed_dep,sconfigpost_c,$(real-external-policy-tmp))
