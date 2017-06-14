/*
 * This snippet can be included from a i.MX flash header configuration
 * file for generating signed images. The necessary keys/certificates
 * are expected in these config variables:
 *
 * CONFIG_HABV4_TABLE_BIN
 * CONFIG_HABV4_CSF_CRT_PEM
 * CONFIG_HABV4_IMG_CRT_PEM
 */

hab [Header]
hab Version = 4.1
hab Hash Algorithm = sha256
hab Engine Configuration = 0
hab Certificate Format = X509
hab Signature Format = CMS
hab Engine = CAAM

hab [Install SRK]
hab File = CONFIG_HABV4_TABLE_BIN
hab # SRK index within SRK-Table 0..3
hab Source index = 0

hab [Install CSFK]
/* target key index in keystore 1 */
hab File = CONFIG_HABV4_CSF_CRT_PEM

hab [Authenticate CSF]

hab [Unlock]
hab Engine = CAAM
hab Features = RNG

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
