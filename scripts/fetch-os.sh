#!/usr/bin/env bash

wgetopts=""
if [ -n "$GITHUB_ACTIONS" ]; then
	wgetopts="-nv"
fi

set -euo pipefail

declare -A images=(
	["debian-13.4.0-arm64-netinst.iso"]="https://cloud.debian.org/images/release/current/arm64/iso-cd/debian-13.4.0-arm64-netinst.iso"
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
