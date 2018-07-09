element14 WaRP7
===============

This CPU card is based on an NXP i.MX7S SoC.

Supported hardware
------------------

- NXP PMIC PFUZE3000
- Kingston 08EMCP04-EL3AV100 eMCP (eMMC and LPDDR3 memory in one package)

  - 8 GiB eMMC Triple-Level cell NAND flash, eMMC standard 5.0 (HS400)
  - 512 MiB LPDDR3 SDRAM starting at address 0x80000000

Bootstrapping barebox
---------------------

The device boots in internal boot mode from eMMC and is shipped with a
vendor modified u-boot imximage.

Barebox can be used as a drop-in replacement for the shipped bootloader.

The WaRP7 IO Board has a double DIP switch where switch number two defines the
boot source of the i.MX7 SoC::

  +-----+
  |     |
  | | O | <--- on = high level
  | | | |
  | O | | <--- off = low level
  |     |
  | 1 2 |
  +-----+

Bootsource is the internal eMMC::

  +-----+
  |     |
  | O | |
  | | | |
  | | O | <---- eMMC
  |     |
  | 1 2 |
  +-----+

Bootsource is the USB::

  +-----+
  |     |
  | O O | <---- USB
  | | | |
  | | | |
  |     |
  | 1 2 |
  +-----+
