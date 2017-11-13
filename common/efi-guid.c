#include <common.h>
#include <efi.h>

efi_guid_t efi_file_info_id = EFI_FILE_INFO_GUID;
efi_guid_t efi_simple_file_system_protocol_guid = EFI_SIMPLE_FILE_SYSTEM_GUID;
efi_guid_t efi_device_path_protocol_guid = EFI_DEVICE_PATH_PROTOCOL_GUID;
efi_guid_t efi_loaded_image_protocol_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
efi_guid_t efi_unknown_device_guid = EFI_UNKNOWN_DEVICE_GUID;
efi_guid_t efi_null_guid = EFI_NULL_GUID;
efi_guid_t efi_global_variable_guid = EFI_GLOBAL_VARIABLE_GUID;
efi_guid_t efi_block_io_protocol_guid = EFI_BLOCK_IO_PROTOCOL_GUID;
efi_guid_t efi_barebox_vendor_guid = EFI_BAREBOX_VENDOR_GUID;
efi_guid_t efi_systemd_vendor_guid = EFI_SYSTEMD_VENDOR_GUID;

#define EFI_GUID_STRING(guid, short, long) do {	\
	if (!efi_guidcmp(guid, *g))		\
		return long;			\
	} while(0)

const char *efi_guid_string(efi_guid_t *g)
{
	EFI_GUID_STRING(EFI_NULL_GUID, "NULL", "NULL GUID");
	EFI_GUID_STRING(EFI_MPS_TABLE_GUID, "MPS Table", "MPS Table GUID in EFI System Table");
	EFI_GUID_STRING(EFI_ACPI_TABLE_GUID, "ACPI Table", "ACPI 1.0 Table GUID in EFI System Table");
	EFI_GUID_STRING(EFI_ACPI_20_TABLE_GUID, "ACPI 2.0 Table", "ACPI 2.0 Table GUID in EFI System Table");
	EFI_GUID_STRING(EFI_SMBIOS_TABLE_GUID, "SMBIOS Table", "SMBIOS Table GUID in EFI System Table");
	EFI_GUID_STRING(EFI_SAL_SYSTEM_TABLE_GUID, "SAL System Table", "SAL System Table GUID in EFI System Table");
	EFI_GUID_STRING(EFI_HCDP_TABLE_GUID, "HDCP Table", "HDCP Table GUID in EFI System Table");
	EFI_GUID_STRING(EFI_UGA_IO_PROTOCOL_GUID, "UGA Protocol", "EFI 1.1 UGA Protocol");
	EFI_GUID_STRING(EFI_GLOBAL_VARIABLE_GUID, "Efi", "Efi Variable GUID");
	EFI_GUID_STRING(EFI_UV_SYSTEM_TABLE_GUID, "SAL System Table", "SAL System Table GUID in EFI System Table");
	EFI_GUID_STRING(EFI_LINUX_EFI_CRASH_GUID, "Linux EFI Crash", "Linux EFI Crash GUID");
	EFI_GUID_STRING(EFI_LOADED_IMAGE_PROTOCOL_GUID, "LoadedImage Protocol", "EFI 1.0 Loaded Image Protocol");
	EFI_GUID_STRING(EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID, "EFI Graphics Output Protocol", "UEFI Graphics Output Protocol");
	EFI_GUID_STRING(EFI_UGA_PROTOCOL_GUID, "UGA Draw Protocol", "EFI 1.1 UGA Draw Protocol");
	EFI_GUID_STRING(EFI_UGA_IO_PROTOCOL_GUID, "UGA Protocol", "EFI 1.1 UGA Protocol");
	EFI_GUID_STRING(EFI_PCI_IO_PROTOCOL_GUID, "PCI IO Protocol", "EFI 1.1 PCI IO Protocol");
	EFI_GUID_STRING(EFI_USB_IO_PROTOCOL_GUID, "USB IO Protocol", "EFI 1.0 USB IO Protocol");
	EFI_GUID_STRING(EFI_FILE_INFO_GUID, "File Info", "EFI File Infom");
	EFI_GUID_STRING(EFI_SIMPLE_FILE_SYSTEM_GUID, "Filesystem", "EFI 1.0 Simple FileSystem");
	EFI_GUID_STRING(EFI_DEVICE_TREE_GUID, "Device Tree", "EFI Device Tree GUID");
	EFI_GUID_STRING(EFI_DEVICE_PATH_PROTOCOL_GUID, "Device Path Protocol", "EFI 1.0 Device Path protocol");
	EFI_GUID_STRING(EFI_SIMPLE_NETWORK_PROTOCOL_GUID, "Simple Network Protocol", "EFI 1.0 Simple Network Protocol");
	EFI_GUID_STRING(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, "Filesystem Protocol", "EFI 1.0 Simple FileSystem Protocol");
	EFI_GUID_STRING(EFI_UNKNOWN_DEVICE_GUID, "Efi Unknown Device", "Efi Unknown Device GUID");
	EFI_GUID_STRING(EFI_BLOCK_IO_PROTOCOL_GUID, "BlockIo Protocol", "EFI 1.0 Block IO protocol");

	EFI_GUID_STRING(EFI_FIRMWARE_VOLUME2_PROTOCOL_GUID, "FirmwareVolume2Protocol", "Efi FirmwareVolume2Protocol");
	EFI_GUID_STRING(EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL_GUID, "FirmwareVolumeBlock Protocol", "Firmware Volume Block protocol");
	EFI_GUID_STRING(EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_GUID, "PciRootBridgeIo Protocol", "EFI 1.1 Pci Root Bridge IO Protocol");
	EFI_GUID_STRING(EFI_ISA_ACPI_PROTOCOL_GUID, "ISA Acpi Protocol", "ISA Acpi Protocol");
	EFI_GUID_STRING(EFI_ISA_IO_PROTOCOL_GUID, "ISA IO Protocol", "ISA IO Protocol");
	EFI_GUID_STRING(EFI_STANDARD_ERROR_DEVICE_GUID, "Standard Error Device Guid", "EFI Standard Error Device Guid");
	EFI_GUID_STRING(EFI_CONSOLE_OUT_DEVICE_GUID, "Console Out Device Guid", "EFI Console Out Device Guid");
	EFI_GUID_STRING(EFI_CONSOLE_IN_DEVICE_GUID, "Console In Device Guid", "EFI Conosle In Device Guid");
	EFI_GUID_STRING(EFI_SIMPLE_TEXT_OUT_PROTOCOL_GUID, "Simple Text Out Protocol", "EFI 1.0 Simple Text Out Protocol");
	EFI_GUID_STRING(EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID, "Simple Text Input Ex Protocol", "UEFI 2.1 Simple Text Input Ex Protocol");
	EFI_GUID_STRING(EFI_SIMPLE_TEXT_IN_PROTOCOL_GUID, "Simple Text In Protocol", "EFI 1.0 Simple Text In Protocol");
	EFI_GUID_STRING(EFI_DISK_IO_PROTOCOL_GUID, "DiskIo Protocol", "EFI 1.0 Disk IO Protocol");
	EFI_GUID_STRING(EFI_IDE_CONTROLLER_INIT_PROTOCOL_GUID, "IDE Controller Init Protocol", "Platform IDE Init Protocol");
	EFI_GUID_STRING(EFI_DISK_INFO_PROTOCOL_GUID, "Disk Info Protocol", "Disk Info Protocol");
	EFI_GUID_STRING(EFI_SERIAL_IO_PROTOCOL_GUID, "SerialIo Protocol", "EFI 1.0 Serial IO Protocol");
	EFI_GUID_STRING(EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL_GUID, "Bus Specific Driver Override Protocol", "EFI 1.1 Bus Specific Driver Override Protocol");
	EFI_GUID_STRING(EFI_LOAD_FILE2_PROTOCOL_GUID, "LoadFile2 Protocol", "EFI Load File 2 Protocol");
	EFI_GUID_STRING(EFI_MTFTP4_SERVICE_BINDING_PROTOCOL_GUID, "MTFTP4 Service Binding Protocol", "MTFTP4 Service Binding Protocol");
	EFI_GUID_STRING(EFI_DHCP4_PROTOCOL_GUID, "DHCP4 Protocol", "DHCP4 Protocol");
	EFI_GUID_STRING(EFI_UDP4_SERVICE_BINDING_PROTOCOL_GUID, "UDP4 Service Binding Protocol", "UDP4 Service Binding Protocol");
	EFI_GUID_STRING(EFI_TCP4_SERVICE_BINDING_PROTOCOL_GUID, "TCP4 Service Binding Protocol", "TCP4 Service Binding Protocol");
	EFI_GUID_STRING(EFI_IP4_SERVICE_BINDING_PROTOCOL_GUID, "IP4 Service Binding Protocol", "IP4 Service Binding Protocol");
	EFI_GUID_STRING(EFI_IP4_CONFIG_PROTOCOL_GUID, "Ip4Config Protocol", "Ip4Config Protocol");
	EFI_GUID_STRING(EFI_ARP_SERVICE_BINDING_PROTOCOL_GUID, "ARP Service Binding Protocol", "ARP Service Binding Protocol");
	EFI_GUID_STRING(EFI_MANAGED_NETWORK_SERVICE_BINDING_PROTOCOL_GUID, "Managed Network Service Binding Protocol", "Managed Network Service Binding Protocol");
	EFI_GUID_STRING(EFI_VLAN_CONFIG_PROTOCOL_GUID, "VlanConfig Protocol", "VlanConfig Protocol");
	EFI_GUID_STRING(EFI_HII_CONFIG_ACCESS_PROTOCOL_GUID, "HII Config Access Protocol", "HII Config Access 2.1 protocol");
	EFI_GUID_STRING(LOAD_FILE_PROTOCOL_GUID, "LoadFile Protocol", "EFI 1.0 Load File Protocol");
	EFI_GUID_STRING(EFI_COMPONENT_NAME2_PROTOCOL_GUID, "Component Name2 Protocol", "UEFI 2.0 Component Name2 Protocol");
	EFI_GUID_STRING(EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_GUID_31, "Network Interface Identifier Protocol_31",  "EFI1.1 Network Interface Identifier Protocol");
	EFI_GUID_STRING(EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_GUID, "Network Interface Identifier Protocol", "EFI Network Interface Identifier Protocol");
	EFI_GUID_STRING(EFI_TIMESTAMP_PROTOCOL_GUID, "Timestamp", "Timestamp");

	/* TPM 1.2 */
	EFI_GUID_STRING( EFI_TCG_PROTOCOL_GUID, "TcgService", "TCGServices Protocol");
	/* TPM 2.0 */
	EFI_GUID_STRING( EFI_TCG2_PROTOCOL_GUID, "Tcg2Service", "TCG2Services Protocol");

	/* File */
	EFI_GUID_STRING(EFI_IDEBUSDXE_INF_GUID, "IdeBusDxe.inf", "EFI IdeBusDxe.inf File GUID");
	EFI_GUID_STRING(EFI_TERMINALDXE_INF_GUID, "TerminalDxe.inf", "EFI TerminalDxe.inf File GUID");
	EFI_GUID_STRING(EFI_ISCSIDXE_INF_GUID, "IScsiDxe.inf", "EFI IScsiDxe.inf File GUID");
	EFI_GUID_STRING(EFI_VLANCONFIGDXE_INF_GUID, "VlanConfigDxe.inf", "EFI VlanConfigDxe.inf File GUID");

	return "unknown";
}
