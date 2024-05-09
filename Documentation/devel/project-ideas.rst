#####################
barebox Project Ideas
#####################

This section collects ideas to improve barebox and should serve as a pool
of ideas for people who want to enter the field of firmware development
but need some guidance what to work on.

These tasks can be adopted as part of programs like Google Summer of Code
or by motivated individuals outside such programs.

If you find a project interesting and would like to work on it, reach out
to the :ref:`mailing list <feedback>` and we can together
try to figure out whether you are a good match for the project.

For GSoC, following barebox developers are mentoring:

  - Ahmad Fatoum (IRC: ``a3f``)
  - Sascha Hauer (IRC: ``_sha_``)
  - Rouven Czerwinski (IRC: ``Emantor``)

This list can be edited and extended by sending patches to the mailing list.
Other interesting ideas: Support for new file systems (EROFS, extfat, btrfs),
improvements for barebox-efi (e.g. as a coreboot payload), ... etc.

Ideas listed below should contain a title, description, expected outcomes,
skills (and HW if any) required and a difficulty rating.
Projects are meant to be about 175 hours of effort, unless otherwise noted.

Address static analyzer feedback for barebox
============================================

Skills: C. Difficulty: Lowest

barebox is automatically tested using Synopsys' free "Coverity Scan" service.
The static analyzer has so far identified 191 possible defects at
https://scan.coverity.com/projects/barebox

There is a number of false positives there, but it already helped us
find actual regressions, e.g. 610720857348 ("fs: ext4: fix bogus behavior
on failure to read ext4 block").

To make this service more useful, the project would involve categorizing
reported issues and handling them as appropriate: Mark them as not applicable
if false positive or provide patches to fix real issues.

Expected outcome is that barebox is coverity-clean.

This project does not require dedicated hardware. QEMU or barebox built
to run under Linux (sandbox) may be used.

Update barebox networking stack for IPv6 support
================================================

Skills: C, Networking. Difficulty: Medium

The barebox network stack is mainly used for TFTP and NFSv3 (over UDP) boot.
Most embedded systems barebox runs on aren't deployed to IPv6 networks yet,
so it's the right time now to future-proof and learn more about networking
internals. One major complication with IPv6 support is neighbor discovery
protocols that require networking to be possible "in the background".
barebox' recent improvements of resource sharing and cooperative scheduling
makes it possible to integrate an IPv6 stack, e.g. lwIP.

There are also community patches to integrate a TCP stack into barebox.
These can be evaluated as time allows.

Expected outcome is that barebox can TFTP/NFS boot over IPv6.

This project does not require dedicated hardware. QEMU or barebox built
to run under Linux (sandbox) may be used.

Improving barebox test coverage
===============================

Skills: C. Difficulty: Lowest

barebox is normally tested end-to-end as part of a deployed system.
More selftests/emulated tests would reduce the round trip time for testing
and thus enable more widely tested patches. A framework has been recently
merged to enable this:

    * Selftest similar to the kernel can be registered and run. These
      directly interface with C-Code.
    * Labgrid tests: These boot barebox in a virtual machine or on real
      hardware and use the shell to test barebox behaviors.

This project will focus on improving the testing coverage by writing more
tests for barebox functionality and by fuzzing the parsers available in
barebox, with special consideration to the FIT parser, which is used in
secure booting setups.

Expected outcome is a richer test suite for barebox and an automated
setup for fuzzing security-critical parsers.

This project does not require dedicated hardware. QEMU or barebox built
to run under Linux (sandbox) may be used.

Porting barebox to new hardware
===============================

Skills: C, low-level affinity. Difficulty: Medium

While Linux and Linux userspace can be quite generic with respect to the
hardware it runs on, the bucket needs to stop somewhere: barebox needs
detailed knowledge of the hardware to initialize it and to pass this
along information to Linux. In this project, familiarity with barebox
and a new unsupported SoC will be established with the goal of porting
barebox to run on it. Prospective developers can suggest suitable
hardware (boards/SoCs) they are interested in. Preference is for
hardware, which is generally available and has more open documentation.

The goal is to have enough support to run barebox on the board, set up
RAM and load a kernel from non-volatile storage and boot it.

If time allows (because most drivers are already available in barebox),
new drivers can be ported to enable not only running Linux on the board,
but bareDOOM as well.

Expected outcome is upstreamed barebox drivers and board support to
enable running on previously unsupported hardware.

This project requires embedded hardware with preferably an ARM SoC, as
these have the widest barebox support, but other architectures are ok
as well.

Improve barebox RISC-V support
==============================

Skills: C, RISC-V interest, low-level affinity. Difficulty: Medium

barebox supports a number of both soft and hardRISC-V targets,
e.g.: BeagleV, HiFive, LiteX and the QEMU/TinyEMU Virt machine.

Unlike e.g. ARM and MIPS, RISC-V support is still in its formative
stage, so much opportunity in implementing the gritty details:

    - Physical memory protection in M-Mode to trap access violations
    - MMU support in S-Mode to trap access violations
    - Improve barebox support for multiple harts (hardware threads)

Expected outcome is better RISC-V support: Access violations are
trapped in both S- and M-Mode. Virtual memory is implemented and
Linux can be booted on multiple harts.

This project does not require dedicated hardware. QEMU can be used.

Improve barebox I/O performance
===============================

Skills: C, low-level affinity. Difficulty: Medium

On a normal modern system, booting may involve mounting and traversing
a file system, which employs caching for directory entries and sits
on top of a block device which caches blocks previously read from the
hardware driver, often by means of DMA. There are a number of improvements
possible to increase throughput of barebox I/O:

    - More efficient erase: Communication protocols like Android Fastboot
      encode large blocks of zeros specially. MMCs with erase-to-zero
      capability could perform such erases in the background instead
      of having to write big chunks of zeros.
    - Block layer block sizes: There is a fixed block size used for
      caching, which is meant to be a good compromise for read
      and write performance. This may not be optimal for all devices
      and can be revisited.

Expected outcome is faster read/write/erasure of MMC block devices.

This project requires embedded hardware with SD/eMMC that is supported
by a barebox media card interface (MCI) driver.

Improve JSBarebox, the barebox web demo
=======================================

Skills: C (Basics), Javascript/Web-assembly, Browser-Profiling. Difficulty: Medium

While Linux and Linux userspace can be quite generic with respect to the
hardware it runs on, the bucket needs to stop somewhere: barebox needs
detailed knowledge of the hardware to initialize it and to pass this
along information to Linux. JSBarebox removes the hurdle of porting
barebox to a new board, for new users who are only interested in
trying it out: The browser runs Tinyemu, a virtual machine in which
barebox executes as if on real hardware and the user can manipulate the
(virtual) hardware from the barebox shell and learn about barebox
conveniences: barebox.org/demo/

The project is about streamlining this demo: CPU usage currently is
quite high and teaching barebox to idle the CPU (as we do on sandbox)
didn't help. This needs to be analyzed with the profiling tools
provided with modern browsers. The remainder of the project can then
focus on improving the tutorial inside the demo. e.g. by adding new
peripherals to the virtual machine.

Expected outcome is snappier and less CPU-intensive barebox demo.
TinyEMU is extended, so the RISC-V machine is more like real
hardware and tutorial is extended to make use of the new peripherals.

This project does not require dedicated hardware. The development
machine need only support a recent browser.
