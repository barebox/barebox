.. _architecture:

####################
barebox architecture
####################

The usual barebox binary consists of two parts. A prebootloader doing
the bare minimum initialization and then the proper barebox binary.

barebox proper
==============

This is the main part of barebox and, like a multi-platform Linux kernel,
is platform-agnostic: The program starts, registers its drivers and tries
to match the drivers with the devices it discovers at runtime.
It initializes file systems and common management facilities and finally
starts an init process. barebox knows no privilege separation and the
init process is built into barebox.
The default init is the :ref:`Hush`, but can be overridden if required.

For such a platform-agnostic program to work, it must receive external
input about what kind of devices are available: For example, is there a
timer? At what address and how often does it tick? For most barebox
architectures this hardware description is provided in the form
of a flattened device tree (FDT). As part of barebox' initialization
procedure, it unflattens (parses) the device tree and starts probing
(matching) the devices described within with the drivers that are being
registered.

The device tree can also describe the RAM available in the system. As
walking the device tree itself consumes RAM, barebox proper needs to
be passed information about an initial memory region for use as stack
and for dynamic allocations. When barebox has probed the memory banks,
the whole memory will become available.

As result of this design, the same barebox proper binary can be reused for
many different boards. Unlike Linux, which can expect a bootloader to pass
it the device tree, barebox *is* the bootloader. For this reason, barebox
proper is prefixed with what is called a prebootloader (PBL). The PBL
handles the low-level details that need to happen before invoking barebox
proper.

Prebootloader (PBL)
===================

The :ref:`prebootloader <pbl>` is a small chunk of code whose objective is
to prepare the environment for barebox proper to execute. This means:

 - Setting up a stack
 - Determining a memory region for initial allocations
 - Provide the device tree
 - Jump to barebox proper

The prebootloader often runs from a constrained medium like a small
(tens of KiB) on-chip SRAM or sometimes even directly from flash.

If the size constraints allow, the PBL will contain the barebox proper
binary in compressed form. After ensuring any external DRAM can be
addressed, it will unpack barebox proper there and call it with the
necessary arguments: an initial memory region and the FDT.

If this is not feasible, the PBL will contain drivers to chain load
barebox proper from the storage medium, usually the same
storage medium the PBL itself was loaded from.

barebox images
==============

In a typical build, the barebox build process generates multiple images
(:ref:`multi_image`).  Normally, all enabled PBLs are each linked with the
same barebox proper binary and then the resulting images are processed to be
in the format expected by the loader.

The loader is often a BootROM, but maybe another first stage bootloader
or a hardware debugger.

Ideally, a single image is all that's needed to boot into barebox.
Depending on BootROM, this may not be possible, because the BootROM hardcodes
assumptions about the image that it loads. This is often the case with
BootROMs that boot from a file system (often FAT): They expect a ``BOOT.BIN``
file, ``MLO`` or similarly named file that does not exceed a fixed file size,
because that file needs to be loaded into the small on-chip SRAM available on
the SoC. In such cases, most barebox functionality will be located in a separate
image, e.g. ``barebox.bin``, which is loaded by ``BOOT.BIN``, once DRAM has
been successfully set up.

There are two ways we generate such small first stage bootloaders in barebox:

Old way: ≥ 2 configs
---------------------

The old way is to have a dedicated barebox first stage config that builds a
very small non-interactive barebox for use as first stage.
Due to size constraints, the first stage config is usually board-specific, while
the second-stage config can target multiple boards at once.

In this setup, each of the first and second stage each consist of their own
prebootloader and barebox proper.

* first stage prebootloader: Does DRAM setup and extracts first stage
  barebox proper into DRAM.

* first stage barebox proper: runs in DRAM and chainloads the second stage binary.

* first stage prebootloader: is already running in DRAM, so it doesn't need to do
  any hardware setup and instead directly extract second stage barebox proper

* second stage barebox proper: your usual barebox experience, which can have an
  interactive shell and boot an operating system

An Example for this is ``am335x_mlo_defconfig``.

New way: single config
----------------------

The new way avoids having a dedicated barebox first stage config by doing both
the low level first stage setup and the chainloading from the same barebox
prebootloader. Despite the prebootloader lacking nearly all driver frameworks found
in barebox proper this is often made possible by reusing the hardware set up
by the BootROM:

BootROMs usually don't deinitialize hardware before jumping to the first stage
bootloader. That means that the barebox prebootloader could just keep reusing
the pre-configured SD-Card and host controller for example and issue read block
commands right away without having to reinitialize the SD-card.

In this setup, only one prebootloader binary will be built. Depending on
the bootflow defined by the BootROM, it may be executed more than once however:

BootROM loads directly into DRAM
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Some BootROMs have built-in support for executing multiple programs and
loading them to different locations. In that case, a full second stage
barebox binary can be loaded as second stage with the first stage RAM
setup handled outside barebox.

An example for Mask ROM loading one image into SRAM and another afterwards
into DRAM are the boards in ``rockchip_v8_defconfig``: A DRAM setup blob
is loaded as first stage followed by barebox directly into DRAM as second
stage.

A slightly different example are what most boards in ``imx_v7_defconfig``
are doing: The i.MX bootrom can execute the bytecode located in the DCD
(Device Configuration Data) table that's part of the bootloader header.
This byte code has simple memory read/write and control flow primitives
that are sufficient to setup a DDR2/DDR3 DRAM, so that barebox can be
loaded into it right away.

The option of being loaded into SRAM first and chainloading from there
is also available, but not used frequently for the 32-bit i.MX platforms.
For the 64-bit platforms with (LP)DDR4, the RAM controller setup is
too complex to express with DCD opcodes, leading to the approach described
below.

BootROM loads into SRAM from offset
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For BootROMs, where the first stage bootloader is loaded from a raw offset
on the boot medium, the barebox image is usually a single binary, which
is processed as follows:

* The BootROM reads the first X bytes containing the prebootloader (and
  some truncated barebox proper content) into the on-chip SRAM and executes
  it.

* The prebootloader will set up DRAM and then chainload the whole of barebox
  proper into it. The offset and size of barebox proper are compiled into
  the PBL, so it knows where to look.

* The prebootloader will then invoke itself again, but this time while running
  from DRAM. The re-executed prebootloader detects that it's running in DRAM
  or at a lower exception level and will then proceed to extract barebox
  proper to the end of the initial memory region and execute it.

And example for this is the ``imx_v8_defconfig``.

.. note:: Some SoCs like the i.MX8M Nano and Plus provide a boot API in ROM
  that can be used by the prebootloader to effortlessly chainload the second stage
  cutting down complexity in the prebootloader greatly. Thanks BootROM authors!

BootROM loads into SRAM from file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the BootROM expects a file, it most often does a size check,
necessitating a second binary for the whole of barebox proper.

The boot flow then looks as follows:

* The BootROM reads the first binary, which includes only the prebootloader
  from the file system into on-chip SRAM and executes it

* The prebootloader will set up DRAM and then load the second binary
  into it. It has no knowledge of its offsets and sizes, but gets that
  information out of the FAT filesystem.

* The second stage binary contains both its own prebootloader and a barebox
  binary. The second stage prebootloader does not need to do any special
  hardware setup, so it will proceed to extract barebox proper to the end of
  the initial memory region and execute it.
