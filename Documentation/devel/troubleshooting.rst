.. _troubleshooting:

##########################
Boot Troubleshooting Guide
##########################

Especially during development or bring-up, very early failure situations can leave
the system hanging before recovery is even possible.

This guide helps diagnose and debug such issues across barebox' different boot stages.

Boot Flow Overview
==================

A barebox binary consists of two main components:

1. **PBL (Pre-Bootloader)**: This is a smaller barebones loader that does
   what's necessary to download the full barebox binary.
   At the very least, this is decompressing barebox proper and jumping
   to it while passing it a device tree.
   Depending on platform, it may also need to setup DRAM, install a secure
   monitory like TF-A or a secure operating system like OP-TEE and chainload
   barebox from a boot medium.
2. **barebox proper**: The main bootloader logic. This is always loaded
   by a prebootloader passing a device tree and including drivers for
   device initialization, environment setup, and booting the OS.

Refer to the :ref:`barebox architecture <architecture>` for more background
information on the two components and how they map to different boot stages
and images.

If barebox hangs, it's essential to identify *where* at which boot stage,
this failure occurs:

- Does the hang happen in the first stage, i.e., while executing from
  on-chip SRAM?

- Or does the hang happen while in the second stage, i.e., while executing
  from external SDRAM?

And also which component in barebox is affected:

- Is the issue in the prebootloader?

- Or is barebox proper already loaded and started?

Enable Earlier Console Output
=============================

Before delving deeper into debugging, make sure to enable following
options:

- Enable ``CONFIG_DEBUG_LL``

  This enables very early low-level UART debugging.
  It bypasses console frameworks and writes directly to UART registers.
  Many boards in barebox, print a ``>`` character, when ``CONFIG_DEBUG_LL``
  is enabled. If you see such a character after enabling ``DEBUG_LL``, it
  indicates that the barebox prebootloader has been found and control was
  successfully handed over to it. Note that on some SoCs, ``DEBUG_LL``
  requires co-operation from the board entry point, e.g., the pin muxing for
  the serial console needs to be done in software in some situations before
  the UART is accessible from the outside.

  .. note::
     Make sure the correct UART index or address is selected under
     **Kernel low-level debugging por** in ``menuconfig``.
     Configuring the wrong UART might hang your system, because barebox would
     be tricked into accessing hardware that's not there or is powered off.
     The numbering/addresses of ports are described in the System-on-Chip
     datasheet or reference manual and may differ from labels on the hardware.
     Refer to the config symbol help text and ``/chosen/stdout-path`` in the
     device tree if unsure.

- Enable ``CONFIG_DEBUG_INITCALLS`` while ``CONFIG_DEBUG_LL`` is enabled

  This shows output for each initcall level, helping pinpoint where execution stops.
  ``CONFIG_DEBUG_LL`` is useful here, because it allows showing output, even
  before the first serial driver is probed.

- If you still don't see any output besides an early ``>`` or ``a``
  character, enable ``CONFIG_PBL_CONSOLE`` and ``CONFIG_DEBUG_PBL``

  For boards that don't have an early ``putc_ll('>');``, the first output
  being printed is often the debugging output from the ``uncompress.c``
  entry point (``barebox_pbl_start()``). Enable these options to see if
  the CPU gets that far.

  .. warning::
     CONFIG_DEBUG_PBL increases the size of the PBL, which can make it
     exceed a hard limit imposed by a previous stage bootloader.
     Best case, this will be caught by the build system, but might not
     if you are adding a new board and haven't told it yet.

The following sections each start with a list of symptoms, common problems
that cause them and what to try to debug them. Skip the sections that don't
align with your symptoms.

Completely Silent Console
=========================

Even the barebox prebootloader is most often loaded by another
bootloader. This is commonly a mask BootROM hardwired into the
System-on-Chip.

**Symptoms**:

- Despite enabling the config options described in the previous
  section, the console is fully silent.

**Common problems**:

- Wrong bootloader image or format
- Bootloader installed to wrong location
- System hang before serial driver probe
- Enabled, but misconfigured CONFIG_DEBUG_LL

**What to try**:

- Check for BootROM boot indicators:

  Some BootROMs (e.g. AT91) write to a serial port when they start up
  or blink a GPIO (e.g. STM32MP) if they fail to boot the next stage
  bootloader.

- Check that barebox is in the format and at the location that the
  previous stage bootloader expects.

  Compare with a previously working bootloader image, refer to the
  barebox documentation and/or the vendor documentation or ask around.

- Output a character from the entry point

  If you don't have any calls to ``putc_ll`` already, you can stick
  your own ``putc_ll('>');`` there and see if it makes it to the
  serial port. Compare with other boards to see what initialization
  is needed for a serial port (pinmux, clocks, baudrate, ...etc.)

- Toggle a GPIO from the board entry point

  A number of platforms (e.g. i.MX or STM32MP) have header-only GPIO helper
  functions that can be used to toggle a GPIO. These can be used for
  debugging early hangs by toggling an LED for example.

- Trace BootROM activity

  If you have no indication that the barebox prebootloader is being started,
  consider tracing what the BootROM is doing, e.g. via JTAG or a logic analyzer
  for the SD card.

If you managed to get some way to output debug info, move along to the
next step.

Hang after First Stage PBL Console Output
=========================================

The first stage prebootloader handles:
- Basic initialization (e.g., clocks, SDRAM)
- Installation of secure firmware if applicable
- Invocation of the second stage

**Symptoms**:

- You see some output from the prebootloader, but you don't see
  any debug messages starting with ``uncompress.c:``.

**Common problems**:

- Issues in board entry point
- Hang in firmware

**What to try**:

- Check where hang occurs

  If you get just some early output, you'll need to pinpoint, where the issue
  occurs. If enabling ``CONFIG_PBL_CONSOLE`` along with a correctly configured
  ``CONFIG_DEBUG_PBL`` doesn't help, try adding ``putc_ll('@')`` (or any other
  character) to find out, where the startup is stuck. ``putc_ll`` has the
  benefit of being usable everywhere, even before ``setup_c()`` is or
  ``relocate_to_current_adr()`` is called. Once these are called, you may
  also use ``puts_ll()`` or just normal ``printf`` if ``CONFIG_PBL_CONSOLE=y``.

- Check if hang occurs in other loaded firmware

  On platforms like i.MX8/9 and RK35xx, barebox will install ARM trusted
  firmware as secure monitor and possibly OP-TEE as secure OS.
  Hangs can happen if TF-A or OP-TEE is configured to access the wrong
  console (hang/abort on accessing peripheral with gated clock).
  If output ends with the banner of the firmware, jumping back to barebox
  may have failed. In that case, double check that the memory size
  configured for TF-A/OP-TEE is correct and that the entry addresses
  used in barebox and TF-A/OP-TEE are identical.

Hang During Chainloading
========================

Once basic system initialization is done, barebox prebootloader
will load the second stage.

**Symptoms**:

- You see debug messages starting with ``uncompress.c:``, but
  none that start with ``start.c:``

**Common problems**:

- Wrong SDRAM setup
- Corrupted barebox proper read from boot medium

**What to try**:

- Check computed addresses

  If your last output is ``jumping to uncompressed image``, this suggests that
  the hang occurred while trying to execute barebox proper. barebox prints
  the regions it uses for its stack, barebox itself and the initial RAM
  as debug output. Verify these with the actual size of RAM installed and
  check if values are sane.

- Check that barebox was loaded correctly

  You can enable ``CONFIG_COMPILE_TEST`` and ``CONFIG_PBL_VERIFY_PIGGY``
  to have the barebox build system compute a hash of barebox proper,
  which the prebootloader will compare against the hash it computes
  over the compressed data read from the boot medium.

- Check SDRAM setup

  SDRAM setup differs according to the RAM chip being used, the System-on-Chip,
  the PCB traces between them as well as outside factors like temperature.
  When a System-on-Module is used, the hardware vendor will optimally provide
  a validated RAM setup to be used. If RAM layout is custom, the System-on-Chip
  vendor usually provides tools for calculating initial timings and tuning them
  at runtime.

  Because writes can be posted, issues with wrongly set up SDRAM may only become
  apparent on first execution or read and not during mere writing.

  Issues of writes silently misbehaving should be detectable by
  ``CONFIG_PBL_VERIFY_PIGGY``, which reads back the data to hash it.

  If the prebootloader is already running from SDRAM, boot hangs due to completely
  wrong SDRAM setup are less likely, but running a memory test from within barebox
  proper is still recommended.

- Check if an exception happened

  barebox can print symbolized stack traces on exceptions, but support for that
  is only installed in barebox proper. Early exceptions are currently not enabled
  by default, but can be enabled manually with ``CONFIG_ARM_EXCEPTIONS_PBL``.

Preinitcall Stage
=================

The prebootloader ``barebox_pbl_start`` ends up calling ``barebox_non_pbl_start``
in barebox proper. This function does:

- relocation and setting up the C environment
- setting up the ``malloc()`` area and KASAN
- calling ``start_barebox``, which runs the registered initcalls

**Symptoms**:

- You see debug messages starting with ``start.c:``, but none that start
  with ``initcall->``

**Common problems**:

- None, this is quite straight-forward code

**What to try**:

- Check if the code is executed. This can be done with ``putc_ll``. ``printf``
  is not safe to use everywhere in this function, because the C environment
  may not be set up yet.

Initcall Stage
==============

After decompression and jumping to barebox proper, barebox will walk through
the compiled in initcalls.

**Symptoms**:

- You see debug messages starting with ``initcall->``, but system hangs before
  reaching a shell

**Common problems**:

- Hangs during hardware initialization

**What to try**:

- Enable ``CONFIG_DEBUG_PROBES``

  Initcalls don't necessarily correspond to driver probes as a driver may be
  registered before a device or the device probe is postponed until resources
  become available.

  This option prints each driver probe attempt and can help isolate the
  problematic peripheral.

- Check what was the last executed function was

  Each ``initcall->`` log message is followed by a barebox function name.
  Each ``probe->`` log message is followed by the name of the device about
  to be probed.
  This should make it possible to pinpoint where the hang occurred.

- Add extra debugging in the file of the hang

  You can add ``#define DEBUG`` at the start of any barebox file (before the
  C headers!) to print out all debug messages for that file regardless of log
  level.

- Isolate where exactly the hang occurs

  By spreading some ``pr_notice("%s:%d\n", __func__, __LINE__);`` around the
  driver, you should be able to pinpoint what causes the hang.

- Disable drivers selectively to see if a shell can be reached.

  This allows you to see if the hang is a general problem or if it's only
  caused by a single device driver.

Interactive Console
===================

**Symptoms**:

- You see output only with ``CONFIG_DEBUG_LL``, but not otherwise

**Common problems**:

- No consoles are enabled or the user is looking at the wrong console.

**What to try**:

- Enable ``CONFIG_CONSOLE_ACTIVATE_ALL``

  Useful for testing. Instructs barebox proper to print out logs on all
  console devices that it registers.

- Enable ``CONFIG_CONSOLE_ACTIVATE_ALL_FALLBACK`` after figuring out correct console

  This will fall back to activating all consoles, when no console was activated
  by normal means (e.g., via the environment or the device tree
  ``/chosen/stdout`` property).

  This should make it easier to debug similar issues in future should you
  run into them.

Kernel Hang
===========

**Symptoms**:

- Hang after a line like
  ``Loaded kernel to 0x40000000, devicetree at 0x41730000``

With kernel hangs, it's important to find out, whether the hang happens in barebox
still or already while executing the kernel.
Without EFI loader support in barebox, there is no calling back from kernel to barebox,
so a kernel hanging is usually indicative of an issue within the kernel itself.

It's often useful to copy the kernel image into ``/tmp`` instead of booting directly
to verify that the hang is not just a very slow network connection for example.
The ``-v`` option to :ref:`command_cp` is useful for that.
The file size copied may differ from the original if the mean of transport rounds
up to a specific block size. In that case, round up the size on the host system
and run a digest function like :ref:`command_md5sum` to check  that the image
was transferred successfully.

If the image is transferred correctly, the :ref:`command_boot` verbosity is increased
by each extra ``-v`` option. At higher verbosity level, this will also print out
the device tree passed to the kernel. The :ref:`command_of_diff` command is useful
to :ref:`visualize only the fixups that were applied by barebox to the device tree<of_diff>`.

If you are sure that the kernel is indeed being loaded, the ``earlycon`` kernel
feature can enable early debugging output before kernel serial drivers are loaded.
barebox can fixup an earlycon option if ``global.bootm.earlycon=1`` is specified.

Spurious Aborts/Hangs
=====================

**Symptoms**:

- Hangs/panics/aborts that happen in a non-deterministic fashion and whose
  probability is greatly influenced by enabling/disabling barebox options
  and corresponding shifts in the barebox binary

It's generally advisable to run a memory test to verify basic operation and to check
if the RAM size is sane. barebox provides two commands for this: :ref:`command_memtest`
and :ref:`command_memtester`. In addition, some silicon vendors like NXP provide their
own memory test blobs, which barebox can load to SRAM via :ref:`command_memcpy` and
execute using :ref:`command_go`. By having the memory test outside DRAM, a much more
thorough memory test is possible.

With ``CONFIG_MMU=y``, the decompression of barebox proper in the prebootloader
and the runtime of barebox proper will execute with MMU enabled for improved performance.

This increase in performance is due to caches and speculative execution.
barebox will mark memory mapped I/O devices and secure firmware as ineligible for
being accessed speculatively, but it can only do so if the memory size it's told
is correct and if secure memory is marked reserved in the device tree.

The memory map as barebox sees it can be printed with the :ref:`command_iomem`
command. Everything outside ``ram`` region is mapped non executable and uncacheable
by default. Everything inside ``ram`` regions that doesn't have a ``[R]`` next
to it is cacheable by default. The :ref:`command_mmuinfo` command can be used
to show specific information about the MMU attributes for an address.

Memory Corruption Issues
========================

Some hangs might be caused by heap corruption, stack overflows, or use-after-free bugs.

**What to try**:

- Enable ``CONFIG_KASAN`` (Kernel Address Sanitizer)

  This provides runtime memory checking in barebox proper and can detect
  invalid memory accesses.

  .. warning::
     KASAN gratly increases memory usage and may itself cause hangs in
     constrained environments.


Summary of Debug Options
========================

+-----------------------------+-------------------------------------------------------+
| Option                      | Description                                           |
+=============================+=======================================================+
| CONFIG_DEBUG_LL             | Early low-level UART output                           |
+-----------------------------+-------------------------------------------------------+
| CONFIG_PBL_CONSOLE          | Print statements from PBL                             |
+-----------------------------+-------------------------------------------------------+
| CONFIG_DEBUG_PBL            | Enable all debug output in the PBL                    |
+-----------------------------+-------------------------------------------------------+
| CONFIG_PBL_VERIFY_PIGGY     | Verify barebox proper in PBL before decompression     |
+-----------------------------+-------------------------------------------------------+
| CONFIG_ARM_EXCEPTIONS_PBL   | Enable exception handlers in PBL                      |
+-----------------------------+-------------------------------------------------------+
| CONFIG_DEBUG_INITCALLS      | Logs each initcall                                    |
+-----------------------------+-------------------------------------------------------+
| CONFIG_DEBUG_PROBES         | Logs each driver probe                                |
+-----------------------------+-------------------------------------------------------+
| CONFIG_KASAN                | Detects memory corruption                             |
+-----------------------------+-------------------------------------------------------+

Final Tips
==========

- Reach out to :ref:`other barebox users <community>`

  Search the mailing list, send a mail yourself or ask on IRC/Matrix.

- If all else fails, a JTAG debugger to single-step through the code can
  be very useful. To help with this, ``CONFIG_PBL_BREAK`` triggers an
  exception at the start of execution of the individual barebox stages,
  which ``scripts/gdb/helper.py`` can use to correctly set the base
  address, so symbols are correctly located.
