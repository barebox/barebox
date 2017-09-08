Dallas DS1307 I2C Serial Real-Time Clock
========================================

Required properties:

* ``compatible``: ``dallas,ds1307``, ``dallas,ds1308``, ``dallas,ds1338``
	"maxim" can be used in place of "dallas"

* ``reg``: I2C address for chip

Optional properties:

* ``ext-clock-input``: Enable external clock input pin
* ``ext-clock-output``:  Enable square wave output.  The above two
  properties are mutually exclusive
* ``ext-clock-bb``: Enable external clock on battery power
* ``ext-clock-rate``:  Expected/Generated rate on external clock pin
  in Hz.  Allowable values are 1, 50, 60, 4096, 8192, and 32768 Hz.
  Not all values are valid for all configurations.

The default is ext-clock-input, ext-clock-output, and ext-clock-bb
disabled and ext-clock-rate of 1 Hz.

Example

.. code-block:: text

	ds1307: rtc@68 {
		compatible = "dallas,ds1307";
		reg = <0x68>;
	};

	ds1308: rtc@68 {
		compatible = "maxim,ds1308";
		reg = <0x68>;
		ext-clock-output;
		ext-clock-rate = <32768>;
	};
