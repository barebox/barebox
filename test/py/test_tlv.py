# SPDX-License-Identifier: GPL-2.0-only

import sys
import os
import re
import subprocess
from pathlib import Path
from .helper import skip_disabled
import pytest

try:
    from crcmod.predefined import mkPredefinedCrcFun
    _crc32_mpeg = mkPredefinedCrcFun("crc-32-mpeg")
except ModuleNotFoundError:
    _crc32_mpeg = None


class _TLV_Testdata:
    def generator(self, args, check=True):
        cmd = [os.sys.executable, str(self.generator_py)] + args
        res = subprocess.run(cmd, text=True)
        if check and res.returncode != 0:
            raise RuntimeError(f"generator failed ({res.returncode}): {res.stdout}\n{res.stderr}")
        return res

    def __init__(self, testfs):
        self.dir = Path(testfs)
        self.scripts_dir = Path("scripts/bareboxtlv-generator")
        self.data = self.scripts_dir / "data-example.yaml"
        self.schema = self.scripts_dir / "schema-example.yaml"
        self.generator_py = self.scripts_dir / "bareboxtlv-generator.py"
        self.unsigned_bin = self.dir / 'unsigned.tlv'
        self.corrupted_bin = self.dir / 'unsigned_corrupted.tlv'

@pytest.fixture(scope="module")
def tlv_testdata(testfs):
    if _crc32_mpeg is None:
        pytest.skip("missing crcmod dependency")

    t = _TLV_Testdata(testfs)
    t.generator(["--input-data", str(t.data), str(t.schema), str(t.unsigned_bin)])
    assert t.unsigned_bin.exists(), "unsigned TLV not created"

    with open(t.unsigned_bin, 'r+b') as f:
        data = bytearray(f.read())
    data[0x20] ^= 1
    with open(t.corrupted_bin, "wb") as f:
        f.write(data)

    return t

def test_tlv_generator(tlv_testdata):
    t = tlv_testdata
    out_yaml = t.dir / 'out.yaml'


    good = t.generator(["--output-data", str(out_yaml), str(t.schema), str(t.unsigned_bin)], check=False)
    assert good.returncode == 0, f"valid unsigned TLV failed to decode: {good.stderr}\n{good.stdout}"

    bad = t.generator(["--output-data", str(t.dir / 'bad.yaml'), str(t.schema), str(t.corrupted_bin)], check=False)
    assert bad.returncode != 0, "unsigned TLV with invalid CRC unexpectedly decoded successfully"

def test_tlv_command(barebox, barebox_config, tlv_testdata):
    skip_disabled(barebox_config, "CONFIG_CMD_TLV")
    t = tlv_testdata
    with open(t.data, 'r', encoding='utf-8') as f:
        yaml_lines = [l.strip() for l in f if l.strip() and not l.strip().startswith('#')]

    stdout = barebox.run_check(f"tlv /mnt/9p/testfs/{t.unsigned_bin.name}")

    # work around 9pfs printing here after a failed network test
    tlv_offset = next((i for i, line in enumerate(stdout) if line.startswith("tlv")), None)
    tlv_lines = stdout[tlv_offset + 1:-1]

    assert len(yaml_lines) == len(tlv_lines), \
        f"YAML and TLV output line count mismatch for {t.unsigned_bin.name}"

    for yline, tline in zip(yaml_lines, tlv_lines):
        m = re.match(r'^\s*([^=]+) = "(.*)";$', tline)
        assert m, f"malformed tlv line: {tline}"
        tkey, tval = m.group(1), m.group(2)
        m = re.match(r'^([^:]+):\s*(?:"([^"]*)"\s*|(.*))$', yline)
        assert m, f"malformed yaml line: {yline}"
        ykey, yval = m.group(1), m.group(2) or m.group(3)
        assert ykey == tkey, f"key mismatch: {ykey} != {tkey}"
        assert str(yval) == str(tval), f"value mismatch for {ykey}: {yval} != {tval}"
