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
barebox proper from the storage medium. As this is usually the same
storage medium the PBL itself was loaded from, shortcuts can often
be taken: e.g. a SD-Card could already be in the correct mode, so the
PBL driver can just read the blocks without having to reinitialize
the SD-card.

barebox images
==============

In a typical build, the barebox build process generates multiple images
(:ref:`multi_image`).  All enabled PBLs are each linked with the same
barebox proper binary and then the resulting images are processed to be
in the format expected by the loader.

The loader is often a BootROM, but maybe another first stage bootloader
or a hardware debugger.
