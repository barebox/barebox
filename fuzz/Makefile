ROOT := $(abspath ..)
BUILD := $(CURDIR)/build
COVERAGE_BUILD := $(CURDIR)/coverage
COVERAGE_FLAGS := --target=$(BAREBOX_CLANG_TARGET) -fintegrated-as
COVERAGE_FLAGS += -Wno-unused-command-line-argument
COVERAGE_HTML_FLAGS := --ignore-errors unsupported,unsupported
COVERAGE_HTML_FLAGS += inconsistent,inconsistent,range,range
COVERAGE_MAKE := $(MAKE) -C $(ROOT) ARCH=sandbox LLVM=1 UTS_MACHINE=arm64 \
	CLANG_FLAGS='$(COVERAGE_FLAGS)' \
	O=$(COVERAGE_BUILD)
CORPUS := $(CURDIR)/corpus
BAREBOX := $(BUILD)/images/barebox
RUN_SEED := $(word 2,$(MAKECMDGOALS))
RUN_INPUT := $(ROOT)/$(RUN_SEED)
PARTIAL_DIGESTS ?= 1
BOOT_COMMAND := global allow_color=0; boot -dd /dev/fuzzdisk*;
BOOT_COMMAND += bootm -d /dev/fuzzdisk;
BOOT_COMMAND += bootm -sd /dev/fuzzdisk; poweroff

.PHONY: fuzz run build coverage clean

ifneq ($(RUN_SEED),)
ifeq ($(firstword $(MAKECMDGOALS)),run)
.PHONY: $(RUN_SEED)
$(RUN_SEED):
	@:
endif
endif

fuzz:
	test -x '$(BAREBOX)' || { echo 'run make -C fuzz build first'; exit 1; }
	AFL_COMPCOV_LEVEL=2 AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 \
	AFL_SKIP_CPUFREQ=1 afl-fuzz -Q \
		-i $(CORPUS) -o $(CURDIR)/out -m none -t 2000 \
		-G 1048576 -- $(BAREBOX) -m 67108864 \
		-i 'fuzzdisk=@@,ro,blkdev' \
		-c '$(BOOT_COMMAND)'

run:
	test -x '$(BAREBOX)' || { echo 'run make -C fuzz build first'; exit 1; }
	test -n '$(RUN_SEED)' || { echo 'usage: make -C fuzz run fuzz/corpus/SEED'; exit 1; }
	test -f '$(RUN_INPUT)' || { echo 'input not found: $(RUN_SEED)'; exit 1; }
	$(BAREBOX) -m 67108864 -i 'fuzzdisk=$(RUN_INPUT),ro,blkdev' \
		-c '$(BOOT_COMMAND)'

build:
	$(MAKE) -C $(ROOT) ARCH=sandbox O=$(BUILD) sandbox_defconfig
	$(ROOT)/scripts/kconfig/merge_config.sh -Q -m -O $(BUILD) \
		$(BUILD)/.config $(CURDIR)/config
	$(ROOT)/scripts/config --file $(BUILD)/.config \
		$(if $(filter 0,$(PARTIAL_DIGESTS)),-d,-e) FUZZ_INSECURE_PARTIAL_DIGEST
	$(MAKE) -C $(ROOT) ARCH=sandbox O=$(BUILD) olddefconfig
	$(MAKE) -C $(ROOT) ARCH=sandbox O=$(BUILD)

coverage:
	$(COVERAGE_MAKE) sandbox_defconfig
	$(ROOT)/scripts/kconfig/merge_config.sh -Q -m -O $(COVERAGE_BUILD) \
		$(COVERAGE_BUILD)/.config $(CURDIR)/config
	$(ROOT)/scripts/config --file $(COVERAGE_BUILD)/.config -e GCOV
	$(COVERAGE_MAKE) olddefconfig
	$(COVERAGE_MAKE)
	rm -rf $(COVERAGE_BUILD)/profiles $(COVERAGE_BUILD)/barebox.coverage_html
	rm -f $(COVERAGE_BUILD)/coverage-*.profraw
	for input in $(CORPUS)/* $(CURDIR)/out/default/queue/*; do \
		test -f "$$input" || continue; \
		LLVM_PROFILE_FILE=$(COVERAGE_BUILD)/coverage-%m.profraw \
		$(COVERAGE_BUILD)/images/barebox -m 67108864 \
		-i "fuzzdisk=$$input,ro,blkdev" -c '$(BOOT_COMMAND)' >/dev/null 2>&1 || \
		{ echo "coverage replay failed: $$input"; exit 1; }; \
	done
	llvm-profdata merge -sparse $(COVERAGE_BUILD)/coverage-*.profraw \
		-o $(COVERAGE_BUILD)/default.profdata
	$(COVERAGE_MAKE) \
		GENHTML_FLAGS='$(COVERAGE_HTML_FLAGS)' \
		PROFRAW=$(firstword $(wildcard $(COVERAGE_BUILD)/coverage-*.profraw)) \
		coverage-html

clean:
	rm -rf $(BUILD) $(COVERAGE_BUILD) $(CURDIR)/out
