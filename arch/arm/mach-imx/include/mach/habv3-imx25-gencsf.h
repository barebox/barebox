/*
 * This snippet can be included from a i.MX flash header configuration
 * file for generating signed images. The necessary keys/certificates
 * are expected in these config variables:
 *
 * CONFIG_HABV3_SRK_PEM
 * CONFIG_HABV3_SRK_PEM
 * CONFIG_HABV3_IMG_CRT_PEM
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
hab File = CONFIG_HABV3_CSF_CRT_DER

hab [Authenticate CSF]
/* below is the command that unlock the access to the DryIce registers */

hab [Write Data]
hab Width = 4
hab Address Data = 0x53FFC03C 0xCA693569

hab [Install Key]
hab Verification index = 1
hab Target index = 2
hab File = CONFIG_HABV3_IMG_CRT_DER

hab [Authenticate Data]
hab Verification index = 2

hab_blocks
