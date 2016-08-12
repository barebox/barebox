Variables
=========

We already have seen some variables in :ref:`device_parameters`. But
there is more to variables.

.. _global_device:

The ``global`` device
---------------------

The ``global`` device is a special purpose device. It only exists as a
storage for global variables. Some global variables are created internally
in barebox (see :ref:`magicvars`). Additional variables can be created with
the :ref:`command_global` command::

  global myvar

This creates the ``global.myvar`` variable which now can be used like any
other variable. You can also directly assign a value during creation::

  global myvar1=foobar

**NOTE:** creating a variable with ``global myvar1=foobar`` looks very similar
to assigning a value with ``global.myvar1=foobar``, but the latter fails when
a variable has not been created before.

.. _config_device:

Non volatile variables
----------------------

Additionally to global variables barebox also has non volatile (nv) variables.
Unlike the global variables the config variables are persistent over reboots.

Each nv variable is linked with the global variable of the same name.
Whenever the nv variable changes its value the corresponding global
variable also changes its value. The other way round though is not true:
Changing a global variable does not change the corresponding nv variable.
This means that changing a global variable changes the behaviour for the
currently running barebox, while changing a nv variable changes the
behaviour persistently over reboots.

nv variables can be created or removed with the :ref:`command_nv`
command. The nv variables are made persistent using the environment
facilities of barebox, so a :ref:`command_saveenv` must be issued to store the
actual values.

examples:

.. code-block:: sh

  barebox@Phytec phyCARD-i.MX27:/ devinfo nv
  barebox@Phytec phyCARD-i.MX27:/ nv model=myboard
  barebox@myboard:/ devinfo nv
  Parameters:
    model: myboard
  barebox@myboard:/ devinfo global
  Parameters:
    [...]
    model: myboard
    [...]
  barebox@myboard:/ global.model=yourboard
  barebox@yourboard:/ devinfo nv
  Parameters:
    model: myboard
  barebox@yourboard:/

Non volatile device variables
-----------------------------

Non volatile device variables are used to make device parameters persistent. They
are regular nv variables but are linked with other devices instead of the global
device. Every nv variable in the form nv.dev.<devname>.<paramname> will be mirrored
to the corresponding <devname>.<paramname> variable.

This example changes the partitioning of the nand0 device:

.. code-block:: sh

  barebox@Phytec phyCARD-i.MX27:/ nv dev.nand0.partitions: 4M(barebox),1M(barebox-environment),-(root)
  barebox@Phytec phyCARD-i.MX27:/ devinfo nand0
    Parameters:
    [...]
    partitions: 4M(barebox),1M(barebox-environment),8M(kernel),1011M(root)
    [...]

.. _magicvars:

Magic variables
---------------

Some variables have special meanings and influence the behaviour
of barebox. Most but not all of them are consolidated in the :ref:`global_device`.
Since it's hard to remember which variables these are and if the current
barebox has support for them the :ref:`command_magicvar` command can print a list
of all variables with special meaning along with a short description::

  barebox:/ magicvar
  OPTARG                           optarg for hush builtin getopt
  PATH                             colon separated list of paths to search for executables
  PS1                              hush prompt
  armlinux_architecture            ARM machine ID
  armlinux_system_rev              ARM system revision
  armlinux_system_serial           ARM system serial
  automount_path                   mountpath passed to automount scripts
  bootargs                         Linux Kernel parameters
  bootsource                       The source barebox has been booted from
  bootsource_instance              The instance of the source barebox has been booted from
  global.boot.default              default boot order
  ...

