..
  SPDX-License-Identifier: GPL-2.0-only

=======
barebox
=======

A bootloader that follows the tradition of Das U-Boot, while
adopting modern design ideas from the Linux kernel.

.. class:: center

`Try it out in your browser ✨ <https://www.barebox.org/jsbarebox/?graphic=0>`_
·
`Check out the documentation 📖 <https://www.barebox.org/doc/latest/index.html>`_
·
`Get involved 🫶 <https://www.barebox.org/doc/latest/user/introduction.html#feedback>`_

Features
--------

* A POSIX-based file API:
  Inside barebox the usual ``open/close/read/write/lseek`` functions are used.
  This makes it familiar to everyone who has programmed under UNIX systems.

* Usual shell commands like ``ls/cd/mkdir/echo/cat/cp/mount``,...
  which work uniformly across file systems

* Filesystem support:
  The loader starts up with mounting a ramdisk on ``/``. Then a devfs is mounted
  on ``/dev`` allowing the user (or shell commands) to access devices. Apart from
  these two filesystems there are a number of different filesystems ported:
  ext4, efi, efivarfs, ext4, fat, jffs2, NFS, tftp, pstore, squashfs, ubifs,
  ratp (RFC916), u-boot variable FS among others.

* Multiplatform support:
  Configurations like ``multi_v7_defconfig`` and ``multi_v8_defconfig`` build
  barebox for many dozens of boards all at once. All resulting images share the
  same barebox proper binary, each time with a different prebootloader that does
  low-level initialization and passes in the device tree. Prebootloaders may
  detect board/SoC type and pass in different device trees to support various
  boards with the same image.

* Focus on developer experience:
  barebox provides many features that should be familiar to kernel developers

  * barebox images are designed to be bootable from within barebox itself,
    so developers can trivially network boot. For chainloading from existing
    bootloaders, there's also an extra ``barebox-dt-2nd.img`` that is bootable
    exactly like a Linux kernel.
  * KASAN (KernelAddressSanitizer) and kallsyms (symbolized stack traces)
    help in hunting down memory issues
  * ramoops allows barebox to share its own ``dmesg`` log with Linux
  * Linux kernel drivers are easier to port, because the driver frameworks are
    closely based on the upstream Linux counterpart

* Device parameter support:
  Each device can have an unlimited number of parameters. They can be accessed
  on the command line with ``<devid>.<param>="..."``, for example
  ``eth0.ip=192.168.0.7`` or echo ``$eth0.ip``.

* Editor:
  Scripts can be edited with a small editor. This editor has no features
  except the ones really needed: moving the cursor and typing characters
  on multiple lines.

* The environment is not a variable store anymore, but a file store.
  The ``saveenv`` command archives the files under a certain directory (by default
  ``/env``) in persistent storage (by default ``/dev/env0``). There is a counterpart
  called ``loadenv``, too. Additionally, barebox-state provides power-fail-safe
  variable-only storage with optional authentication, so the mutable environment
  can be disabled for security reasons.

* Uniformity: Finding out where barebox booted from, determining what caused
  the last reset, booting in a redundant fail-safe manner, updating barebox on disk
  are examples of functionality handled generically by frameworks, so boards can look
  and feel the same and custom scripts or board code are only needed for the
  exceptional stuff.

* device/driver model:
  Devices are no longer described by defines in the config file. Instead
  devices are registered as they are discovered (e.g. through OpenFirmware
  device tree traversal or EFI handles) or by board code.
  Drivers will match upon the devices automatically.

* Device Tree Manipulation:
  barebox has extensive support for fixing up both its own device tree at
  runtime and the kernel's, whether from board code, shell scripts or within
  device tree overlays

* Initcalls:
  hooks in the startup process can be achieved with ``*_initcall()`` directives
  in each file.

* ``ARCH=sandbox`` simulation target:
  barebox can be compiled to run under Linux. While this is rather useless
  in real world this is a great debugging and development aid. New features
  can be easily developed and tested on long train journeys and started
  under gdb. There is a console driver for Linux which emulates a serial
  device and a TAP-based Ethernet driver. Linux files can be mapped to
  devices under barebox to emulate storage devices.

* and `yes, of course it runs DOOM <https://doomwiki.org/wiki/BareDOOM>`_.

Building barebox
----------------

barebox uses the Linux kernel's build system. It consists of two parts:
the Makefile infrastructure (kbuild), plus a configuration system
(kconfig). So building barebox is very similar to building the Linux
kernel.

For the examples below, we use the User Mode barebox implementation, which
is a port of barebox to the Linux userspace. This makes it possible to
test drive the code without having real hardware. So for this test
scenario, ``ARCH=sandbox`` is the valid architecture selection. This currently
works on at least IA32 hosts and x86-64 hosts.

Selection of the architecture and the cross compiler can be done by using
the environment variables ``ARCH`` and ``CROSS_COMPILE``.

In order to configure the various aspects of barebox, start the barebox
configuration system::

  # make menuconfig

This command starts a menu box and lets you select all the different
options available for your architecture. Once the configuration was
finished (you can simulate this by using the standard demo config file
with ``make sandbox_defconfig``), there is a ``.config`` file in the
toplevel directory of the source code.

Once barebox is configured, we can start the compilation::

  # make

If everything goes well, the result is a file called barebox::

  # ls -l barebox
    -rwxr-xr-x 1 rsc ptx 114073 Jun 26 22:34 barebox

To get some files to play with, you can generate a squashfs image::

  # mksquashfs somedir/ squashfs.bin

The barebox image is a normal Linux executable, so it can be started
just like every other program::

  # ./barebox -i squashfs.bin

  add fd0 backed by file squashfs.bin
  add stickypage backed by file /run/user/1000/barebox/stickypage.1661112

  barebox 2024.07.0 #0 Wed Jul 18 11:36:31 CEST 2024
  [...]

  barebox@Sandbox:/

Specifying ``-[ie] <file>`` tells barebox to map the file as a device
under /dev. Files given with ``-e`` will appear as ``/dev/env[n]``. Files
given with ``-i`` will appear as ``/dev/fd[n]``.

If barebox finds a valid configuration sector on ``/dev/env0`` it will
load it to ``/env``. It then source ``/env/init/*`` if it exists. If you have
loaded the example environment, barebox will show you a menu asking for
your settings. The sandbox configuration embeds a "sticky page", which is
inherited across barebox resets. This way, there's a usable environment
out-of-the-box.

If you have started barebox as root, you will find a new tap device on your
host which you can configure using ifconfig. Once you configured barebox'
network settings accordingly you can do a ping or tftpboot.

If you have mapped a squashfs image, try mounting it with::

  barebox@Sandbox:/ mount fd0
  mounted /dev/fd0 on /mnt/fd0

When called with a single argument, the barebox ``mount`` command will inspect
the source device file's magic bytes and mount it with the appropriate file system
under ``/mnt``.

Memory can be examined as usual using md/mw commands. They understand
the ``-s <file>`` and ``-d <file>`` options respectively to tell the commands
that they should work on the specified files instead of ``/dev/mem`` which holds
the complete address space.

Directory Layout
----------------

Most of the directory layout is based upon the Linux Kernel

.. list-table::

  * - | ``arch/*``
      | ``arch/*/include``
      | ``arch/*/mach-*``
      | ``include/mach/*``
    - | contains architecture specific parts
      | architecture specific includes
      | SoC specific code
      | SoC specific includes

  * - | ``drivers/serial``
      | ``drivers/net/dsa/``
      | ``drivers/...``
    - | drivers

  * - | ``fs/``
    - | filesystem support and filesystem drivers

  * - | ``lib/``
    - | generic library functions (getopt, readline and the like)

  * - | ``common/``
      | ``common/boards``
    - | common stuff
      | board code shared between multiple boards (possibly of different architectures)

  * - | ``commands/``
    - | Commands that can be executed by the barebox shell

  * - | ``net/``
    - | Networking stuff

  * - | ``scripts/``
    - | Kconfig system and tools used during barebox build or for deploying it

  * - | ``Documentation/``
    - | Sphinx generated documentation. Call "make docs" to generate a HTML version in Documentation/html.

  * - | ``defaultenv/``
    - | default environment assembled into images. Board environment is overlaid on top

  * - | ``dts/``
    - | An import of the upstream device tree repository maintained as part of the Linux kernel

Release Strategy
----------------

barebox is developed with git. On a monthly schedule, tarball releases are
branched from the repository and released on the project web site. Here
are the release rules:

- Releases follow a time based scheme::

    barebox-xxxx.yy.z.tar.bz2
            ^^^^ ^^ ^----------- Bugfix Number, starting at 0
              \   \------------- Month
               \---------------- Year

  Example: barebox-2024.08.0.tar.bz2

- As we are aiming for monthly releases, development is considered to be
  a continuous process. If you find bugs in one release, you have the chance
  to get patches in on a very short time scale (usually a month at most).

- New features are applied to the ``next`` branch. Fixes directly to the
  ``master`` branch. Releases are always branched from ``master`` and then
  ``next`` is merged into ``master``. Thus new features take 1-2 months
  until they hit a release.

- Usually, there are no bugfix releases, so z=0. If there is a need
  to make a bugfix release, z is the right place to increment.

- If there may be a reason for pre releases, they are called ::

    barebox-xxxx.yy.z-pren.tar.bz
                         ^------ Number of prerelease, starting with 1

  Example: barebox-2009.12.0-pre1.tar.bz2

  We think that there is no need for pre releases, but if it's ever
  necessary, this is the scheme we follow.

- Only the monthly releases are archived on the web site. The tarballs
  are located in https://www.barebox.org/download/ and this location
  does never change, in order to make life easier for distribution
  people.

.. _contributing:

Contributing
------------

barebox collaboration happens chiefly on the
<barebox@lists.infradead.org> mailing list.

The repository can be forked on `Github <https://github.com/barebox/barebox>`
to run the CI test suite against the virtualized targets before submitting
patches.

Refer to the
`introduction section <https://www.barebox.org/doc/latest/user/introduction.html#feedback>`_
in the documentation for more information.

License
-------

| Copyright (C) 2000 - 2005 Wolfgang Denk, DENX Software Engineering, wd@denx.de.
| Copyright (C) 2018 Sascha Hauer, Pengutronix, and individual contributors
|


barebox is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License, version 2, as published by the Free
Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License in the file
COPYING along with this program. If not, see <https://www.gnu.org/licenses/>.

Individual files may contain the following SPDX license tags as a shorthand for
the above copyright and warranty notices::

    SPDX-License-Identifier: GPL-2.0-only
    SPDX-License-Identifier: GPL-2.0-or-later

This eases machine processing of licensing information based on the SPDX
License Identifiers that are available at http://spdx.org/licenses/.

Also note that some files in the barebox source tree are available under
several different GPLv2-compatible open-source licenses. This fact is noted
clearly in the file headers of the respective files and the full license
text is reproduced in the ``LICENSES/`` directory.
