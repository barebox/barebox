Memory areas
============

Several barebox commands like :ref:`command_md`, :ref:`command_erase`
or :ref:`command_crc32` work on an area of memory. Areas have the following form::

  <start>-<end>

or::

  <start>+<count>

start, end and count are interpreted as decimal values if not prefixed with 0x.
Additionally these can be postfixed with K, M or G for kilobyte, megabyte or
gigabyte.

Examples
--------

Display a hexdump from 0x80000000 to 0x80001000 (inclusive):

.. code-block:: sh

  md 0x80000000-0x80001000

Display 1 kilobyte of memory starting at 0x80000000:

.. code-block:: sh

  md 0x80000000+1k

