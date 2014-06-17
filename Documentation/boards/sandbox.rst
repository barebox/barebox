Sandbox
=======

barebox can be run as a simulator on your host to check and debug new non
hardware related features.

Build barebox for simulation
----------------------------

the barebox sand box can be built with the host compiler::

  ARCH=sandbox make sandbox_defconfig
  ARCH=sandbox make

Run sandbox
-----------

::

  $ barebox [\<OPTIONS\>]

Options can be::

  -m, --malloc=\<size\>

Start sandbox with a specified malloc-space \<size\> in bytes.

::

  -i \<file\>

Map a \<file\> to barebox. This option can be given multiple times. The \<file\>s
will show up as /dev/fd0 ... /dev/fdx in the barebox simulator.

::

  -e \<file\>

Map \<file\> to barebox. With this option \<file\>s are mapped as /dev/env0 ...
/dev/envx and thus are used as default environment. A clean file generated
with dd will do to get started with an empty environment

::

  -O \<file\>

Register \<file\> as a console capable of doing stdout. \<file\> can be a
regular file or a fifo.

::

  -I \<file\>

Register \<file\> as a console capable of doing stdin. \<file\> can be a regular
file or a fifo.

::

  -x, --xres \<res\>

Specify SDL width

::

  -y, --yres \<res\>

Specify SDL height
