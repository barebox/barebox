#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
#
# rk-otp.sh - Print the key hash that needs to be written to the OTP of a
# Rockchip SoC to enable secure boot.

set -e

if [ "$#" -lt "1" ]; then
    echo "Usage: $0 [FILE]>"
    exit 1
fi

FILE=$1

# Pad INPUT to SIZE bytes and reverse byte order
pad_reverse () {
    SIZE=$1
    INPUT=$2

    # A byte consists of two hex values
    SIZE=$((SIZE * 2))

    # Pad using sed since numbers are too large
    PAD=$(printf "%0${SIZE}x" 0 | sed -nE "s/0{${#INPUT}}$/${INPUT}/p")

    # TODO Replace bashism with POSIX sh
    REVERSE=""
    for (( i = 0; i < SIZE; i += 2 )); do
        REVERSE+="${PAD:${SIZE} - 2 - $i:2}"
    done

    echo "$REVERSE"
}

rkss_read () {
    RKSS=$1

    # Extract the public key from the image
    xxd -ps -s 512 -l 560 "$RKSS"
}

pem_read () {
    PEM=$1

    KEY=$(openssl rsa -in "$PEM" -pubin -modulus -text -noout)
    # Extract size of key in bits
    KEY_SIZE=$(echo "$KEY" | sed -nE 's/Public-Key: \(([0-9]+) bit\)/\1/p')

    # Extract modulus as hex value
    MODULUS=$(echo "$KEY" | sed -nE 's/Modulus=([0-9ABCDEF]+)/\1/p')
    # Extract exponent and convert it to hex value
    EXPONENT=$(echo "$KEY" | sed -nE 's/Exponent: ([0-9]+) (.*)/obase=16;\1/p' | BC_LINE_LENGTH=0 bc)
    # Calculate acceleration factor as hex value
    NP=$(echo "ibase=16;modulus=$MODULUS;ibase=A;obase=16;2 ^ ($KEY_SIZE + 132) / modulus" | BC_LINE_LENGTH=0 bc)

    # Build the public key with padding in reverse byte order
    pad_reverse 512 "$MODULUS"
    pad_reverse 16 "$EXPONENT"
    pad_reverse 32 "$NP"
}

if [ "$(head -c 4 "$FILE")" = "RKSS" ]; then
    KEYHEX=$(rkss_read "$FILE")
else
    KEYHEX=$(pem_read "$FILE")
fi

# Convert hex format of public key to binary and calculate sha256 as hex
echo "$KEYHEX" | xxd -r -p | sha256sum | sed -nE 's/([0-9abcdef]+).*/\1/p'
