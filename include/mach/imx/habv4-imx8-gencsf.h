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
#if defined(CONFIG_HABV4) && defined(CONFIG_CPU_64)
#if defined(CONFIG_HABV4_QSPI)
hab_qspi
#endif
hab [Header]
hab Version = 4.3
hab Hash Algorithm = sha256
hab Engine Configuration = 0
hab Certificate Format = X509
hab Signature Format = CMS
hab Engine = CAAM

hab [Install SRK]
hab File = CONFIG_HABV4_TABLE_BIN
hab # SRK index within SRK-Table 0..3
hab Source index = CONFIG_HABV4_SRK_INDEX

hab [Install CSFK]
/* target key index in keystore 1 */
hab File = CONFIG_HABV4_CSF_CRT_PEM

hab [Authenticate CSF]

hab [Unlock]
hab Engine = CAAM
hab Features = RNG, MID

#if defined(CONFIG_HABV4_CSF_SRK_REVOKE_UNLOCK)
hab [Unlock]
hab Engine = OCOTP
hab Features = SRK REVOKE
#endif

#if defined(CONFIG_HABV4_CSF_UNLOCK_FIELD_RETURN)
hab [Unlock]
hab Engine = OCOTP
hab Features = FIELD RETURN
hab UID = HABV4_CSF_UNLOCK_UID
#endif

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
#endif
