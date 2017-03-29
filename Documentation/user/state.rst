.. _state_framework:

Barebox State Framework
=======================

The state framework is build to exchange data between Barebox and Linux
userspace using a non-volatile storage. There are several components involved.
Barebox has a state driver to access the variables. For the Linux Userspace
there is a userspace tool.

Devicetree
----------

Currently the devicetree defines the layout of the variables and data.
Variables are fixed size. Several types are supported, see the binding
documentation for details.

Data Formats
------------

The state data can be stored in different ways. Currently two formats are
available, ``raw`` and ``dtb``. Both format the state data differently.
Basically these are serializers. The raw serializer additionally supports a
HMAC algorithm to detect manipulations.

The data is always stored in a logical unit called ``bucket``. A ``bucket`` has
its own size depending on some external contraints. These contraints are listed
in more detail below depending on the used memory type and storage backend. A
``bucket`` stores exactly one state. A default number of three buckets is used
to store data redundantely.

Redundancy
----------

The state framework is safe against powerfailures during write operations. To
archieve that multiple buckets are stored to disk. When writing all buckets are
written in order. When reading, the buckets are read in order and the first
one found that passes CRC tests is used. When all data is read the buckets
containing invalid or outdated data are written with the data just read. Also
NAND blocks need cleanup due to excessive bitflips are rewritten in this step.
With this it is made sure that after successful initialization of a state the
data on the storage device is consistent and redundant.

Storage Backends
----------------

The serialized data can be stored to different backends. Currently two
implementations exist, ``circular`` and ``direct``. The state framework automatically
selects the correct backend depending on the storage medium. Media requiring
erase operations (NAND, NOR flash) use the ``circular`` backend, others use the ``direct``
backend. The purpose of the ``circular`` backend is to save erase cycles which may
wear out the flash blocks. It continuously fills eraseblocks with updated data
and only when an eraseblock if fully written erases it and starts over writing
new data to the same eraseblock again.

For all backends multiple copies are written to handle read errors.

Commands
--------

The ``state`` command can be used to store and manipulate the state. Using
``state`` without argument lists you all available states with their name.
``devinfo STATE_NAME`` shows you all variables and their values. ``state -s``
stores the state.

Starting Barebox will automatically load the last written state. If loading the
state fails the defaults are used.
