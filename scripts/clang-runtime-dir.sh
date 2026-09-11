#!/bin/sh

if [ $# -eq 0 ]; then
	# unquoted on purpose: $CC may carry a wrapper, e.g. "sccache clang-21"
	set -- ${CC}
fi

if [ $# -eq 0 ]; then
	echo "Error: No compiler specified." >&2
	printf "Usage:\n\t$0 <clang-command>\n" >&2
	exit 1
fi

rundir=$("$@" --print-runtime-dir 2>/dev/null)
if [ ! -e "$rundir" ]; then
	# Workaround https://github.com/llvm/llvm-project/issues/112458
	guess=$(dirname "$rundir")/linux
	if [ -e "$guess" ]; then
		rundir="$guess"
	fi
fi
echo "$rundir"
