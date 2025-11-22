# SPDX-License-Identifier: GPL-2.0
# SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/Makefile?id=7d0a66e4bb9081d75c82ec4957c50034cb0ea449

obj-$(CONFIG_RUST) += core.o compiler_builtins.o ffi.o
always-$(CONFIG_RUST) += exports_core_generated.h

# Missing prototypes are expected in the helpers since these are exported
# for Rust only, thus there is no header nor prototypes.
obj-$(CONFIG_RUST) += helpers/helpers.o
CFLAGS_REMOVE_helpers/helpers.o = -Wmissing-prototypes -Wmissing-declarations

always-$(CONFIG_RUST) += bindings/bindings_generated.rs bindings/bindings_helpers_generated.rs
obj-$(CONFIG_RUST) += bindings.o pin_init.o kernel.o
always-$(CONFIG_RUST) += exports_helpers_generated.h \
    exports_bindings_generated.h exports_kernel_generated.h

always-$(CONFIG_RUST) += uapi/uapi_generated.rs
obj-$(CONFIG_RUST) += uapi.o

ifdef CONFIG_RUST_BUILD_ASSERT_ALLOW
obj-$(CONFIG_RUST) += build_error.o
else
always-$(CONFIG_RUST) += build_error.o
endif

obj-$(CONFIG_RUST) += exports.o

# Avoids running `$(RUSTC)` when it may not be available.
ifdef CONFIG_RUST

libmacros_name := $(shell MAKEFLAGS= $(RUSTC) --print file-names --crate-name macros --crate-type proc-macro - </dev/null)
libmacros_extension := $(patsubst libmacros.%,%,$(libmacros_name))

libpin_init_internal_name := $(shell MAKEFLAGS= $(RUSTC) --print file-names --crate-name pin_init_internal --crate-type proc-macro - </dev/null)
libpin_init_internal_extension := $(patsubst libpin_init_internal.%,%,$(libpin_init_internal_name))

always-$(CONFIG_RUST) += $(libmacros_name) $(libpin_init_internal_name)

# `$(rust_flags)` is passed in case the user added `--sysroot`.
rustc_sysroot := $(shell MAKEFLAGS= $(RUSTC) $(rust_flags) --print sysroot)
RUST_LIB_SRC ?= $(rustc_sysroot)/lib/rustlib/src/rust/library

core-cfgs = \
    --cfg no_fp_fmt_parse \
    --cfg no_io_statics

core-edition := $(if $(call rustc-min-version,108700),2024,2021)

ifdef CONFIG_CC_IS_CLANG
bindgen_c_flags = $(c_flags)
else
# bindgen relies on libclang to parse C. Ideally, bindgen would support a GCC
# plugin backend and/or the Clang driver would be perfectly compatible with GCC.
#
# For the moment, here we are tweaking the flags on the fly. This is a hack,
# and some kernel configurations may not work (e.g. `GCC_PLUGIN_RANDSTRUCT`
# if we end up using one of those structs).
bindgen_skip_c_flags := -mno-fp-ret-in-387 -mpreferred-stack-boundary=% \
	-mskip-rax-setup -mgeneral-regs-only -msign-return-address=% \
	-mindirect-branch=thunk-extern -mindirect-branch-register \
	-mfunction-return=thunk-extern -mrecord-mcount -mabi=lp64 \
	-mindirect-branch-cs-prefix -mstack-protector-guard% -mtraceback=no \
	-mno-pointers-to-nested-functions -mno-string \
	-mno-strict-align -mstrict-align -mdirect-extern-access \
	-mexplicit-relocs -mno-check-zero-division \
	-fconserve-stack -falign-jumps=% -falign-loops=% \
	-femit-struct-debug-baseonly -fno-ipa-cp-clone -fno-ipa-sra \
	-fno-partial-inlining -fplugin-arg-arm_ssp_per_task_plugin-% \
	-fno-reorder-blocks -fno-allow-store-data-races -fasan-shadow-offset=% \
	-fzero-call-used-regs=% -fno-stack-clash-protection \
	-fno-inline-functions-called-once -fsanitize=bounds-strict \
	-fstrict-flex-arrays=% -fmin-function-alignment=% \
	-fzero-init-padding-bits=% -mno-fdpic \
	--param=% --param asan-% -fno-isolate-erroneous-paths-dereference

# Derived from `scripts/Makefile.clang`.
BINDGEN_TARGET_x86	:= x86_64-linux-gnu
# ARCH=arm covers both ARM32 and ARM64
BINDGEN_TARGET_arm	:= $(if $(CONFIG_CPU_64),aarch64-linux-gnu,arm-linux-gnueabi)
# sandbox is built for the host
BINDGEN_TARGET_sandbox	:= $(shell $(CC) -dumpmachine)
BINDGEN_TARGET		:= $(BINDGEN_TARGET_$(SRCARCH))

# All warnings are inhibited since GCC builds are very experimental,
# many GCC warnings are not supported by Clang, they may only appear in
# some configurations, with new GCC versions, etc.
bindgen_extra_c_flags = -w --target=$(BINDGEN_TARGET)

# Auto variable zero-initialization requires an additional special option with
# clang that is going to be removed sometime in the future (likely in
# clang-18), so make sure to pass this option only if clang supports it
# (libclang major version < 16).
#
# https://github.com/llvm/llvm-project/issues/44842
# https://github.com/llvm/llvm-project/blob/llvmorg-16.0.0-rc2/clang/docs/ReleaseNotes.rst#deprecated-compiler-flags
ifdef CONFIG_INIT_STACK_ALL_ZERO
libclang_maj_ver=$(shell $(BINDGEN) $(srctree)/scripts/rust_is_available_bindgen_libclang.h 2>&1 | sed -ne 's/.*clang version \([0-9]*\).*/\1/p')
ifeq ($(shell expr $(libclang_maj_ver) \< 16), 1)
bindgen_extra_c_flags += -enable-trivial-auto-var-init-zero-knowing-it-will-be-removed-from-clang
endif
endif

bindgen_c_flags = $(filter-out $(bindgen_skip_c_flags), $(c_flags)) \
	$(bindgen_extra_c_flags)
endif

ifdef CONFIG_LTO
bindgen_c_flags_lto = $(filter-out $(CC_FLAGS_LTO), $(bindgen_c_flags))
else
bindgen_c_flags_lto = $(bindgen_c_flags)
endif

# `-fno-builtin` is passed to avoid `bindgen` from using `clang` builtin
# prototypes for functions like `memcpy` -- if this flag is not passed,
# `bindgen`-generated prototypes use `c_ulong` or `c_uint` depending on
# architecture instead of generating `usize`.
bindgen_c_flags_final = $(bindgen_c_flags_lto) -fno-builtin -D__BINDGEN__

# `--rust-target` points to our minimum supported Rust version.
quiet_cmd_bindgen = BINDGEN $@
      cmd_bindgen = \
	$(BINDGEN) $< $(bindgen_target_flags) --rust-target 1.85 \
		--use-core --with-derive-default --ctypes-prefix ffi --no-layout-tests \
		--no-debug '.*' --enable-function-attribute-detection \
		-o $@ -- $(bindgen_c_flags_final) -DMODULE \
		$(bindgen_target_cflags) $(bindgen_target_extra)

$(obj)/bindings/bindings_generated.rs: private bindgen_target_flags = \
    $(shell grep -Ev '^#|^$$' $(src)/bindgen_parameters)
$(obj)/bindings/bindings_generated.rs: private bindgen_target_extra = ; \
    sed -Ei 's/pub const RUST_CONST_HELPER_([a-zA-Z0-9_]*)/pub const \1/g' $@
$(obj)/bindings/bindings_generated.rs: $(src)/bindings/bindings_helper.h \
    $(src)/bindgen_parameters FORCE
	$(call if_changed_dep,bindgen)

$(obj)/uapi/uapi_generated.rs: private bindgen_target_flags = \
    $(shell grep -Ev '^#|^$$' $(src)/bindgen_parameters)
$(obj)/uapi/uapi_generated.rs: $(src)/uapi/uapi_helper.h \
    $(src)/bindgen_parameters FORCE
	$(call if_changed_dep,bindgen)

# See `CFLAGS_REMOVE_helpers.o` above. In addition, Clang on C does not warn
# with `-Wmissing-declarations` (unlike GCC), so it is not strictly needed here
# given it is `libclang`; but for consistency, future Clang changes and/or
# a potential future GCC backend for `bindgen`, we disable it too.
$(obj)/bindings/bindings_helpers_generated.rs: private bindgen_target_flags = \
    --blocklist-type '.*' --allowlist-var '' \
    --allowlist-function 'rust_helper_.*'
$(obj)/bindings/bindings_helpers_generated.rs: private bindgen_target_cflags = \
    -I$(objtree)/$(obj) -Wno-missing-prototypes -Wno-missing-declarations
$(obj)/bindings/bindings_helpers_generated.rs: private bindgen_target_extra = ; \
    sed -Ei 's/pub fn rust_helper_([a-zA-Z0-9_]*)/#[link_name="rust_helper_\1"]\n    pub fn \1/g' $@
$(obj)/bindings/bindings_helpers_generated.rs: $(src)/helpers/helpers.c FORCE
	$(call if_changed_dep,bindgen)

rust_exports = $(NM) -p --defined-only $(1) | awk '$$2~/(T|R|D|B)/ && $$3!~/__(pfx|cfi|odr_asan)/ { printf $(2),$$3 }'

quiet_cmd_exports = EXPORTS $@
      cmd_exports = \
	$(call rust_exports,$<,"EXPORT_SYMBOL_RUST_GPL(%s);\n") > $@

$(obj)/exports_core_generated.h: $(obj)/core.o FORCE
	$(call if_changed,exports)

# Even though Rust kernel modules should never use the bindings directly,
# symbols from the `bindings` crate and the C helpers need to be exported
# because Rust generics and inlined functions may not get their code generated
# in the crate where they are defined. Other helpers, called from non-inline
# functions, may not be exported, in principle. However, in general, the Rust
# compiler does not guarantee codegen will be performed for a non-inline
# function either. Therefore, we export all symbols from helpers and bindings.
# In the future, this may be revisited to reduce the number of exports after
# the compiler is informed about the places codegen is required.
$(obj)/exports_helpers_generated.h: $(obj)/helpers/helpers.o FORCE
	$(call if_changed,exports)

$(obj)/exports_bindings_generated.h: $(obj)/bindings.o FORCE
	$(call if_changed,exports)

$(obj)/exports_kernel_generated.h: $(obj)/kernel.o FORCE
	$(call if_changed,exports)

quiet_cmd_rustc_procmacro = $(RUSTC_OR_CLIPPY_QUIET) P $@
      cmd_rustc_procmacro = \
	$(RUSTC_OR_CLIPPY) $(rust_common_flags) $(rustc_target_flags) \
		-Clinker-flavor=gcc -Clinker=$(HOSTCC) \
		-Clink-args='$(call escsq,$(KBUILD_PROCMACROLDFLAGS))' \
		--emit=dep-info=$(depfile) --emit=link=$@ --extern proc_macro \
		--crate-type proc-macro \
		--crate-name $(patsubst lib%.$(libmacros_extension),%,$(notdir $@)) \
		@$(objtree)/include/generated/rustc_cfg $<

# Procedural macros can only be used with the `rustc` that compiled it.
$(obj)/$(libmacros_name): $(src)/macros/lib.rs FORCE
	+$(call if_changed_dep,rustc_procmacro)

$(obj)/$(libpin_init_internal_name): private rustc_target_flags = --cfg kernel
$(obj)/$(libpin_init_internal_name): $(src)/pin-init/internal/src/lib.rs FORCE
	+$(call if_changed_dep,rustc_procmacro)

quiet_cmd_rustc_library = $(if $(skip_clippy),RUSTC,$(RUSTC_OR_CLIPPY_QUIET)) L $@
      cmd_rustc_library = \
	OBJTREE=$(abspath $(objtree)) \
	$(if $(skip_clippy),$(RUSTC),$(RUSTC_OR_CLIPPY)) \
		$(filter-out $(skip_flags),$(rust_flags)) $(rustc_target_flags) \
		--emit=dep-info=$(depfile) --emit=obj=$@ \
		--emit=metadata=$(dir $@)$(patsubst %.o,lib%.rmeta,$(notdir $@)) \
		--crate-type rlib -L$(objtree)/$(obj) \
		--crate-name $(patsubst %.o,%,$(notdir $@)) $< \
		--sysroot=/dev/null \
	$(if $(rustc_objcopy),;$(OBJCOPY) $(rustc_objcopy) $@) \
	$(cmd_objtool)

rust-analyzer:
	$(Q)MAKEFLAGS= $(srctree)/scripts/generate_rust_analyzer.py \
		--cfgs='core=$(core-cfgs)' $(core-edition) \
		$(realpath $(srctree)) $(realpath $(objtree)) \
		$(rustc_sysroot) $(RUST_LIB_SRC) $(if $(KBUILD_EXTMOD),$(srcroot)) \
		> rust-project.json

redirect-intrinsics = \
	__addsf3 __eqsf2 __extendsfdf2 __gesf2 __lesf2 __ltsf2 __mulsf3 __nesf2 __truncdfsf2 __unordsf2 \
	__adddf3 __eqdf2 __ledf2 __ltdf2 __muldf3 __unorddf2 \
	__muloti4 __multi3 \
	__udivmodti4 __udivti3 __umodti3

ifdef CONFIG_ARM32
	# Add eabi initrinsics for ARM 32-bit
	redirect-intrinsics += \
		__aeabi_fadd __aeabi_fmul __aeabi_fcmpeq __aeabi_fcmple __aeabi_fcmplt __aeabi_fcmpun \
		__aeabi_dadd __aeabi_dmul __aeabi_dcmple __aeabi_dcmplt __aeabi_dcmpun \
		__aeabi_uldivmod
endif
ifneq ($(or $(CONFIG_ARM64),$(and $(CONFIG_RISCV),$(CONFIG_64BIT))),)
	# These intrinsics are defined for ARM64 and RISCV64
	redirect-intrinsics += \
		__ashrti3 \
		__ashlti3 __lshrti3
endif

define rule_rustc_library
	$(call cmd_and_fixdep,rustc_library)
	$(call cmd,gen_objtooldep)
endef

define rule_rust_cc_library
	$(call if_changed_rule,cc_o_c)
	$(call cmd,force_checksrc)
endef

# helpers.o uses the same export mechanism as Rust libraries, so ensure symbol
# versions are calculated for the helpers too.
$(obj)/helpers/helpers.o: $(src)/helpers/helpers.c $(recordmcount_source) FORCE
	+$(call if_changed_rule,rust_cc_library)

$(obj)/core.o: private skip_clippy = 1
$(obj)/core.o: private skip_flags = --edition=2021 -Wunreachable_pub
$(obj)/core.o: private rustc_objcopy = $(foreach sym,$(redirect-intrinsics),--redefine-sym $(sym)=__rust$(sym))
$(obj)/core.o: private rustc_target_flags = --edition=$(core-edition) -Aunused_variables $(core-cfgs)
$(obj)/core.o: $(RUST_LIB_SRC)/core/src/lib.rs \
    $(wildcard $(objtree)/include/config/RUSTC_VERSION_TEXT) FORCE
	+$(call if_changed_rule,rustc_library)
KCOV_INSTRUMENT_core.o := n

$(obj)/compiler_builtins.o: private rustc_objcopy = -w -W '__*'
$(obj)/compiler_builtins.o: $(src)/compiler_builtins.rs $(obj)/core.o FORCE
	+$(call if_changed_rule,rustc_library)

$(obj)/pin_init.o: private rustc_target_flags = --extern pin_init_internal \
    --extern macros --cfg kernel
$(obj)/pin_init.o: $(src)/pin-init/src/lib.rs $(obj)/compiler_builtins.o \
    $(obj)/$(libpin_init_internal_name) $(obj)/$(libmacros_name) FORCE
	+$(call if_changed_rule,rustc_library)

# Even if normally `build_error` is not a kernel object, it should still be
# treated as such so that we pass the same flags. Otherwise, for instance,
# `rustc` will complain about missing sanitizer flags causing an ABI mismatch.
$(obj)/build_error.o: private is-kernel-object := y
$(obj)/build_error.o: $(src)/build_error.rs $(obj)/compiler_builtins.o FORCE
	+$(call if_changed_rule,rustc_library)

$(obj)/ffi.o: $(src)/ffi.rs $(obj)/compiler_builtins.o FORCE
	+$(call if_changed_rule,rustc_library)

$(obj)/bindings.o: private rustc_target_flags = --extern ffi --extern pin_init
$(obj)/bindings.o: $(src)/bindings/lib.rs \
    $(obj)/ffi.o \
    $(obj)/pin_init.o \
    $(obj)/bindings/bindings_generated.rs \
    $(obj)/bindings/bindings_helpers_generated.rs FORCE
	+$(call if_changed_rule,rustc_library)

$(obj)/uapi.o: private rustc_target_flags = --extern ffi --extern pin_init
$(obj)/uapi.o: private skip_gendwarfksyms = 1
$(obj)/uapi.o: $(src)/uapi/lib.rs \
    $(obj)/ffi.o \
    $(obj)/pin_init.o \
    $(obj)/uapi/uapi_generated.rs FORCE
	+$(call if_changed_rule,rustc_library)

$(obj)/kernel.o: private rustc_target_flags = --extern ffi --extern pin_init \
    --extern build_error --extern macros --extern bindings --extern uapi
$(obj)/kernel.o: $(src)/kernel/lib.rs $(obj)/build_error.o $(obj)/pin_init.o \
    $(obj)/$(libmacros_name) $(obj)/bindings.o $(obj)/uapi.o FORCE
	+$(call if_changed_rule,rustc_library)

endif # CONFIG_RUST
