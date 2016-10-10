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

Storage Backends
----------------

The serialized data can be stored to different backends which are automatically
selected depending on the defined backend in the devicetree. Currently two
implementations exist, ``circular`` and ``direct``. ``circular`` writes the
data sequentially on the backend storage device. Each save is appended until
the storage area is full. It then erases the block and starts from offset 0.
``circular`` is used for MTD devices with erase functionality. ``direct``
writes the data directly to the file without erasing.

For all backends multiple copies are written to handle read errors.

Commands
--------

The ``state`` command can be used to store and manipulate the state. Using
``state`` without argument lists you all available states with their name.
``devinfo STATE_NAME`` shows you all variables and their values. ``state -s``
stores the state.

Starting Barebox will automatically load the last written state. If loading the
state fails the defaults are used.
