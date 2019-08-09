NXP i.MX6ULL EVK Evaluation Board
=================================

Compute module comes with:

* 512MiB of DDR3L SDRAM
* 256 MB Quad SPI Flash


Boot Configuration
==================

The NXP i.MX6ULL EVK Evaluation Board has two switches responsible for
configuring bootsource/boot mode:

 * SW602 for selecting appropriate BOOT_MODE
 * SW601 for selecting appropriate boot medium

In order to select internal boot set SW602 as follows::

  +-----+
  |     |
  | O | | <--- on = high level
  | | | |
  | | O | <--- off = low level
  |     |
  | 1 2 |
  +-----+

Bootsource is the QSPI::

  +---------+
  |         |
  | | | | | |
  | | | | | |  <---- QSPI
  | O O O O |
  |         |
  | 1 2 3 4 |
  +---------+

Bootsource is the MicroSD::

  +---------+
  |         |
  | | | O | |
  | | | | | |  <---- MicroSD
  | O O | O |
  |         |
  | 1 2 3 4 |
  +---------+


Serial boot SW602 setting needed for ``imx-usb-loader`` is as follows::

  +-----+
  |     |
  | | O | <--- on = high level
  | | | |
  | O | | <--- off = low level
  |     |
  | 1 2 |
  +-----+
