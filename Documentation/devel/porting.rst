##########################
The barebox Porter's Guide
##########################

While barebox puts much emphasis on portability, running on bare-metal
means that there is always machine-specific glue that needs to be provided.
This guide shows places where the glue needs to be applied and how to go
about porting barebox to new hardware.

.. note::
   This guide is written with mainly ARM and RISC-V barebox in mind.
   Other architectures may differ.

************
Introduction
************

Your usual barebox binary consists of two parts. A prebootloader doing
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

Let us now put these new concepts into practice. We will start by adding
a new board for a platform, for which similar boards already exist.
Then we'll look at adding a new SoC, then a new SoC family and finally
a new architecture.

**********************
Porting to a new board
**********************

.. note::
   Parts of this guide are taken from this ELC-E 2020 talk:
   https://www.youtube.com/watch?v=Oj7lKbFtyM0

Chances are there's already a supported board similar to yours, e.g.
an evaluation kit from the vendor. Take a look at ``arch/$ARCH/boards/``
and do likewise for you board. The main steps would be:

Entry point
===========

The PBL's entry point is the first of your code that's run. What happens
there depends on the previously running code. If a previous stage has already
initialized the DRAM, the only thing you need to do is to set up a stack and
call the common PBL code with a memory region and your device tree blob::

  #include <asm/barebox-arm.h>
  #include <console.h>

  ENTRY_FUNCTION_WITHSTACK(start_my_board, MY_STACK_TOP, r0, r1, r2)
  {
  	extern char __dtb_my_board_start[];
  	void *fdt;

  	relocate_to_current_adr();
  	setup_c();

  	pbl_set_putc(my_serial_putc, (void *)BASE_ADDR);

  	barebox_arm_entry(0x80000000, SZ_256M, __dtb_my_board_start);
  }

Lets look at this line by line:

``ENTRY_FUNCTION_WITHSTACK(start_my_board, MY_STACK_TOP, r0, r1, r2)``
   The entry point is special: It needs to be located at the beginning of the
   image, it does not return and may run before a stack is set up.
   To make it possible to write this entry point in C, the macro places
   a machine code prologue that uses ``MY_STACK_TOP`` as the initial stack
   pointer. If the stack is already set up, you may pass 0 here.

   Additionally, the macro passes along a number of registers, in case the
   Boot ROM has placed something interesting there.

``extern char __dtb_my_board_start[];``
   When a device tree is built as part of the PBL, ``__dtb_*_start`` and
   ``__dtb_*_end`` will be defined for it by the build system;
   its name is determined by the name of the device tree source file.
   Declare the start variable, so you can pass along the address of the device
   tree.

``relocate_to_current_adr();``
   Machine code contains a mixture of relative and absolute addressing.
   Because the PBL doesn't know in advance which address it's loaded to,
   the link address of global variables may not be correct. To correct
   them a runtime offset needs to be added, so they point at the
   correct location. This procedure is called relocation and is achieved
   by this function. Note that this is self-modifying code, so it's not
   safe to call this when executing in-place from flash or ROM.

``setup_c();``
   As a size optimization, zero-initialized variables of static storage
   duration are not written to the executable. Instead only the region
   where they should be located is described and at runtime that region
   is zeroed. This is what ``setup_c()`` does.

``pbl_set_putc(my_serial_putc, (void *)BASE_ADDR);``
   Now that we have a C environment set up, lets set our first global
   variable. ``pbl_set_putc`` saves a pointer to a function
   (``my_serial_putc``) that is called by the ``pr_*`` functions to output a
   single character. This can be used for the early PBL console to output
   messages even before any drivers are initialized.
   The second parameter (UART register base address in this instance) is passed
   as a user parameter when the provided function is called.

``barebox_arm_entry(...)``
   This will compute a new stack top from the supplied memory
   region, uncompress barebox proper and pass along its arguments.

Looking at other boards you might see some different patterns:

``*_cpu_lowlevel_init();``
   Often some common initialization and quirk handling
   needs to be done at start. If a board similar to yours does this, you probably
   want to do likewise.

``__naked``
   All functions called before stack is correctly initialized must be
   marked with this attribute. Otherwise, function prologue and epilogue may access
   the uninitialized stack. Note that even with ``__naked``, the compiler may still
   spill excess local C variables used in a naked function to the stack before it
   was initialized. A naked function should thus preferably only contain inline
   assembly, set up a stack and jump directly after to a ``noinline`` non naked
   function where the stack is then normally usable. This pattern is often seen
   together with ``ENTRY_FUNCTION``. Modern boards better avoid this footgun
   by using ``ENTRY_FUNCTION_WITHSTACK``, which will take care to initialize the
   stack beforehand. If either a barebox assembly entry point,
   ``ENTRY_FUNCTION_WITHSTACK`` or earlier firmware has set up the stack, there is
   no reason to use ``__naked``, just use ``ENTRY_FUNCTION_WITHSTACK`` with a zero
   stack top.

``noinline``
   Compiler code inlining is oblivious to stack manipulation in
   inline assembly. If you want to ensure a new function has its own stack frame
   (e.g. after setting up the stack in a ``__naked`` function), you must jump to
   a ``__noreturn noinline`` function. This is already handled by
   ``ENTRY_FUNCTION_WITHSTACK``.

``arm_setup_stack``
   For 32-bit ARM, ``arm_setup_stack`` initializes the stack
   top when called from a naked C function, which allowed to write the entry point
   directly in C. Modern code should use ``ENTRY_FUNCTION_WITHSTACK`` instead.
   Note that in both cases the stack pointer will be decremented before pushing values.
   Avoid interleaving with C-code. See ``__naked`` above for more details.

``__dtb_z_my_board_start[];``
   Because the PBL normally doesn't parse anything out
   of the device tree blob, boards can benefit from keeping the device tree blob
   compressed and only unpack it in barebox proper. Such compressed device trees
   are prefixed with ``__dtb_z_``. It's usually a good idea to use this.

``imx6q_barebox_entry(...);``
   Sometimes it's possible to query the memory
   controller for the size of RAM. If there are SoC-specific helpers to achieve
   this, you should use them.

``get_runtime_offset()/global_variable_offset()``
   This functions return the difference
   between the link and load address. This is zero after relocation, but the
   function can be useful to pass along the correct address of a variable when
   relocation has not yet occurred. If you need to use this for anything more
   then passing along the FDT address, you should reconsider and probably rather
   call ``relocate_to_current_adr();``.

``*_start_image(...)/*_load_image(...)/*_xload_*(...)``
   If the SRAM couldn't fit both PBL and the compressed barebox proper, PBL
   will need to chainload full barebox binary from the boot medium.

Repeating previous advice: The specifics about how different SoCs handle
things can vary widely. You're best served by mimicking a similar recently
added board if one exists. If there's none, continue reading the following
sections.

Board code
==========

If you need board-specific setup that's not covered by any upstream device
tree binding, you can write a driver that matches against your board's
``/compatible``::

  static int my_board_probe(struct device *dev)
  {
  	/* Do some board-specific setup */
  	return 0;
  }

  static const struct of_device_id my_board_of_match[] = {
  	{ .compatible = "my,cool-board" },
  	{ /* sentinel */ },
  };

  static struct driver my_board_driver = {
  	.name = "board-mine",
  	.probe = my_board_probe,
  	.of_compatible = my_board_of_match,
  };
  device_platform_driver(my_board_driver);

Keep what you do here to a minimum. Many thing traditionally done here
should rather happen in the respective drivers (e.g. PHY fixups).

Device-Tree
===========

barebox regularly synchronizes its ``/dts/src`` directory with the
upstream device trees in Linux. If your device tree happens to already
be there you can just include it::

   #include <arm/st/stm32mp157c-odyssey.dts>
   #include "stm32mp151.dtsi"

   / {
   	chosen {
   		environment-emmc {
   			compatible = "barebox,environment";
   			device-path = &sdmmc2, "partname:barebox-environment";
   		};
   	};
   };

   &phy0 {
   	reset-gpios = <&gpiog 0 GPIO_ACTIVE_LOW>;
   };

Here, the upstream device tree is included, then a barebox-specific
SoC device tree ``"stm32mp151.dtsi"`` customizes it. The device tree
adds some barebox-specific info like the environment used for storing
persistent data during development. If the upstream device tree lacks
some info which are necessary for barebox there can be added here
as well. Refer to :ref:`bareboxdt` for more information.

Boilerplate
===========

A number of places need to be informed about the new board:

 - Either ``arch/$ARCH/Kconfig`` or ``arch/$ARCH/mach-$platform/Kconfig``
   needs to define a Kconfig symbol for the new board
 - ``arch/$ARCH/boards/Makefile`` needs to be told which directory the board
   code resides in
 - ``arch/$ARCH/dts/Makefile`` needs to be told the name of the device tree
   to be built
 - ``images/Makefile.$platform`` needs to be told the name of the entry point(s)
   for the board

Example::

  --- /dev/null
  +++ b/arch/arm/boards/seeed-odyssey/Makefile
  +lwl-y += lowlevel.o
  +obj-y += board.o

  --- a/arch/arm/mach-stm32mp/Kconfig
  +++ b/arch/arm/mach-stm32mp/Kconfig
  +config MACH_SEEED_ODYSSEY
  + select ARCH_STM32MP157
  + bool "Seeed Studio Odyssey"

  --- a/arch/arm/boards/Makefile
  +++ b/arch/arm/boards/Makefile
  +obj-$(CONFIG_MACH_SEEED_ODYSSEY) += seeed-odyssey/

  --- a/arch/arm/dts/Makefile
  +++ b/arch/arm/dts/Makefile
  +lwl-$(CONFIG_MACH_SEEED_ODYSSEY) += stm32mp157c-odyssey.dtb.o

  --- a/images/Makefile.stm32mp
  +++ b/images/Makefile.stm32mp
  +pblb-$(CONFIG_MACH_SEEED_ODYSSEY) += start_stm32mp157c_seeed_odyssey
  +FILE_barebox-stm32mp157c-seeed-odyssey.img = start_stm32mp157c_seeed_odyssey.pblb
  +image-$(CONFIG_MACH_SEEED_ODYSSEY) += barebox-stm32mp157c-seeed-odyssey.img

********************
Porting to a new SoC
********************

So, barebox supports the SoC's family, but not this particular SoC.
For example, the new fancy network controller is lacking support.

.. note::
   If your new SoC requires early boot drivers, like e.g. memory
   controller setup. Refer to the next section.

Often drivers can be ported from other projects. Candidates are
the Linux kernel, the bootloader maintained by the vendor or other
projects like Das U-Boot, Zephyr or EDK.

Porting from Linux is often straight-forward, because barebox
imports many facilities from Linux. A key difference is that
barebox does not utilize interrupts, so kernel code employing them
needs to be modified into polling for status change instead.
In this case, porting from U-Boot may be easier if a driver already
exists. Usually, ported drivers will be a mixture of both if they're
not written from scratch.

Drivers should probe from device tree and use the same bindings
like the Linux kernel. If there's no upstream binding, the barebox
binding should be documented and prefixed with ``barebox,``.

Considerations when writing Linux drivers also apply to barebox:

  * Avoid use of ``#ifdef HARDWARE``. Multi-image code should detect at
    runtime what hardware it is, preferably through the device tree

  * Don't use ``__weak`` symbols for ad-hoc plugging in of code. They
    make code harder to reason about and clash with multi-image.

  * Write drivers so they can be instantiated more than once

  * Modularize. Describe inter-driver dependency in the device tree

Miscellaneous Linux porting advice:

  * Branches dependent on ``system_state``: Take the ``SYSTEM_BOOTING`` branch
  * ``usleep`` and co.: use ``[mud]elay``
  * ``jiffies``: use ``get_time_ns()``
  * ``time_before``: use ``!is_timeout()``
  * ``clk_prepare``: is for the non-atomic code preparing for clk enablement. Merge it into ``clk_enable``

***************************
Porting to a new SoC family
***************************

Extending support to a new SoC family can involve a number of things:

New header format
=================

Your loader may require a specific header or format. If the header is meant
to be executable, it should be written in assembly.
If the C compiler for that platform supports ``__attribute__((naked))``, it
can be written in inline assembly inside such a naked function. See for
example ``__barebox_arm_head`` for ARM32 or ``__barebox_riscv_header`` for RISC-V.

For platforms, without naked function support, inline assembly may not be used
and the entry point should be written in a dedicated assembly file.
This is the case with ARM64, see for example ``__barebox_arm64_head`` and the
``ENTRY_PROC`` macro.

Another way, which is often used for non-executable headers with extra
meta-information like a checksum, is adding a new tool to ``scripts/``
and have it run as part the image build process. ``images/`` contains
various examples.

Memory controller setup
=======================

If you've an external DRAM controller, you will need to configure it.
This may involve enabling clocks and PLLs. This should all happen
in the PBL entry point.

Chainloading
============

If the whole barebox image couldn't be loaded initially due to size
constraints, the prebootloader must arrange for chainloading the full
barebox image.

One good way to go about it is to check whether the program counter
is in DRAM or SRAM. If in DRAM, we can assume that the image was loaded
in full and we can just go into the common PBL entry and extract barebox
proper. If in SRAM, we'll need to load the remainder from the boot medium.

This loading requires the PBL to have a driver for the boot medium as
well as its prerequisites like clocks, resets or pin multiplexers.

Examples for this are the i.MX xload functions. Some BootROMs boot from
a FAT file system. There is vfat support in the PBL. Refer to the sama5d2
board support for an example.

Core drivers
============

barebox contains some stop-gap alternatives that can be used before
dedicated drivers are available:

  * Clocksource: barebox often needs to delay for a specific time.
    ``CLOCKSOURCE_DUMMY_RATE`` can be used as a stop-gap solution
    during initial bring up.

  * Console driver: serial output is very useful for debugging. Stop-gap
    solution can be ``DEBUG_LL`` console

*****************************
Porting to a new architecture
*****************************

Makefile
========

``arch/$ARCH/Makefile`` defines how barebox is built for the
architecture. Among other things, it configures which compiler
and linker flags to use and which directories Kbuild should
descend into.

Kconfig
=======

``arch/$ARCH/Kconfig`` defines the architecture's main Kconfig symbol,
the supported subarchitectures as well as other architecture specific
options. New architectures should select ``OFTREE`` and ``OFDEVICE``
as well as ``HAVE_PBL_IMAGE`` and ``HAVE_PBL_MULTI_IMAGES``.

Header files
============

Your architecture needs to implement following headers:

 - ``<asm/bitops.h>``
   Defines optimized bit operations if available
 - ``<asm/bitsperlong.h>``
   ``sizeof(long)`` Should be the size of your pointer
 - ``<asm/byteorder.h>``
   If the compiler defines a macro to indicate endianness,
   use it here.
 - ``<asm/elf.h>``
   If using ELF relocation entries
 - ``<asm/dma.h>``
   Only if ``HAS_DMA`` is selected by the architecture.
 - ``<asm/io.h>``
   Defines I/O memory and port accessors
 - ``<asm/barrier.h>``
   Can normally just include ``<asm-generic/barrier.h>``
 - ``<asm/mmu.h>``
 - ``<asm/string.h>``
 - ``<asm/swab.h>``
 - ``<asm/types.h>``
 - ``<asm/unaligned.h>``
   Defines accessors for unaligned access
 - ``<asm/setjmp.h>``
   Must define ``setjmp``, ``longjmp`` and ``initjmp``.
   ``setjmp`` and ``longjmp`` can be taken out of libc. As barebox
   does no floating point operations, saving/restoring these
   registers can be dropped. ``initjmp`` is like ``setjmp``, but
   only needs to store 2 values in the ``jmpbuf``:
   new stack top and address ``longjmp`` should branch to

Most of these headers can be implemented by referring to the
respective ``<asm-generic/*.h>`` versions.

Relocation
==========

Because there might be no single memory region that works for all
images in a multi-image build, barebox needs to be relocatable.
This can be done by implementing three functions:

 - ``get_runtime_offset()``: This function should return the
   difference between the link and load address. One easy way
   to implement this is to force the link address to ``0`` and to
   determine the load address of the barebox ``_text`` section.

 - ``relocate_to_current_adr()``: This function walks through
   the relocation entries and fixes them up by the runtime
   offset. After this is done ``get_runtime_offset()`` should
   return `0` as ``_text`` should also be fixed up by it.

 - ``relocate_to_adr()``: This function copies the running barebox
   to a new location in RAM, then does ``relocate_to_current_adr()``
   and resumes execution at the new location. This can be omitted
   if barebox won't initially execute out of ROM.

 - ``relocate_to_adr_full()``: This function does what
   ``relocate_to_adr()`` does and in addition moves the piggy data
   (the usually compressed barebox appended to the prebootloader).

Of course, for these functions to work. The linker script needs
to ensure that the ELF relocation records are included in the
final image and define start and end markers so code can iterate
over them.

To ease debugging, even when relocation has no yet happened,
barebox supports ``DEBUG_LL``, which acts similarly to the
PBL console, but does not require relocation. This is incompatible
with multi-image, so this should only be considered while debugging.

Linker scripts
==============

You'll need two linker scripts, one for barebox proper and the
other for the PBL. Refer to the ARM and/or RISC-V linker scripts
for an example.

Generic DT image
================

It's a good idea to have the architecture generate an image that
looks like and can be booted just like a Linux kernel. This allows
easy testing with QEMU or booting from barebox or other bootloaders.
Refer to ``BOARD_GENERIC_DT`` for examples. If not possible, the
(sub-)architecture making use of the image should
``register_image_handler`` that can chain-boot the format from
a running barebox. This allows for quick debugging iterations.
