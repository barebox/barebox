.. _devboot:

Devboot
=======

Devboot is a development convenience that builds on top of the
:ref:`boot override <boot_overrides>` mechanism. It sources a per-user,
per-board script from the network and translates it into ``boot -o``
arguments, making it easy to redirect any combination of kernel, device tree
and initrd to network-hosted files without touching the boot medium.

.. _boot_overrides:

Boot overrides
--------------

Enable ``CONFIG_BOOT_OVERRIDE`` to allow partial overrides of boot entries.
This is already enabled in ``multi_v7_defconfig`` and ``multi_v8_defconfig``.

The ``boot`` command then accepts one or more ``-o`` flags:

.. code-block:: sh

  boot -o bootm.image=/mnt/tftp/my-zImage mmc
  boot -o bootm.oftree=/mnt/tftp/my-board.dtb mmc
  boot -o bootm.initrd=/mnt/tftp/my-initrd mmc

Multiple overrides can be combined in a single invocation:

.. code-block:: sh

  boot -o bootm.image=/mnt/tftp/zImage \
       -o bootm.oftree=/mnt/tftp/board.dtb \
       -o bootm.initrd=/mnt/tftp/initrd \
       mmc

Overrides are applied after the boot entry has been loaded, so the original
entry still selects the boot handler, kernel arguments, etc. Only the
specified components are replaced.

Override rules
^^^^^^^^^^^^^^

``bootm.image``
  The override file is probed for its type. If it is a FIT image, the
  FIT configuration is re-opened and all components (kernel, initrd, device
  tree) are taken from the override FIT. If it is a plain kernel image
  (zImage, Image, uImage, ...), only the OS loadable is replaced.

``bootm.oftree``
  A colon-separated list of files. The first entry is the base device tree;
  subsequent entries are applied as overlays:

  .. code-block:: sh

    # Use a as the device tree
    boot -o bootm.oftree=a mmc

    # Use a as the device tree, apply b as overlay
    boot -o bootm.oftree=a:b mmc

    # Keep the existing device tree, apply b as overlay
    boot -o bootm.oftree=:b mmc

    # Use a as the device tree, apply b and c as overlays
    boot -o bootm.oftree=a:b:c mmc

  Setting ``bootm.oftree`` to an empty string (``bootm.oftree=``) discards
  whatever device tree the boot entry provides. If
  ``CONFIG_BOOTM_OFTREE_FALLBACK`` is enabled (the default), the
  barebox-internal device tree is passed to the kernel instead. If the
  option is disabled, no device tree is passed at all.

``bootm.initrd``
  A colon-separated list of initrd/CPIO files. Linux can transparently
  extract any number of concatenated CPIOs, even if individually compressed.
  A leading colon appends to the initrd that the boot entry already provides:

  .. code-block:: sh

    # Replace with a single initrd
    boot -o bootm.initrd=/mnt/tftp/initrd mmc

    # Concatenate two initrds
    boot -o bootm.initrd=initrd1:initrd2 mmc

    # Append an extra CPIO to the existing initrd
    boot -o bootm.initrd=:extra.cpio mmc

  .. tip::

     The Linux kernel ``make cpio-modules-pkg`` target builds a CPIO archive
     containing all kernel modules. This is useful for supplementing an existing
     initramfs with modules without modifying the root filesystem.
     Combined with initrd concatenation, a devboot script
     can append it to any boot entry's initrd::

       devboot_initrd=":afa-modules-arm64"

     To make use of the modules, the initramfs init will need to make initramfs
     /modules available to the rootfs, e.g. via a bind mount:

       mount -n --bind /lib/modules ${ROOT_MOUNT}/lib/modules

.. note::

   When secure boot is enforced (``bootm_signed_images_are_forced()``),
   overrides are silently ignored.

The devboot script
------------------

The ``devboot`` command (``/env/bin/devboot``) automates the override
workflow for network-based development:

1. Brings up the network (``ifup -a1``).
2. Changes into the fetch directory
   (:ref:`global.net.fetchdir <magicvar_global_net_fetchdir>`, default
   ``/mnt/tftp``).
3. Sources a configuration script named
   ``${global.user}-devboot-${global.hostname}``. If that file does not
   exist, it falls back to ``${global.user}-devboot-${global.arch}``.
4. Translates the variables set by the script into ``boot -o`` arguments.
5. Passes any extra arguments through to the ``boot`` command.

.. note::

   TFTP is not a real file system, e.g., listing of directories is not
   possible. Keep that in mind if the devboot script does more than
   just fetch files by name from TFTP.

Usage
^^^^^

.. code-block:: sh

  # Boot from MMC with devboot overrides
  devboot mmc

  # Boot from the default boot entry
  devboot

Configuration script
^^^^^^^^^^^^^^^^^^^^

The configuration script lives in the fetch directory and sets up to three
variables:

``devboot_image``
  Override for ``bootm.image``. Path is relative to the fetch directory.

``devboot_oftree``
  Override for ``bootm.oftree``. An empty string discards the boot
  entry's device tree (see ``bootm.oftree`` rules above).

``devboot_initrd``
  Override for ``bootm.initrd``. A leading colon appends to the boot
  entry's existing initrd.

Variables that are not set (using ``[[ -v ... ]]``) are left alone,
so unset variables do not generate override arguments.

The script can also set arbitrary barebox variables, for example
kernel command line fragments:

.. code-block:: sh

  # /tftpboot/afa-devboot-rock3a
  devboot_image=afa-fit-rock3a
  devboot_oftree=
  devboot_initrd=":afa-rsinit-arm64"

  global linux.bootargs.dyn.rsinit="rsinit.bind=/lib/modules"

With this script in place and ``global.user=afa``, ``global.hostname=rock3a``,
running ``devboot mmc`` on the board expands to::

  boot -o bootm.image="afa-fit-rock3a" \
       -o bootm.oftree="" \
       -o bootm.initrd=":afa-rsinit-arm64" \
       mmc

This fetches the FIT image ``afa-fit-rock3a`` from the fetch directory,
discards the FIT's device tree (falling back to the barebox-internal one
when ``CONFIG_BOOTM_OFTREE_FALLBACK`` is enabled), and appends the
``afa-rsinit-arm64`` CPIO archive to whatever initrd the FIT already
contains.

Setting up the variables
^^^^^^^^^^^^^^^^^^^^^^^^

The ``global.user`` and ``global.hostname`` variables control which
script is loaded. Make them persistent with:

.. code-block:: sh

  nv user=afa
  nv hostname=rock3a

``global.hostname`` is typically derived from the device tree, but can be
overridden.

Forwarding a remote build directory over the internet
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If the build server is not reachable from the Device-Under-Test, one
possibility is to forward the build artifacts to a local TFTP server
via SSHFS::

  sshfs -o follow_symlinks,allow_other buildserver:/tftpboot /tftpboot

The local development host can then export /tftpboot via TFTP.

If the TFTP server is not running on the local development host, the
``dpipe`` utility can be used to connect a local SFTP server process to
an SSH session that runs ``sshfs`` in slave mode on the remote machine:

.. code-block:: sh

  dpipe /usr/lib/openssh/sftp-server = ssh a3f@tftpserver.in-my.lan sshfs -o slave,follow_symlinks,allow_other -o idmap=user -o gid=1001 :/tftpboot /tftpboot

This works as follows:

- ``dpipe`` connects the standard input/output of both sides of ``=``.
- The left side runs the local SFTP server, giving the remote end access
  to the local filesystem.
- The right side runs ``sshfs`` on the TFTP server host in ``slave``
  mode, reading SFTP protocol directly from stdin instead of spawning its
  own SSH connection.
- ``follow_symlinks`` resolves symlinks on the build host, so the TFTP
  server sees regular files even if the build tree uses symlinks.
- ``allow_other`` permits the TFTP daemon (which typically runs as a
  different user) to read the mounted files.
- ``idmap=user`` maps the remote user's UID to the local user, avoiding
  permission issues.
- ``gid=1001`` sets the group ID for all files (adjust to match the
  group that the TFTP daemon runs as on the server, e.g. ``tftp`` or
  ``nogroup``).
- The colon prefix in ``:/tftpboot`` refers to the root of the *local*
  (build host) filesystem as exported by the SFTP server.

The result is that ``/tftpboot`` on the remote TFTP server mirrors the
``/tftpboot`` directory on the build host. Build output can be
symlinked there and the board will fetch it directly over TFTP.

To undo the mount, terminate the ``dpipe`` process and run
``fusermount -u /tftpboot`` on the TFTP server.

.. note::

   On Debian/Ubuntu, ``dpipe`` can be installed via ``apt install vde2``
   ``sshfs`` and ``fuse`` must be installed on the TFTP server.
   ``/etc/fuse.conf`` on the server must have ``user_allow_other``
   enabled for ``allow_other`` to work.
