File system development in barebox
==================================

The barebox file system support is based heavily on Linux.

Access to all file systems goes through the virtual file system
layer, which provides uniform access to all mounted file systems
under the same root.

As power-fail safe writing of file system couldn't be guaranteed,
most file systems supported by barebox are read-only.
Safe writing is possible, however, via the :ref:`state_framework`.

For an up-to-date listing of writable filesystems, refer to the
``CONFIG_FS_WRITABLE`` Kconfig symbol.

Testing File systems
--------------------

Nearly all file system operations have commands that directly exercise them:

+--------------------------+-----------------------------------------+
| Command                  | Operations                              |
+==========================+=========================================+
| :ref:`command_cat`       | ``open``, ``close``, ``read``           |
+--------------------------+-----------------------------------------+
| :ref:`command_echo`      | ``create``, ``write``                   |
+--------------------------+-----------------------------------------+
| :ref:`command_sync`      | ``flush``                               |
+--------------------------+-----------------------------------------+
| :ref:`command_erase`     | ``erase``                               |
+--------------------------+-----------------------------------------+
| :ref:`command_protect`   | ``protect``                             |
+--------------------------+-----------------------------------------+
| :ref:`command_md`        | ``lseek``, ``memmap``                   |
+--------------------------+-----------------------------------------+
| :ref:`command_rm`        | ``unlink``                              |
+--------------------------+-----------------------------------------+
| :ref:`command_mkdir`     | ``mkdir``                               |
+--------------------------+-----------------------------------------+
| :ref:`command_rmdir`     | ``rmdir``                               |
+--------------------------+-----------------------------------------+
| :ref:`command_ln`        | ``symlink``                             |
+--------------------------+-----------------------------------------+
| :ref:`command_readlink`  | ``readlink``                            |
+--------------------------+-----------------------------------------+
| | :ref:`command_ls`      |  ``opendir``, ``readdir``, ``closedir`` |
| | :ref:`command_tree`    |                                         |
+--------------------------+-----------------------------------------+
| :ref:`command_stat`      | ``stat``                                |
+--------------------------+-----------------------------------------+
| :ref:`command_truncate`  | ``truncate``                            |
+--------------------------+-----------------------------------------+

This leaves two specialized operations that can't be easily tested
via the shell:

- ``discard_range``: Advise that a range need not be preserved
- ``ioctl``: Issue device-specific output and input control commands

Unused metadata
---------------

barebox currently ignores ownership and permission information
inside file systems as well as special nodes like FIFOs or
sockets. When porting file systems, these parts can be omitted.

Background execution
--------------------

Outside command context (i.e. in
:ref:`pollers and secondary barebox threads <background_execution>`),
virtual file system access is only permitted with ramfs.
