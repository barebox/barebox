.. index:: nfs (filesystem)

.. _filesystems_nfs:

NFS Support
===========

barebox has readonly support for NFSv3 in UDP mode.

Example:

.. code-block:: console

   barebox:/ mount -t nfs 192.168.23.4:/home/user/nfsroot /mnt/nfs

The barebox NFS driver adds two ``linux.bootargs`` device parameters to the NFS
device. These parameters will be combined into a Linux kernel commandline
snippet containing a suitable root= option for booting from exactly that NFS
share.

Example:

.. code-block:: console

  barebox:/ devinfo nfs0
  ...
  linux.bootargs.root: /dev/nfs
  linux.bootargs.rootopts: nfsroot=192.168.23.4:/home/sha/nfsroot/generic-v7,v3,tcp

The options default to ``v3,tcp`` but can be adjusted before mounting the NFS share with
the ``global.linux.rootnfsopts`` variable
