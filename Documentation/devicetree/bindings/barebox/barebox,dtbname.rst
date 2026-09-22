.. _barebox,dtbname:

barebox,dtbname property
========================

Device trees built as part of barebox carry the name they were built under
in ``barebox,dtbname``: the blob's path relative to its build directory,
except under ``dts/src``, where the architecture directory is dropped but the
vendor directory kept, so the name is the one Linux uses::

  arch/arm/dts/imx6q-foo.dts        -> imx6q-foo.dtb
  dts/src/arm/nxp/imx/imx6q-foo.dts -> nxp/imx/imx6q-foo.dtb

Overlays are not stamped; they are known by the tree they end up in.

barebox uses the property to tell its own device trees from one handed to it
by a preceding boot stage: a foreign tree still boots, but is warned about,
as it describes only what Linux of its vintage needed.
