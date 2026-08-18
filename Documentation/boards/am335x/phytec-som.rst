Phytec AM335x based SOMs
========================

The phytec-som-am335x is actually not a real board. It represents a set of
am335x based Phytec module families and their boards in barebox.
You can find out more about the Phytec SOM concept on the website:

  http://phytec.com/products/system-on-modules/


Supported modules and boards
----------------------------

Currently, barebox supports the following SOMs and boards:

  - phyCORE

    - PCM-953
    - phyBOARD-MAIA
    - phyBOARD-WEGA

  - phyFLEX

    - PBA-B-01

  - phyCARD

    - PCA-A-XS1


Building phytec-som-am335x
--------------------------

The phytec-som-am335x boards are covered by the ``am335x_mlo_defconfig``
for the MLO and ``omap_defconfig`` for the regular barebox image.
