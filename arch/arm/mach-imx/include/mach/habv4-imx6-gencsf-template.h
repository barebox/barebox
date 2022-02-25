/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * This snippet can be included from a i.MX flash header configuration
 * file for generating signed images. The necessary keys/certificates
 * are expected in these config variables:
 *
 * CONFIG_HABV4_TABLE_BIN
 * CONFIG_HABV4_CSF_CRT_PEM
 * CONFIG_HABV4_IMG_CRT_PEM
 */

#ifndef SETUP_HABV4_ENGINE
#error "SETUP_HABV4_ENGINE undefined"
#endif

hab [Header]
hab Version = 4.1
hab Hash Algorithm = sha256
hab Engine Configuration = 0
hab Certificate Format = X509
hab Signature Format = CMS
hab Engine = SETUP_HABV4_ENGINE

hab [Install SRK]
hab File = CONFIG_HABV4_TABLE_BIN
hab # SRK index within SRK-Table 0..3
hab Source index = CONFIG_HABV4_SRK_INDEX

hab [Install CSFK]
/* target key index in keystore 1 */
hab File = CONFIG_HABV4_CSF_CRT_PEM

hab [Authenticate CSF]

hab [Unlock]
hab Engine = SETUP_HABV4_ENGINE
#ifdef SETUP_HABV4_FEATURES
hab Features = SETUP_HABV4_FEATURES
#endif

/*
// allow fusing FIELD_RETURN
// # ocotp0.permanent_write_enable=1
// # mw -l -d /dev/imx-ocotp 0xb8 0x1
hab [Unlock]
hab Engine = OCOTP
hab Features = FIELD RETURN
// device-specific UID:
// $ dd if=/sys/bus/nvmem/devices/imx-ocotp0/nvmem bs=4 skip=1 count=2 status=none | hexdump -ve '1/1 "0x%.2x, "' | sed 's/, $//'
hab UID = 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
*/

hab [Install Key]
/* verification key index in key store (0, 2...4) */
hab Verification index = 0
/* target key index in key store (2...4) */
hab Target index = 2
hab File = CONFIG_HABV4_IMG_CRT_PEM

hab [Authenticate Data]
/* verification key index in key store (2...4) */
hab Verification index = 2

hab_blocks

hab_encrypt [Install Secret Key]
hab_encrypt Verification index = 0
hab_encrypt Target index = 0
hab_encrypt_key
hab_encrypt_key_length 256
hab_encrypt_blob_address

hab_encrypt [Decrypt Data]
hab_encrypt Verification index = 0
hab_encrypt Mac Bytes = 16

hab_encrypt_blocks
