Amazon Kindle 4/5 Model No. D01100, D01200 and EY21
===================================================

The Kindle Model No. D01100 (Kindle Wi-Fi), D01200 (Kindle Touch)
and EY21 (Paperwhite) are refered as the Kindle 4th and 5th generation.
Those e-book readers share a common set of hardware:

* a Freescale i.MX50 SOC
* 2 or 4GiB eMMC
* a MC13892 PMIC

The older readers D01100 and D01200 use 256MiB of LPDDR1,
while the newer EY21 uses 256MiB of LPDDR2.

The devices boot up in internal boot mode from an eMMC boot partition and
are shipped with a vendor modified u-boot imximage based on u-boot v2009.08.

To upload and run a new bootloader the older devices can be put into
USB-downloader mode by the SOC microcode when a specific key is pressed during
startup:

* the fiveway down button on the model D01100
* the home button on model D01200

A new USB device "NS Blank CODEX" should appear, barebox may be uploaded using

::

        $ scripts/imx/imx-usb-loader barebox-kindle-d01100.img
        $ scripts/imx/imx-usb-loader barebox-kindle-d01200.img

Hint: keep the select button pressed down to get the barebox USB console.

Barebox may be used as drop-in replacement for the shipped bootloader, when
the imximg fits into 258048 bytes. When installing the barebox imximg on
the eMMC, take care not to overwrite the vendor supplied serial numbers stored
on the eMMC,
e.g. for the D01100 just write the imx-header and the application section::

        loady -t usbserial
        memcpy -b -s barebox-kindle-d01100.img -d /dev/disk0.boot0.imx_header 1024 0 2048
        memcpy -b -s barebox-kindle-d01100.img -d /dev/disk0.boot0.self 4096 0 253952

Note: a USB serial ACM console will be launched by a barebox init script
when

* the cursor select key is pressed during startup of model D01100
* the home button is pressed within a second after startup of model D01200.
  If you press the home button during startup, you will enter USB boot mode.
* the EY21 has no keys to press, a USB console will be launched for 10s.

This device is battery-powered and there is no way to switch the device off.
When the device is inactive, the kindle software will first reduce the
power consumption to a few milliamps of battery power, after some minutes
the power consumption is further reduced to about 550 microamps. Switching
on iomux pullups may significantly reduce your standby-time.

Hints to reduce the build image size
------------------------------------

Note that a drop-in replacement barebox imximage must not exceed 258048 bytes
since the space behind it is in use. Hence, don't build in drivers and FS
that are not required, e.g.
``NET, DISK_AHCI, DISK_INTF_PLATFORM_IDE, DISK_ATA, VIDEO, PWM, LED,
USB_STORAGE, USB_ULPI, NAND, MTD_UBI, FS_UBIFS, MFD_MC34704, MFD_MC9SDZ60,
MFD_STMPE, EEPROM_AT25, EEPROM_AT24, KEYBOARD_GPIO, PARTITION_DISK_EFI``

Also unselect support for other boards to get rid of their dependencies.
Further select ``IMAGE_COMPRESSION_XZKERN``.
