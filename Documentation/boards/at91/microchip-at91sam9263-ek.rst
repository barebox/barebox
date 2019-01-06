Atmel AT91SAM9263-EK Evaluation Kit
===================================

The AT91SAM9263-EK evaluation kit supports Device Tree and Multi Images.

Building barebox:

.. code-block:: sh

  make ARCH=arm at91sam9263ek_defconfig

Notes while working on at91sam9263ek bootstrap support

The at91sam9263 have support for a boot program,
like the other members in the Atmel at91 series.

The boot program (ROMBOOT) will try to load the
boot program from DataFlash, SD Card, NAND Flash and USB

SD Card is the first to try.
It looks for a file named BOOT.BIN in the first
partition in a FAT16/32 filesystem.

To generate the SD Card image I used genimage:
(https://github.com/pengutronix/genimage)
Onle 2 GB SD card works, 4 GB did not work. ROMBOOT do not
support high capacity SD cards.

Configuration file:

.. code-block:: none

  image boot.vfat {
	name = "boot"
	vfat {
		/*
		 * RomBOOT in the at91sam9263 does not recognize
		 * the default FAT partition created by mkdosfs.
		 * -n BOOT - Set volume label to "BOOT"
		 * -F 16   - Force the partition to FAT 16
		 * -D 0    - Set drive number to 0
		 * -R 1 -a - Reserve only one sector
		 * The combination of "-D 0" AND "-R 1 -a"
		 * is required with mkdosfs version 4.1
		 */
		extraargs = "-n BOOT -F 16 -D 0 -R 1 -a"
		file BOOT.BIN    { image = "barebox.bin" } // barebox.bin from root of barebox dir 
		file barebox.bin { image = "barebox-at91sam9263ek.img" }
		file zImage      { image = "zImage" }
	}

	size = 16M
  }

  image rootfs.ext4 {
	ext4 {
		label = "root"
	}
	mountpoint = "/"
	size = 1500M
  }

  image SD {
	hdimage {}

	partition boot {
		partition-type = 0xc
		bootable = "true"
		image = "boot.vfat"
	}

	partition root {
		image = "rootfs.ext4"
		partition-type = 0x83
	}

  }

ROMBOOT will load the BOOT.BIN file to internal SRAM that
starts at 0x300000. Maximum size 0x12000 (72 KiB).
When loaded ROMBOOT will remap like this:

.. code-block:: none

  0x00000000           0x00000000
    Internal ROM   =>     Internal SRAM

  0x00300000          0x00400000
    Internal SRAM  =>     Internal ROM

It is not documented but assumed that ROMBOOT uses the
MMU to remap the addresses.
There seems not to be a dedicated remapping feature that is used.

Note: For DataFlash and NAND Flash the image is validated.
The first 28 bytes must be valid load PC or PC relative addressing.
Vector 0x6 must include the size of the image (in bytes).
This validation is (according to datasheet) not done for SD Card boots.

barebox related notes when trying to make it work with PBL enabled

To let barebox detect the SD card early use: CONFIG_MCI_STARTUP=y

When PBL (and MULTI_IMAGE) are enabled then barebox creates
a binary with the following structure:

.. code-block:: none

  +----------------------+
  | PBL (PreBootLoader)  |
  +----------------------+
  | piggy.o              |
  |+--------------------+|
  ||barebox second stage||
  |+--------------------+|
  +----------------------+

The PBL contains code from the sections .text_head_entry*, .text_bare_init* and .text*

``.text_head_entry*:``
This is the reset vector and exception vectors. Must be the very first in the file

``.text_bare_init*:``
Everything in this section, and , is checked at link time.
Size most be less than BAREBOX_MAX_BARE_INIT_SIZE / ARCH_BAREBOX_MAX_BARE_INIT_SIZE

at91 specify the size of the two sections in exception vector 6 (see above),
if CONFIG_AT91_LOAD_BAREBOX_SRAM is defined.
I think this is because some at91 variants have only very limited SRAM size,
and we copy only a minimal part to the SRAM. The remaining part is then
executed in-place.
For at91sam9263 we have a large SRAM so there is room for the full bootstrap binary.
