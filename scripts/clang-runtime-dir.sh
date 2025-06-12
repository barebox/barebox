#!/bin/sh

CC=${1:-${CC}}

if [ "${CC}" = "" ]; then
	echo "Error: No compiler specified." >&2
	printf "Usage:\n\t$0 <clang-command>\n" >&2
	exit 1
fi

rundir=$("${CC}" --print-runtime-dir 2>/dev/null)
if [ ! -e "$rundir" ]; then
	# Workaround https://github.com/llvm/llvm-project/issues/112458
	guess=$(dirname "$rundir")/linux
	if [ -e "$guess" ]; then
		rundir="$guess"
	fi
fi
echo "$rundir"
