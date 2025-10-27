# SPDX-License-Identifier: GPL-2.0-or-later

import pytest
import os
import subprocess
import shutil
from .helper import skip_disabled


def pad128k(f):
    f.write(b"\0" * 128 * 1024)


@pytest.fixture(scope="module")
def dm_testdata(testfs):
    path = os.path.join(testfs, "dm")
    os.makedirs(path, exist_ok=True)
    cwd = os.getcwd()
    os.chdir(path)

    with open("latin", "wb") as f:
        pad128k(f)
        f.write(b"veritas vos liberabit")
        pad128k(f)

    with open("english", "wb") as f:
        pad128k(f)
        f.write(b"truth will set you free")
        pad128k(f)

    try:
        subprocess.run(["truncate", "-s", "1M", "good.fat"], check=True)
        subprocess.run(["mkfs.vfat", "good.fat"], check=True)
        subprocess.run(["mcopy", "-i", "good.fat", "latin", "english", "::"],
                       check=True)

        subprocess.run([
            "veritysetup", "format",
            "--root-hash-file=good.hash",
            "good.fat", "good.verity"
        ], check=True)
    except FileNotFoundError as e:
        os.chdir(cwd)
        shutil.rmtree(path)
        pytest.skip(f"missing dependency: {e}")

    with open("good.fat", "rb") as f:
        data = f.read()
    with open("bad.fat", "wb") as f:
        f.write(data.replace(
            b"truth will set you free",
            b"LIAR LIAR PANTS ON FIRE"
        ))

    os.chdir(cwd)

    yield path

    shutil.rmtree(path)


@pytest.fixture(autouse=True)
def reset_pwd(barebox):
    yield
    barebox.run("cd")


def test_dm_verity(barebox, barebox_config, dm_testdata):
    skip_disabled(barebox_config, "CONFIG_CMD_VERITYSETUP")

    barebox.run_check("cd /mnt/9p/testfs/dm")

    # Since commands run in a subshell, export the root hash in a
    # global, so that we can access it from subsequent commands
    barebox.run_check("readf good.hash roothash && global roothash=$roothash")

    barebox.run_check("veritysetup open good.fat good good.verity $global.roothash")
    barebox.run_check("veritysetup open bad.fat  bad  good.verity $global.roothash")

    barebox.run_check("md5sum /mnt/good/latin /mnt/good/english")

    # 'latin' has not been modified, so it should read fine even from
    # 'bad'
    barebox.run_check("md5sum /mnt/bad/latin")

    # 'english' however, does not match the data in the hash tree and
    # MUST thus fail
    _, _, returncode = barebox.run("md5sum /mnt/bad/english")
    assert returncode != 0, "'english' should not be readable from 'bad'"

    barebox.run_check("umount /dev/good")
    barebox.run_check("veritysetup close good")

    barebox.run_check("umount /dev/bad")
    barebox.run_check("veritysetup close bad")
