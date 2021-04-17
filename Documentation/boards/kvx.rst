KVX
===

The Kalray VLIW processor family (KVX) has the following features:
 - 32/64 bits execution mode
 - 6-issue VLIW architecture
 - 64 x 64bits general purpose registers
 - SIMD instructions
 - little-endian
 - deep learning co-processor

Kalray kv3 core which is the third of the KVX family is embedded in Kalray
MPPA3-80 SoC currently used on K200 boards.

This SoC contains 5 clusters which are each made of:
 - 4MiB of on-chip memory
 - 1 dedicated safety/security core (kv3 core).
 - 16 PEs (Processing Elements) (kv3 cores).
 - 16 Co-processors (one per PE)
 - 2 x Crypto accelerators

MPPA3-80 SoC contains the following features:
 - 5 x Clusters
 - 2 x 100G Ethernet controllers
 - 8 x PCIe GEN4 controllers (Root Complex and Endpoint capable)
 - 2 x USB 2.0 controllers
 - 1 x Octal SPI-NOR flash controller
 - 1 x eMMC controller
 - 3 x Quad SPI controllers
 - 6 x UART
 - 5 x I2C controllers (3 x SMBus capable)
 - 4 x CAN controller
 - 1 x OTP memory

The Kalray VLIW architecture barebox port allows to boot it as a second stage
bootloader (SSBL). It is loaded after the FSBL which initialize DDR and needed
peripherals. FSBL always start on the Security Core of Cluster 0

The FSBL can load elf files and pass them a device tree loaded from SPI NOR
flash. As such, barebox should be flashed as an elf file into the SSBL
partition.

KVX boards
----------

.. toctree::
  :glob:
  :maxdepth: 1

  kvx/*

Getting a toolchain
-------------------

Pre-built toolchain are available from github ([#f1]_). In order to build one
from scratch, build scripts are available on github too ([#f2]_).
Once built or downloaded, a ``kvx-elf-`` toolchain will be available and should
be added to your ``PATH``.

Building barebox
----------------

Currently, kvx port is provided with a defconfig named ``generic_defconfig``.
To build it, first generate the config and then run the build:

.. code-block:: sh

  make ARCH=kvx O=$PWD/build defconfig
  make ARCH=kvx O=$PWD/build all

This will generate a ``barebox`` elf file. By default barebox for kvx is
compiled to be run at address 0x110000000.

Booting barebox
---------------

``barebox`` elf file can be loaded using ``kvx-jtag-runner`` to execute the
image via JTAG on an existing board.

Depending on your board and barebox version, you should see the following
message appearing on the serial console.

.. code-block:: console

  barebox 2020.03.0-00126-ga74988bf5 #3 Wed Apr 15 11:31:28 CEST 2020

  Board: KONIC 200 (K200)
  malloc space: 0x110050fe0 -> 0x200000000 (size 3.7 GiB)

  Hit any to stop autoboot:    3
  barebox:/


.. rubric:: References

.. [#f1] `Toolchain releases <https://github.com/kalray/build-scripts/releases>`_
.. [#f2] `Build scripts <https://github.com/kalray/build-scripts/>`_

