Voltage/Current Regulators
==========================

In addition to the upstream bindings, another property is added:

Optional properties:
- ``barebox,allow-dummy-supply`` : A property to allow usage of dummy power
  regulator. This can be added to regulator nodes, whose drivers are not yet
  supported. It will rely on regulator reset defaults and use of dummy regulator
  instead.

Examples:

.. code-block:: none

  pmic@58 {
	pinctrl-names = "default";
	pinctrl-0 = <&pinctrl_pmic>;
	compatible = "dlg,da9063";
	reg = <0x58>;

	regulators {
		barebox,allow-dummy-supply;

		vddcore_reg: bcore1 {
			regulator-min-microvolt = <730000>;
			regulator-max-microvolt = <1380000>;
		};

		vddsoc_reg: bcore2 {
			regulator-min-microvolt = <730000>;
			regulator-max-microvolt = <1380000>;
		};
	}
  }
