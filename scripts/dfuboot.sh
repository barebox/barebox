#!/bin/bash

DEVICETREE=
KERNEL=
CMDLINE=

usage() {
	echo "usage: $0 [OPTIONS]"
	echo "This script uploads a kernel and optionally a devicetree"
	echo "and a kernel commandline to barebox via DFU running the"
	echo "'boot dfu' command."
	echo "OPTIONS:"
	echo "  -k <kernel>       kernelimage to upload"
	echo "  -d <dtb>          devicetree binary blob to upload"
	echo "  -c \"cmdline\"      kernel commandline"
	echo "  -h                This help text"

	exit 0
}

while getopts "k:d:c:h" opt
do
	case "$opt" in
	h)
		usage
		;;
	d)
		DEVICETREE="$OPTARG"
		;;
	k)
		KERNEL="$OPTARG"
		;;
	c)
		CMDLINE="$OPTARG"
		;;
	esac
done

dfu-util -D "${KERNEL}" -a kernel
if [ $? != 0 ]; then
	echo "Failed to upload kernel"
	exit 1
fi

if [ -n "$DEVICETREE" ]; then
	dfu-util -D "${DEVICETREE}" -a dtb
	if [ $? != 0 ]; then
		echo "Failed to upload devicetree"
		exit 1
	fi
fi

if [ -n "$CMDLINE" ]; then
	cmdlinefile=$(mktemp)

	echo -e "$CMDLINE" > "${cmdlinefile}"

	dfu-util -D "${cmdlinefile}" -a cmdline -R
	result=$?

	rm -f "${cmdlinefile}"

	if [ $result != 0 ]; then
		echo "Failed to upload cmdline"
		exit 1
	fi

fi
