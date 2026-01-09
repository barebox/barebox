#!/usr/bin/env bash

wgetopts=""
if [ -n "$GITHUB_ACTIONS" ]; then
	wgetopts="-nv"
fi

set -euo pipefail

declare -A images=(
	["debian-13-nocloud-arm64.qcow2"]="https://cloud.debian.org/images/cloud/trixie/latest/debian-13-nocloud-arm64.qcow2"
)

found=0
dl=0

for image in "${!images[@]}"; do
	if [ -e "$image" ]; then
		((++found))
	else
		wget -c $wgetopts -O "$image" "${images[$image]}"
		((++dl))
	fi
done

if [ "$found" -eq 0 ]; then
	echo -n "No images found. ";
else
	echo -n "Found $found images(s). ";
fi
if [ "$dl" -eq 0 ]; then
	echo "Nothing needed to be downloaded.";
else
	echo "$dl missing images downloaded.";
fi
