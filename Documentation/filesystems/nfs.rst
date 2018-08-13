.. index:: nfs (filesystem)

.. _filesystems_nfs:

NFS Support
===========

barebox has readonly support for NFSv3 in UDP mode.

Example:

.. code-block:: console

   barebox:/ mount -t nfs 192.168.23.4:/home/user/nfsroot /mnt/nfs

The barebox NFS driver adds a ``linux.bootargs`` device parameter to the NFS device.
This parameter holds a Linux kernel commandline snippet containing a suitable root=
option for booting from exactly that NFS share.

Example:

.. code-block:: console

  barebox:/ devinfo nfs0
  ...
  linux.bootargs: root=/dev/nfs nfsroot=192.168.23.4:/home/sha/nfsroot/generic-v7,v3,tcp

The options default to ``v3,tcp`` but can be adjusted before mounting the NFS share with
the ``global.linux.rootnfsopts`` variable
