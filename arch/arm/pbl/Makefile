
OBJCOPYFLAGS_zbarebox.bin = -O binary

targets := zbarebox.lds zbarebox zbarebox.bin zbarebox.S

$(obj)/zbarebox.bin:	$(obj)/zbarebox FORCE
	$(call if_changed,objcopy)
	$(call cmd,check_file_size,$(CONFIG_BAREBOX_MAX_IMAGE_SIZE))
	@echo '  Barebox: $@ is ready'

$(obj)/zbarebox.S: $(obj)/zbarebox FORCE
	$(call if_changed,disasm)

LDFLAGS_zbarebox	:= -Map zbarebox.map
zbarebox-common := $(barebox-pbl-common)
zbarebox-lds := $(obj)/zbarebox.lds

quiet_cmd_zbarebox__ ?= LD      $@
      cmd_zbarebox__ ?= $(LD) $(LDFLAGS) $(LDFLAGS_zbarebox) -o $@ \
      -T $(zbarebox-lds)                                           \
      --start-group $(zbarebox-common) --end-group                 \
      $(filter-out $(zbarebox-lds) $(zbarebox-common) FORCE ,$^)

$(obj)/zbarebox: $(zbarebox-lds) $(zbarebox-common) FORCE
	$(call if_changed,zbarebox__)
