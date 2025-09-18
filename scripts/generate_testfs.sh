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

if [ -f .github/testfs/${KBUILD_DEFCONFIG}-gzipped.its ]; then
	generate_fit
fi
