.. _booting_linux:

Booting Linux
=============

Introduction
------------

The lowlevel boot command in barebox is :ref:`command_bootm`. This command
can be used directly, but there is also a :ref:`command_boot` command
which offers additional features like a boot sequence which tries to
boot different entries until one succeeds.

The bootm command
-----------------

The :ref:`command_bootm` command is the lowlevel boot command. Depending on the
architecture the bootm command handles different image types. On ARM the
following images are supported:

* ARM Linux zImage
* U-Boot uImage
* barebox images

The images can either be passed directly to the bootm command as argument or
in the ``global.bootm.image`` variable:

.. code-block:: sh

  bootm /path/to/zImage

  # same as:

  global.bootm.image=/path/to/zImage
  bootm

When barebox has an internal devicetree it is passed to the kernel. You can
specify an alternative devicetree with the ``-o DTS`` option or the ``global.bootm.oftree``
variable:

.. code-block:: sh

  bootm -o /path/to/dtb /path/to/zImage

  # same as:

  global.bootm.oftree=/path/to/dtb
  global.bootm.image=/path/to/zImage
  bootm

**NOTE:** it may happen that barebox is probed from the devicetree, but you have
want to start a Kernel without passing a devicetree. In this case set the
``global.bootm.boot_atag`` variable to ``true``.

Passing Kernel Arguments
^^^^^^^^^^^^^^^^^^^^^^^^

The simple method to pass bootargs to the kernel is with
``CONFIG_FLEXIBLE_BOOTARGS`` disabled: in this case the bootm command
takes the bootargs from the ``bootargs`` environment variable.

With ``CONFIG_FLEXIBLE_BOOTARGS`` enabled, the bootargs are composed
from different :ref:`global device<global_device>` variables. All variables beginning
with ``global.linux.bootargs.`` will be concatenated to the bootargs:

.. code-block:: sh

  global linux.bootargs.base="console=ttyO0,115200"
  global linux.bootargs.debug="earlyprintk ignore_loglevel"

  bootm zImage

  ...

  Kernel command line: console=ttymxc0,115200n8 earlyprintk ignore_loglevel

Additionally all variables starting with ``global.linux.mtdparts.`` are concatenated
to a ``mtdparts=`` parameter to the kernel. This makes it possible to consistently
partition devices with the :ref:`command_addpart` command and pass the same string as used
with addpart to the Kernel:

.. code-block:: sh

  norparts="512k(bootloader),512k(env),4M(kernel),-(root)"
  nandparts="1M(bootloader),1M(env),4M(kernel),-(root)"

  global linux.mtdparts.nor0="physmap-flash.0:$norparts"
  global linux.mtdparts.nand0="mxc_nand:$nandparts"

  addpart /dev/nor0 $norparts
  addpart /dev/nand0 $nandparts

  ...

  bootm zImage

  ...

  Kernel command line: mtdparts=physmap-flash.0:512k(bootloader),512k(env),4M(kernel),-(root);
			mxc_nand:1M(bootloader),1M(env),4M(kernel),-(root)

Creating root= options for the Kernel
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It's a common case that the Linux Kernel is loaded from a filesystem
that later becomes the root filesystem for the Kernel. For several
filesystems barebox can automatically append a suitable root= option
to the Kernel command line. This is done when ``global.bootm.appendroot``
is true. How the root= option is appended depends on the device type
and filesystem the kernel is booted from. For disk like devices (SD/MMC,
ATA) the partition UUID will be used, the root= option will be something
like ``root=PARTUUID=deadbeef-1``. For UBIFS fileystems it will be
``root=ubi0:volname ubi.mtd=mtdpartname rootfstype=ubifs``. NFS
filesystems will result in ``root=/dev/nfs nfsroot=ip:/path/to/nfsroot,v3,tcp``.
The ``v3,tcp`` part is configurable in ``global.linux.rootnfsopts``.

The boot command
----------------

The :ref:`command_boot` command offers additional convenience for the :ref:`command_bootm`
command. It works with :ref:`boot_entries` and :ref:`bootloader_spec` entries. Boot entries
are located under /env/boot/ and are scripts which setup the bootm variables so that the
``boot`` command can run ``bootm`` without further arguments.

.. _boot_entries:

Boot entries
^^^^^^^^^^^^

A simple boot entry in ``/env/boot/mmc`` could look like this:

.. code-block:: sh

  #!/bin/sh

  global.bootm.image=/mnt/mmc1/zImage
  global.bootm.oftree=/env/oftree

  global linux.bootargs.dyn.root="root=PARTUUID=deadbeef:01"

This takes the kernel from ``/mnt/mmc1/zImage`` (which could be an
:ref:`automount` path registered earlier). The devicetree will be used from
``/env/oftree``. The Kernel gets the command line
``root=PARTUUID=deadbeef:01``. Note the ``.dyn`` in the bootargs variable name.
boot entries should always add Kernel command line parameters to variables with
``.dyn`` in it. These will be cleared before booting different boot entries.
This is done so that following boot entries do not leak command line
parameters from the previous boot entries.

This entry can be booted with ``boot mmc``. It can also be made the default by
setting the ``global.boot.default`` variable to ``mmc`` and then calling
``boot`` without arguments.

.. _bootloader_spec:

Bootloader Spec
^^^^^^^^^^^^^^^

barebox supports booting according to the bootloader spec:

http://www.freedesktop.org/wiki/Specifications/BootLoaderSpec/

It follows another philosophy than the :ref:`boot_entries`. With Boot Entries
booting is completely configured in the bootloader. Bootloader Spec Entries
on the other hand the boot entries are on a boot medium. This gives a boot medium
the possibility to describe where a Kernel is and what parameters it needs.

All Bootloader Spec Entries are in a partition on the boot medium under ``/loader/entries/*.conf``.
In the Bootloader Spec a boot medium has a dedicated partition to use for
boot entries. barebox is less strict, it accepts Bootloader Spec Entries on
every partition barebox can read.

A Bootloader Spec Entry consists of key value pairs::

  /loader/entries/6a9857a393724b7a981ebb5b8495b9ea-3.8.0-2.fc19.x86_64.conf:

  title      Fedora 19 (Rawhide)
  version    3.8.0-2.fc19.x86_64
  machine-id 6a9857a393724b7a981ebb5b8495b9ea
  options    root=UUID=6d3376e4-fc93-4509-95ec-a21d68011da2
  linux      /6a9857a393724b7a981ebb5b8495b9ea/3.8.0-2.fc19.x86_64/linux
  initrd     /6a9857a393724b7a981ebb5b8495b9ea/3.8.0-2.fc19.x86_64/initrd

All paths are absolute paths in the partition. Bootloader Spec Entries can
be created manually, but there also is the ``scripts/kernel-install`` tool to
create/list/modify entries directly on a MMC/SD card or other media. To use
it create a SD card / USB memory stick with a /boot partition with type 0xea.
The partition can be formatted with FAT or EXT4 filesystem. If you wish to write
to it from barebox later you must use FAT. The following creates a Bootloader
Spec Entry on a SD card:

.. code-block:: sh

  scripts/kernel-install --device=/dev/mmcblk0 -a \
                --machine-id=11ab7c89d02c4f66a4e2474ea25b2b84 --kernel-version="3.15" \
                --kernel=/home/sha/linux/arch/arm/boot/zImage --add-root-option \
                --root=/dev/mmcblk0p1 -o "console=ttymxc0,115200"

The entry can be listed with the -l option:

.. code-block:: sh

  scripts/kernel-install --device=/dev/mmcblk0 -l

  Entry 0:
        title:      Linux-3.15
        version:    3.15
        machine_id: 11ab7c89d02c4f66a4e2474ea25b2b84
        options:    console=ttymxc0,115200 root=PARTUUID=0007CB20-01
        linux:      11ab7c89d02c4f66a4e2474ea25b2b84.15/linux

When on barebox the SD card shows up as ``mmc1`` then this entry can be booted with
``boot mmc1`` or with setting ``global.boot.default`` to ``mmc1``.

``machine-id`` is an optional key. If ``global.boot.machine_id`` variable is set to
non-empty value, then barebox accepts only Bootloader Spec entries with ``machine-id``
key. In case if value of global variable and Bootloader Spec key match each other,
barebox will choose the boot entry for booting. All other Bootloader Spec entries will
be ignored.

A bootloader spec entry can also reside on an NFS server in which case a RFC2224
compatible NFS URI string must be passed to the boot command:

.. code-block:: sh

  boot nfs://nfshost[:port]//path/

Additionally to the options defined in the original spec barebox understands the
``linux-appendroot`` option. This is a boolean value and if set to ``true`` barebox
will automatically append a ``root=`` string to the Linux commandline based on the
device where the entry is found on. This makes it possible to use the same rootfs
image on different devices without having to specify a different root= option each
time.

Additionally to the options defined in the original spec, Barebox has the
``devicetree-overlay`` option. This is a string value that refer to overlays
that will be applied to the device tree before passing it to Linux.

Network boot
------------

With the following steps, barebox can start the kernel and root filesystem
over the network, a standard development case.

See :ref:`networking` for informations how to configure your network interfaces.

Note that barebox will pass the same IP settings to the kernel, i.e. it passes
``ip=$ipaddr:$serverip:$gateway:$netmask::<linuxdevname>:`` for a static IP setup
and ``ip=dhcp`` for a dynamic DHCP setup. ``<linuxdevname>`` is a configurable value.
set ``nv.dev.<ethdev>.linuxdevname`` to the name the device has in Linux.

By default, barebox uses the variables ``global.user`` and ``global.hostname``
to retrieve its kernel image over TFTP, which makes it possible to use multiple
boards for multiple users with one single server.
You can adjust those variables using nvvars with these commands::

  nv user=sha
  nv hostname=efikasb

Copy the kernel (and devicetree if needed) to the root directory of your TFTP
server, and name them accordingly; for example::

  cp zImage /tftpboot/sha-linux-efikasb
  cp myboard.dtb /tftpboot/sha-oftree-efikasb

(In this example, the directory ``/tftpboot`` represents the root directory of
the TFTP server.
That directory depends on the configuration of your TFTP server, some servers
may also use ``/srv/tftp`` instead.)

barebox will pass ``nfsroot=/home/${global.user}/nfsroot/${global.hostname}`` to
the kernel.
This causes the kernel to mount its root filesystem from a NFS server, which is
detected through the DHCP reply.
To choose a different server, simply prepend its IP address to the mount path,
e.g. ``nfsroot=192.168.23.5:/home/...``.
In any case, make sure that the specified mountpoint is exported by your NFS
server.

For more information about booting with ``nfsroot``, see
`Documentation/filesystems/nfs/nfsroot.txt <https://github.com/torvalds/linux/blob/master/Documentation/filesystems/nfs/nfsroot.txt>`__
in the Linux kernel documentation.

If the preconfigured paths or names are not suitable, they can be adjusted in
``/env/boot/net``:

.. code-block:: sh

  #!/bin/sh

  path="/mnt/tftp"

  global.bootm.image="${path}/${global.user}-linux-${global.hostname}"

  oftree="${path}/${global.user}-oftree-${global.hostname}"
  if [ -f "${oftree}" ]; then
         global.bootm.oftree="$oftree"
  fi

  nfsroot="/home/${global.user}/nfsroot/${global.hostname}"
  ip_route_get -b ${global.net.server} global.linux.bootargs.dyn.ip
  
  global.linux.bootargs.dyn.root="root=/dev/nfs nfsroot=$nfsroot,v3,tcp"

``boot net`` will then retrieve the kernel (and also the device tree and
initramfs, if used) over TFTP and boot it.
