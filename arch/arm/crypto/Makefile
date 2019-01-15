#
# Arch-specific CryptoAPI modules.
#

obj-$(CONFIG_DIGEST_SHA1_ARM) += sha1-arm.o
obj-$(CONFIG_DIGEST_SHA256_ARM) += sha256-arm.o

sha1-arm-y	:= sha1-armv4-large.o sha1_glue.o
sha256-arm-y	:= sha256-core.o sha256_glue.o

quiet_cmd_perl = PERL    $@
      cmd_perl = $(PERL) $(<) > $(@)

$(src)/sha256-core.S_shipped: $(src)/sha256-armv4.pl
	$(call cmd,perl)
