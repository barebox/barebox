Variscite DT8MCustomBoard with DART-MX8M-PLUS SOM
=================================================

This board is an eval-kit for the Variscite DART-MX8M-PLUS SOM. The latter is a
SOM based on the i.MX8M Plus processor. As seen in official Variscite documents there exist
several hardware revisions for this board. Currently only revision 3.0 could was tested
with Barebox.

The Variscite DART-MX8M-PLUS SOM is available in different configurations. For a rough overview,
these are some of the possible options:

* Processor: NXP i.MX8M Plus, either @1.6GHz or @1.8GHz
* 1 to 8 GiB LPDDR4 RAM
* 8 to 128 GiB eMMC
* (optional) GigE PHY on module
* (optional) Wifi + Bluetooth Chip on module

Besides that the SOM offers a lot of interfaces. Among some of the interfaces that are
made available by the eval-board are:

* USB3
* GigE Ethernet
* PCIe
* SD-card slot
* HDMI

More Information about the eval-board can be found at
https://www.variscite.com/product/single-board-computers/var-dt8mcustomboard/

More Information about the targeted SOM can be found at
https://www.variscite.com/product/system-on-module-som/cortex-a53-krait/dart-mx8m-plus-nxp-i-mx-8m-plus

Providing necessary binary files
--------------------------------

Barebox requires some blobs to successfully bringup the system. These blobs
serve different use cases. Barebox's build system will look for these files
in the configured firmware directory (``firmware`` by default). The build
systems expects these files to have certain names.

Hence the very first thing before building Barebox is to obtain these files and
placing them in the firmware folder.

The DDR4 training files are part of a set of files that is provided by NXP.
They are provided under the terms of a proprietary EULA one has to agree to,
before getting access to the blobs. They are provided as self-extracting
archive. To get a hand on them, perform the following::

   $ wget https://www.nxp.com/lgfiles/NMG/MAD/YOCTO/firmware-imx-8.10.bin
   $ chmod u+x firmware-imx-8.10.bin
   $ ./firmware-imx-8.10.bin

Assuming that the downloaded executable was run from inside the toplevel directory of the Barebox repo,
the necessary DDR4 training files can simply be hardlinked (or copied)::

   $ ln firmware-imx-8.10/firmware/ddr/synopsys/lpddr4_pmu_train_1d_dmem_202006.bin firmware/lpddr4_pmu_train_1d_dmem.bin
   $ ln firmware-imx-8.10/firmware/ddr/synopsys/lpddr4_pmu_train_1d_imem_202006.bin firmware/lpddr4_pmu_train_1d_imem.bin
   $ ln firmware-imx-8.10/firmware/ddr/synopsys/lpddr4_pmu_train_2d_dmem_202006.bin firmware/lpddr4_pmu_train_2d_dmem.bin
   $ ln firmware-imx-8.10/firmware/ddr/synopsys/lpddr4_pmu_train_2d_imem_202006.bin firmware/lpddr4_pmu_train_2d_imem.bin

Another required binary is the Secure Monitor Firmware (BL31). This is build by some ARM Trusted Firmware project (ATF).
One fork is provided by NXP and can be downloaded from https://github.com/nxp-imx/imx-atf. Variscite does maintain it's
own fork of NXP's ATF project. This can be found at https://github.com/varigit/imx-atf/.

Once the ATF has been built successfully, the resulting BL31 binary needs to be placed in the ``firmware`` directory
under the filename ``imx8mp-bl31.bin``.

After those files are in place, one can proceed with the usual Barebox build routine.

Compiling Barebox
-----------------

A quick way to configure and compile Barebox for this board is by starting from
the `imx_v8_defconfig`::

   export ARCH=arm
   export CROSS_COMPILE=/path/to/your/toolchain/bin/aarch64-v8a-linux-gnu-
   make imx_v8_defconfig
   make

With this procedure one might need some additional firmware files in place to
successfully build the Barebox images for all selected boards. A solution to this is
either to copy the necessary files to the `firmware` directory or simply run
`make menuconfig` and deselect the unwanted boards under "System Type".

When the build succeeds, the Barebox image `barebox-variscite-imx8mp-dart-cb.img`
can be found in the `images` subdirectory.

Boot Configuration
------------------

The DT8MCustomBoard allows the user to choose whether to proceed with the *internal*
or *external boot mode*. With this board, *internal boot mode* refers to booting
from the eMMC memory, while *external boot mode* refers to booting from an SD-card.

The mode is selected with switch SW7, located below the buttons on the board.

Set the switch to **ON** for the BootROM to perform an *internal boot*. Otherwise
set the switch to **OFF** to follow the *external boot* procedure.

If in doubt, refer to the silk screen on the board, to select the correct switch
position.

If the BootROM cannot find a valid bootloader image in the selected source,
it'll try several fallbacks until it finally ends in USB download mode or finds
a valid bootloader image to load.

To load an image when the board is in USB download mode the imx-usb-loader tool
is required. To build this tool alongside the Barebox image, select it in the
config menu under "Host Tools".

Starting Barebox
----------------

An easy solution to start Barebox bare metal is to use the *external boot* mode and
copy Barebox onto a SD-card.

To copy the Barebox binary onto a SD-card, use the `dd` tool on linux::

   dd if=images/barebox-variscite-imx8mp-dart-cb.img of=/dev/mmcblk0 bs=512 seek=1 skip=1

Next, you insert the SD-card into the eval board and select *external boot mode* on
switch SW7.

When you power up the board, you should now see Barebox's output appearing on your
serial console.

Currently Supported Features
----------------------------

The Barebox binary configured by the `variscite_imx8mp_dart_cb_defconfig` does currently
not support all possible features of the DT8MCustomBoard. Yet the binary does contain
everything necessary to boot an operating system on the i.MX8MP.

Some of the currently supported features:

* general i.MX8MP bringup, including DRAM initialisation
* working eMMC and SD-card support
* serial console on UART 1 - available through the micro-USB connector on the board
* working gigabit ethernet on the first port (labeled ETH, named `eth0` in Barebox and linux)
* working LED and GPIO support

Some functionality that is currently missing or untested:

* secondary ethernet interface (labeled ETH2) will currently not work
* secure boot (not tested)
* framebuffer support (missing driver)
* OP-TEE integration (not tested - early loading currently not supported by the startup code)
* running on other hardware revisions of the DT8MCustomBoard than v3.0 (not tested)