
.. _imd:

Image MetaData (IMD)
====================

barebox images can be enriched with metadata. This is useful to get information
the board an image is compiled for and which barebox version an image contains.

There are predefined tags for:

- The build timestamp
- The barebox release version
- The model (board) the image is compiled for
- The toplevel device tree compatible properties the image can handle

Additionally there is a generic key/value tag to add information which does not
fit into the above categories, for example the memory size for boards which come
with different memory sizes which can't be automatically detected.

The informations can be extracted with the ``bareboximd`` tool which lives under
``scripts/`` in the barebox sourcecode. If enabled it is compiled for the compile
host and also for the target architecture. barebox itself has the :ref:`command_imd`
command to extract the informations. Here is an example output of the tool called
without additional options:

.. code-block:: none

  # imd barebox-phytec-pbab01dl-1gib.img
  build: #890 Wed Jul 30 16:15:24 CEST 2014
  release: 2014.07.0-00167-ge6632a9-dirty
  parameter: memsize=1024
  of_compatible: phytec,imx6x-pbab01 phytec,imx6dl-pfla02 fsl,imx6dl
  model: Phytec phyFLEX-i.MX6 Duallite Carrier-Board

Single informations can be extracted with the ``-t <type>`` option:

.. code-block:: none

  # imd barebox-phytec-pbab01dl-1gib.img -t release
  2014.07.0-00167-ge6632a9-dirty

Since the barebox hush does not have output redirection the barebox too has the
``-s <var>`` option to assign the output to a variable for later evaluation.

Limitations
-----------

The IMD tags are generated in the barebox binary before a SoC specific image is
generated. Some SoCs encrypt or otherwise manipulate the images in a way that the
IMD information is lost. The IMD mechanism does not work on these SoCs. A known
example is the Freescale i.MX28.

IMD and barebox_update
----------------------

The IMD informations could well be used to check if an image is suitable for updating
barebox for a particular board. Support for such a check is planned but not yet implemented.
