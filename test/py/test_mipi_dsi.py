# SPDX-License-Identifier: GPL-2.0-only

import pytest
from .helper import skip_disabled

DISPLAY_ON = 1 << 2


@pytest.fixture(autouse=True)
def mipi_dsi(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_MIPI_DSI")

    devices = barebox.run_check("mipi_dsi -l")
    if not any(d and "not attached" not in d for d in devices):
        pytest.skip("no attached MIPI DSI device")

    _, _, ret = barebox.run("test -e /dev/fb0")
    if ret != 0:
        pytest.skip("no framebuffer to enable the DSI link")

    barebox.run_check("fb0.enable=1")
    yield
    barebox.run_check("fb0.enable=1")


def power_mode(barebox):
    out = barebox.run_check("mipi_dsi 0a")
    return int(out[-1], 16)


def test_mipi_dsi_identify(barebox):
    out = barebox.run_check("mipi_dsi -i")
    assert any(line.startswith("04: ") and len(line) == 10 for line in out)


def test_mipi_dsi_read_len(barebox):
    assert len(barebox.run_check("mipi_dsi 04")[-1]) == 6
    assert len(barebox.run_check("mipi_dsi -n 1 04")[-1]) == 2


def test_mipi_dsi_dump(barebox):
    out = barebox.run_check("mipi_dsi")
    assert any(line.startswith("0a: ") for line in out)


def test_mipi_dsi_list_verbose(barebox):
    out = barebox.run_check("mipi_dsi -l -v")
    assert any(" lanes, " in line for line in out)


@pytest.mark.parametrize("opts", ["", "-g", "-L", "-H"])
def test_mipi_dsi_display_off_on(barebox, opts):
    assert power_mode(barebox) & DISPLAY_ON

    barebox.run_check(f"mipi_dsi {opts} -w 28")
    assert not power_mode(barebox) & DISPLAY_ON

    barebox.run_check(f"mipi_dsi {opts} -w 29")
    assert power_mode(barebox) & DISPLAY_ON


def test_mipi_dsi_channel(barebox):
    barebox.run_check("mipi_dsi -c 0 0a")

    _, _, ret = barebox.run("mipi_dsi -c 1 0a")
    assert ret != 0


def test_mipi_dsi_peripheral(barebox):
    barebox.run_check("mipi_dsi -p on")


def test_mipi_dsi_powered_down(barebox):
    barebox.run_check("fb0.enable=0")

    out, _, ret = barebox.run("mipi_dsi -w 29")
    assert ret != 0
    assert "DSI host is powered down" in "\n".join(out)
