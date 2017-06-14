/*
 * This snippet can be included from a i.MX flash header configuration
 * file for generating signed images. The necessary keys/certificates
 * are expected in these config variables:
 *
 * CONFIG_HABV3_SRK_PEM
 * CONFIG_HABV3_CSF_CRT_DER
 * CONFIG_HABV3_IMG_CRT_DER
 */
super_root_key CONFIG_HABV3_SRK_PEM

hab [Header]
hab Version = 3.0
hab Security Configuration = Production
hab Hash Algorithm = SHA256
hab Engine = RTIC
hab Certificate Format = WTLS
hab Signature Format = PKCS1
hab UID = Generic
hab Code = 0x00

hab [Install SRK]
hab File = "not-used"

hab [Install CSFK]
/* target key index in keystore 1 */
hab File = CONFIG_HABV3_CSF_CRT_DER

hab [Authenticate CSF]

/* unlock the access to the DryIce registers */
hab [Write Data]
hab Width = 4
hab Address Data = 0x53FFC03C 0xCA693569

hab [Install Key]
/* verification key index in key store (1...4) */
/* in contrast to documentation 0 seems to be valid, too */
hab Verification index = 1
/* target key index in key store (1...4) */
hab Target index = 2
hab File = CONFIG_HABV3_IMG_CRT_DER

hab [Authenticate Data]
/* verification key index in key store (2...4) */
hab Verification index = 2

hab_blocks
