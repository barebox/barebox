#!/bin/sh

MKIMAGE=${MKIMAGE:-mkimage}
KGZIP=${KGZIP:-gzip}

set -e

if [ -z "${KBUILD_OUTPUT}" ] || [ -z "${KBUILD_DEFCONFIG}" ] ; then
	2>&1 echo "KBUILD_OUTPUT and KBUILD_DEFCONFIG must be set"
	exit 1
fi

rm -rf "${KBUILD_OUTPUT}/testfs/"
mkdir -p ${KBUILD_OUTPUT}/testfs

generate_fit()
{
    cat ${KBUILD_OUTPUT}/images/barebox-dt-2nd.img | \
	${KGZIP} -n -f -9 >${KBUILD_OUTPUT}/barebox-dt-2nd.img.gz

    cp .github/testfs/${KBUILD_DEFCONFIG}-gzipped.its ${KBUILD_OUTPUT}/

    find COPYING LICENSES/ | cpio -o -H newc | ${KGZIP} \
						   > ${KBUILD_OUTPUT}/ramdisk.cpio.gz

    ${MKIMAGE} -G $PWD/test/self/development_rsa2048.pem -r \
	       -f ${KBUILD_OUTPUT}/${KBUILD_DEFCONFIG}-gzipped.its \
	       ${KBUILD_OUTPUT}/testfs/barebox-gzipped.fit
}
[ -f .github/testfs/${KBUILD_DEFCONFIG}-gzipped.its ] && generate_fit

alias pad128k="dd if=/dev/zero bs=128k count=1 status=none"

generate_dm_verity()
{
    work=$(mktemp -d)
    cd ${work}

    # Create two dummy files; use lots of padding to make sure that
    # when we alter the contents of 'english' in root-bad, 'latin' is
    # still be readable, as their contents wont (a) share the same
    # hash block and (b) the block cache layer won't accedentally read
    # the invalid block.

    pad128k  >latin
    echo -n "veritas vos liberabit" >>latin
    pad128k >>latin

    pad128k  >english
    echo -n "truth will set you free" >>english
    pad128k >>english

    truncate -s 1M good.fat
    mkfs.vfat good.fat
    mcopy -i good.fat latin english ::

    veritysetup format \
		--root-hash-file=good.hash \
		good.fat good.verity

    sed 's/truth will set you free/LIAR LIAR PANTS ON FIRE/' \
	<good.fat >bad.fat

    cd -
    cp \
	${work}/good.fat \
	${work}/good.verity \
	${work}/good.hash \
	${work}/bad.fat \
	${KBUILD_OUTPUT}/testfs

    rm -rf ${work}
}
generate_dm_verity
