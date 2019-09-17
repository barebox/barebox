TQ-Group TQMLS1046A Module
==========================

Ethernet Ports
--------------

There two RGMII ports are the two closest to the RS-232 socket.
They are ``eth2`` for the lower port and ``eth3`` for the upper port.

MBLS10xxA (Base Board) Boot DIP Switches
----------------------------------------

Boot source selection happens via the ``S5`` DIP-Switch::

  +---------+
  |         |
  | | | O x |
  | | | | x |  <---- SDHC (X31)
  | O O | x |
  |         |
  | 1 2 3 4 |
  +---------+

  +---------+
  |         |
  | O | O x |
  | | | | x |  <---- eMMC
  | | O | x |
  |         |
  | 1 2 3 4 |
  +---------+

  +---------+
  |         |
  | | O O x |
  | | | | x |  <---- QSPI (eSDHC controls SDHC)
  | O | | x |
  |         |
  | 1 2 3 4 |
  +---------+

  +---------+
  |         |
  | O O O x |
  | | | | x |  <---- QSPI (eSDHC controls eMMC)
  | | | | x |
  |         |
  | 1 2 3 4 |
  +---------+
