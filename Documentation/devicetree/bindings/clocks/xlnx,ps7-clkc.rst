Xilinx PS7 clkc
===============

In addition to the upstream bindings, the following properties are understood:

Optional properties:

- ``ps-clk-frequency`` : Overrides the ps clock frequency set by the driver.
  By default the clock is set to 33.3MHz. When this property is set, the frequency
  is overwritten by the devicetree property.
