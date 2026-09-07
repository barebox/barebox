ROOT := $(abspath ..)
BUILD := $(CURDIR)/build
IMAGE := $(BUILD)/images/barebox-dt-2nd.img
PLUGIN := $(BUILD)/qemu-guest-coverage.so
FORKSERVER := $(BUILD)/afl-forkserver.so
CORPUS ?= $(CURDIR)/seeds
OUT ?= $(CURDIR)/out
INPUT ?= $(CURDIR)/seeds/linux-arm64
AFL_RUNTIME := $(dir $(shell command -v afl-clang-fast))../lib/afl/afl-compiler-rt.o

QEMU := $(shell command -v qemu-system-aarch64)
QEMU += -machine virt,highmem=off -cpu cortex-a57 -m 256M
QEMU += -display none -monitor none -serial stdio -no-reboot -kernel $(IMAGE)

.PHONY: fuzz run coverage build clean

fuzz:
	AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 AFL_SKIP_BIN_CHECK=1 \
	AFL_PRELOAD=$(FORKSERVER) AFL_SKIP_CPUFREQ=1 afl-fuzz \
		-i $(CORPUS) -o $(OUT) -m none -t 1000 -- \
		$(QEMU) -plugin $(PLUGIN) \
		-drive if=none,id=fuzzdisk,file=@@,format=raw,readonly=on \
		-device virtio-blk-device,drive=fuzzdisk

run:
	cp '$(INPUT)' $(BUILD)/input
	$(QEMU) -drive if=none,id=fuzzdisk,file=$(BUILD)/input,format=raw,readonly=on \
		-device virtio-blk-device,drive=fuzzdisk

coverage:
	cp '$(INPUT)' $(BUILD)/input
	rm -f $(BUILD)/coverage.pcs
	BAREBOX_COVERAGE=$(BUILD)/coverage.pcs $(QEMU) -plugin $(PLUGIN) \
		-drive if=none,id=fuzzdisk,file=$(BUILD)/input,format=raw,readonly=on \
		-device virtio-blk-device,drive=fuzzdisk
	$(CURDIR)/coverage.py $(ROOT) $(BUILD)/barebox $(BUILD)/coverage.pcs \
		$(BUILD)/coverage.info llvm-dwarfdump llvm-addr2line
	rm -rf $(CURDIR)/coverage
	genhtml --quiet -o $(CURDIR)/coverage $(BUILD)/coverage.info

build:
	$(MAKE) -C $(ROOT) ARCH=arm64 O=$(BUILD) allnoconfig
	$(ROOT)/scripts/kconfig/merge_config.sh -Q -m -O $(BUILD) $(BUILD)/.config config
	$(MAKE) -C $(ROOT) ARCH=arm64 O=$(BUILD) olddefconfig
	$(MAKE) -C $(ROOT) ARCH=arm64 O=$(BUILD)
	clang -O2 -Wall -Wextra -Werror -shared -fPIC qemu-guest-coverage.c -o $(PLUGIN)
	clang -shared $(AFL_RUNTIME) -o $(FORKSERVER)

clean:
	rm -rf $(BUILD) $(CURDIR)/out $(CURDIR)/coverage
