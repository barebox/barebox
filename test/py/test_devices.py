# SPDX-License-Identifier: GPL-2.0-only

import pytest


def test_devices_present(env, barebox):
    devices = env.config.data["targets"]["main"].get("devices")
    if not devices:
        pytest.skip("No devices listed in YAML to assert existence of")

    for devname in devices.keys():
        assert devices[devname] in barebox.run_check(f"devinfo {devname}")
