NXP Layerscape
==============

barebox has support for some of the ARM64 based Layerscape SoCs from NXP.

Booting barebox
---------------

The Layerscape SoCs contain logic dubbed the Pre-Bootloader (PBL). This unit
reads the boot medium and conducts basic IO multiplexing according to the RCW
(Reset Configuration Word). The RCW then refers the PBL to the location of the
Pre-Bootloader Instructions (PBI). These do basic device configuration and
afterwards poke the barebox PBL into On-Chip SRAM.
The barebox PBL then loads the complete barebox image and runs the PBL again,
this time from SDRAM after it has been set up.

For each board, a barebox image per supported boot medium is generated.
They may differ in the RCW, PBI and endianess depending on the boot medium.

Flashing barebox
----------------

The barebox binary is expected to be located 4K bytes into the SD-Card::

  dd if=images/barebox-${boardname}-sd.image of=/dev/sdX bs=512 seek=8

From there on, ``barebox_update`` can be used to flash
barebox to the QSPI NOR-Flash if required::

  barebox_update -t qspi /mnt/tftp/barebox-${global.hostname}-qspi.imaag

Flashing to the eMMC is possible likewise::

  barebox_update -t sd /mnt/tftp/barebox-${global.hostname}-sd.imaag

.. note:: Some SoCs like the LS1046A feature only a single eSDHC.
  In such a case, using eMMC and SD-Card at the same time is not possible.
  Boot from QSPI to flash the eMMC.

Firmware Blobs
--------------

Network: `fsl_fman_ucode_ls1046_r1.0_106_4_18.bin <https://github.com/NXP/qoriq-fm-ucode/raw/integration/fsl_fman_ucode_ls1046_r1.0_106_4_18.bin>`_.

PSCI Firmware: `ppa-ls1046a.bin <https://github.com/NXP/qoriq-ppa-binary/raw/integration/soc-ls1046/ppa.itb>`_.

Layerscape boards
-----------------

With multi-image and device trees, it's expected to have ``layerscape_defconfig``
as sole defconfig for all Layerscape boards::

  make ARCH=arm layerscape_defconfig

Generated images will be placed under ``images/``.

.. toctree::
  :glob:
  :maxdepth: 1

  layerscape/*
