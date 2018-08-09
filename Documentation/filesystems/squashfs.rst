.. index:: squashfs (filesystem)

SquashFS filesystem
===================

SquashFS is a highly compressed read-only filesystem for Linux.
It uses zlib, lzo or xz compression to compress both files, inodes
and directories. A SquashFS filesystem can be mounted using the
:ref:`command_mount` command:

.. code-block:: console

  barebox:/ mkdir /mnt
  barebox:/ mount -t squashfs /dev/spiflash.FileSystem /mnt
  barebox:/ ls /mnt
  zImage barebox.bin
  barebox:/ umount /mnt
