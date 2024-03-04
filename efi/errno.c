// SPDX-License-Identifier: GPL-2.0-only

#include <efi/efi-util.h>
#include <efi.h>
#include <errno.h>

const char *efi_strerror(efi_status_t err)
{
	const char *str;

	switch (err) {
	case EFI_SUCCESS: str = "Success"; break;
	case EFI_LOAD_ERROR: str = "Load Error"; break;
	case EFI_INVALID_PARAMETER: str = "Invalid Parameter"; break;
	case EFI_UNSUPPORTED: str = "Unsupported"; break;
	case EFI_BAD_BUFFER_SIZE: str = "Bad Buffer Size"; break;
	case EFI_BUFFER_TOO_SMALL: str = "Buffer Too Small"; break;
	case EFI_NOT_READY: str = "Not Ready"; break;
	case EFI_DEVICE_ERROR: str = "Device Error"; break;
	case EFI_WRITE_PROTECTED: str = "Write Protected"; break;
	case EFI_OUT_OF_RESOURCES: str = "Out of Resources"; break;
	case EFI_VOLUME_CORRUPTED: str = "Volume Corrupt"; break;
	case EFI_VOLUME_FULL: str = "Volume Full"; break;
	case EFI_NO_MEDIA: str = "No Media"; break;
	case EFI_MEDIA_CHANGED: str = "Media changed"; break;
	case EFI_NOT_FOUND: str = "Not Found"; break;
	case EFI_ACCESS_DENIED: str = "Access Denied"; break;
	case EFI_NO_RESPONSE: str = "No Response"; break;
	case EFI_NO_MAPPING: str = "No mapping"; break;
	case EFI_TIMEOUT: str = "Time out"; break;
	case EFI_NOT_STARTED: str = "Not started"; break;
	case EFI_ALREADY_STARTED: str = "Already started"; break;
	case EFI_ABORTED: str = "Aborted"; break;
	case EFI_ICMP_ERROR: str = "ICMP Error"; break;
	case EFI_TFTP_ERROR: str = "TFTP Error"; break;
	case EFI_PROTOCOL_ERROR: str = "Protocol Error"; break;
	case EFI_INCOMPATIBLE_VERSION: str = "Incompatible Version"; break;
	case EFI_SECURITY_VIOLATION: str = "Security Violation"; break;
	case EFI_CRC_ERROR: str = "CRC Error"; break;
	case EFI_END_OF_MEDIA: str = "End of Media"; break;
	case EFI_END_OF_FILE: str = "End of File"; break;
	case EFI_INVALID_LANGUAGE: str = "Invalid Language"; break;
	case EFI_COMPROMISED_DATA: str = "Compromised Data"; break;
	default: str = "unknown error";
	}

	return str;
}

int efi_errno(efi_status_t err)
{
	int ret;

	switch (err) {
	case EFI_SUCCESS: ret = 0; break;
	case EFI_LOAD_ERROR: ret = EIO; break;
	case EFI_INVALID_PARAMETER: ret = EINVAL; break;
	case EFI_UNSUPPORTED: ret = ENOTSUPP; break;
	case EFI_BAD_BUFFER_SIZE: ret = EINVAL; break;
	case EFI_BUFFER_TOO_SMALL: ret = EINVAL; break;
	case EFI_NOT_READY: ret = EAGAIN; break;
	case EFI_DEVICE_ERROR: ret = EIO; break;
	case EFI_WRITE_PROTECTED: ret = EROFS; break;
	case EFI_OUT_OF_RESOURCES: ret = ENOMEM; break;
	case EFI_VOLUME_CORRUPTED: ret = EIO; break;
	case EFI_VOLUME_FULL: ret = ENOSPC; break;
	case EFI_NO_MEDIA: ret = ENOMEDIUM; break;
	case EFI_MEDIA_CHANGED: ret = ENOMEDIUM; break;
	case EFI_NOT_FOUND: ret = ENODEV; break;
	case EFI_ACCESS_DENIED: ret = EACCES; break;
	case EFI_NO_RESPONSE: ret = ETIMEDOUT; break;
	case EFI_NO_MAPPING: ret = EINVAL; break;
	case EFI_TIMEOUT: ret = ETIMEDOUT; break;
	case EFI_NOT_STARTED: ret = EINVAL; break;
	case EFI_ALREADY_STARTED: ret = EINVAL; break;
	case EFI_ABORTED: ret = EINTR; break;
	case EFI_ICMP_ERROR: ret = EINVAL; break;
	case EFI_TFTP_ERROR: ret = EINVAL; break;
	case EFI_PROTOCOL_ERROR: ret = EPROTO; break;
	case EFI_INCOMPATIBLE_VERSION: ret = EINVAL; break;
	case EFI_SECURITY_VIOLATION: ret = EINVAL; break;
	case EFI_CRC_ERROR: ret = EINVAL; break;
	case EFI_END_OF_MEDIA: ret = EINVAL; break;
	case EFI_END_OF_FILE: ret = EINVAL; break;
	case EFI_INVALID_LANGUAGE: ret = EINVAL; break;
	case EFI_COMPROMISED_DATA: ret = EINVAL; break;
	default: ret = EINVAL;
	}

	return ret;
}
