.. index:: fat (filesystem)

FAT filesystem
==============

barebox supports FAT filesystems in both read and write modes with optional
support for long filenames. A FAT filesystem can be mounted using the
:ref:`command_mount` command::

  mkdir /mnt
  mount /dev/disk0.0 fat /mnt
  ls /mnt
  zImage barebox.bin
  umount /mnt

