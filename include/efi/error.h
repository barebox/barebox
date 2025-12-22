/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __EFI_ERROR_H_
#define __EFI_ERROR_H_

#include <asm/bitsperlong.h>
#include <efi/types.h>

/* Bit mask for EFI status code with error */
#define EFI_ERROR_MASK (1UL << (BITS_PER_LONG-1))

#define EFI_SUCCESS                       0
#define EFI_LOAD_ERROR                  ( 1 | EFI_ERROR_MASK)
#define EFI_INVALID_PARAMETER           ( 2 | EFI_ERROR_MASK)
#define EFI_UNSUPPORTED                 ( 3 | EFI_ERROR_MASK)
#define EFI_BAD_BUFFER_SIZE             ( 4 | EFI_ERROR_MASK)
#define EFI_BUFFER_TOO_SMALL            ( 5 | EFI_ERROR_MASK)
#define EFI_NOT_READY                   ( 6 | EFI_ERROR_MASK)
#define EFI_DEVICE_ERROR                ( 7 | EFI_ERROR_MASK)
#define EFI_WRITE_PROTECTED             ( 8 | EFI_ERROR_MASK)
#define EFI_OUT_OF_RESOURCES            ( 9 | EFI_ERROR_MASK)
#define EFI_VOLUME_CORRUPTED            (10 | EFI_ERROR_MASK)
#define EFI_VOLUME_FULL                 (11 | EFI_ERROR_MASK)
#define EFI_NO_MEDIA                    (12 | EFI_ERROR_MASK)
#define EFI_MEDIA_CHANGED               (13 | EFI_ERROR_MASK)
#define EFI_NOT_FOUND                   (14 | EFI_ERROR_MASK)
#define EFI_ACCESS_DENIED               (15 | EFI_ERROR_MASK)
#define EFI_NO_RESPONSE                 (16 | EFI_ERROR_MASK)
#define EFI_NO_MAPPING                  (17 | EFI_ERROR_MASK)
#define EFI_TIMEOUT                     (18 | EFI_ERROR_MASK)
#define EFI_NOT_STARTED                 (19 | EFI_ERROR_MASK)
#define EFI_ALREADY_STARTED             (20 | EFI_ERROR_MASK)
#define EFI_ABORTED                     (21 | EFI_ERROR_MASK)
#define EFI_ICMP_ERROR                  (22 | EFI_ERROR_MASK)
#define EFI_TFTP_ERROR                  (23 | EFI_ERROR_MASK)
#define EFI_PROTOCOL_ERROR              (24 | EFI_ERROR_MASK)
#define EFI_INCOMPATIBLE_VERSION        (25 | EFI_ERROR_MASK)
#define EFI_SECURITY_VIOLATION          (26 | EFI_ERROR_MASK)
#define EFI_CRC_ERROR                   (27 | EFI_ERROR_MASK)
#define EFI_END_OF_MEDIA                (28 | EFI_ERROR_MASK)
#define EFI_END_OF_FILE                 (31 | EFI_ERROR_MASK)
#define EFI_INVALID_LANGUAGE            (32 | EFI_ERROR_MASK)
#define EFI_COMPROMISED_DATA            (33 | EFI_ERROR_MASK)

#define EFI_WARN_UNKNOWN_GLYPH		1
#define EFI_WARN_DELETE_FAILURE		2
#define EFI_WARN_WRITE_FAILURE		3
#define EFI_WARN_BUFFER_TOO_SMALL	4
#define EFI_WARN_STALE_DATA		5
#define EFI_WARN_FILE_SYSTEM		6
#define EFI_WARN_RESET_REQUIRED		7

#define EFI_ERROR(a)	(((signed long) a) < 0)

const char *efi_strerror(efi_status_t err);
int efi_errno(efi_status_t err);

#endif
