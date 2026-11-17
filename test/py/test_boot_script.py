# SPDX-License-Identifier: GPL-2.0-or-later


def test_boot_script_exit_status(barebox):
    barebox.run_check("echo -o /env/boot/exit3 '#!/bin/sh'")
    barebox.run_check("echo -a /env/boot/exit3 'exit 3'")

    stdout, _, ret = barebox.run("boot exit3")
    assert ret != 0
    assert "ERROR: Booting entry 'exit3' failed: exit status 3" in stdout
    assert not any("Running script" in line for line in stdout)

    barebox.run_check("rm /env/boot/exit3")
