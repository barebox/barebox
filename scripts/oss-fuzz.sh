#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Build the fuzz targets the way OSS-Fuzz expects, so barebox can be
# fuzzed there by a projects/barebox/build.sh that just runs this script.
# The OSS-Fuzz variables (SANITIZER, CFLAGS, WORK, OUT, LIB_FUZZING_ENGINE)
# have defaults, so it runs locally as well:
#
#   ./scripts/oss-fuzz.sh
#   ./oss-fuzz-work/out/fuzz-filetype
#
# LLVM selects the toolchain like for kbuild, CORPORA points at a
# barebox-fuzz-corpora checkout to package seed corpora from.

set -eux

srctree=$(readlink -f "$(dirname "$0")/..")
SANITIZER=${SANITIZER:-all}
WORK=${WORK:-$srctree/oss-fuzz-work}
OUT=${OUT:-$WORK/out}
CORPORA=${CORPORA:-${SRC:-$(dirname "$srctree")}/barebox-fuzz-corpora}
BUILD=$WORK/build

# Only libFuzzer is supported: e.g. AFL++ instruments via its own $CC,
# which kbuild overrides
if [ "${FUZZING_ENGINE:-libfuzzer}" != libfuzzer ]; then
	echo "Fuzzing engine $FUZZING_ENGINE unsupported" >&2
	exit 1
fi

# LLVM=1 needs unsuffixed tools, but e.g. Debian may lack an unsuffixed
# ld.lld, so fall back to the newest complete versioned toolchain
if [ -z "${LLVM:-}" ]; then
	LLVM=1
	for v in '' $(compgen -c clang- | sed -n 's/^clang\(-[0-9]\+\)$/\1/p' |
		      sort -rV); do
		if type -P clang$v ld.lld$v llvm-ar$v >/dev/null; then
			LLVM=${v:-1}
			break
		fi
	done
fi

if [ -z "${CFLAGS:-}" ]; then
	# -O0 breaks BUILD_BUG_ON(!__builtin_constant_p(...))
	CFLAGS="-O1 -g -fno-omit-frame-pointer -fno-sanitize-recover=undefined"
	case "$SANITIZER" in
	all) CFLAGS+=" -fsanitize=address,undefined" ;;
	address|undefined) CFLAGS+=" -fsanitize=$SANITIZER" ;;
	coverage) CFLAGS+=" -fprofile-instr-generate -fcoverage-mapping" ;;
	esac
fi

kmake() {
	make -C "$srctree" O="$BUILD" LLVM="$LLVM" "$@"
}

cfg() {
	"$srctree"/scripts/config --file "$BUILD/.config" "$@"
}

mkdir -p "$WORK" "$OUT"
kmake libfuzzer_defconfig

# OSS-Fuzz builds one sanitizer at a time: keep only the matching Kconfig
# option, as it also enables barebox-specific ASan/UBSan support
case "$SANITIZER" in
all) ;;
address) cfg -d UBSAN ;;
undefined) cfg -d ASAN ;;
memory) echo "MSan unsupported: sandbox links the host libc" >&2; exit 1 ;;
*) cfg -d ASAN -d UBSAN ;;
esac
kmake olddefconfig

# -fsanitize=fuzzer would also link libFuzzer's interceptors, whose
# memcmp & co. clash with barebox' own, so link just the runtime archive.
# Unlike the sandbox Makefile's fallback, this knows both runtime layouts
LIB_FUZZING_ENGINE=${LIB_FUZZING_ENGINE:--fsanitize=fuzzer}
if [ "$LIB_FUZZING_ENGINE" = -fsanitize=fuzzer ]; then
	rundir=$(sed -n 's/^CONFIG_CLANG_RUNTIME_DIR="\(.*\)"$/\1/p' \
		 "$BUILD/.config")
	LIB_FUZZING_ENGINE=$rundir/libclang_rt.fuzzer.a
	if [ ! -e "$LIB_FUZZING_ENGINE" ]; then
		LIB_FUZZING_ENGINE=$rundir/libclang_rt.fuzzer-$(uname -m).a
	fi
	export LIB_FUZZING_ENGINE
fi

# The sandbox Makefile appends to BAREBOX_LDFLAGS from the environment,
# so the link pulls in the sanitizer and profile runtimes, too
export BAREBOX_LDFLAGS=$CFLAGS
kmake KCFLAGS="$CFLAGS" -j"$(nproc)"

# The binary picks the target by its basename. Install copies, as OSS-Fuzz
# repacks $OUT per target, which would break symlinks
targets=$("$BUILD/images/barebox" --list-fuzzers)
for t in $targets; do
	cp "$BUILD/images/barebox" "$OUT/fuzz-$t"
	rm -f "$OUT/fuzz-${t}_seed_corpus.zip"
	if [ -d "$CORPORA/$t" ]; then
		zip -jqr "$OUT/fuzz-${t}_seed_corpus.zip" "$CORPORA/$t"
	fi
done
