# SPDX-License-Identifier: GPL-2.0-only

import sys
import os
import re
import subprocess
import struct
from pathlib import Path
from .helper import skip_disabled
import pytest

try:
    from crcmod.predefined import mkPredefinedCrcFun
    _crc32_mpeg = mkPredefinedCrcFun("crc-32-mpeg")
except ModuleNotFoundError:
    _crc32_mpeg = None


class _TLV_Testdata:
    def generator(self, args, expect_failure=False, input=None):
        cmd = [os.sys.executable, str(self.generator_py)] + args
        res = subprocess.run(cmd, text=True, input=input, encoding="utf-8", capture_output=True)
        if res.returncode == 127:
            pytest.skip("test skipped due to missing host dependencies")
        if res.returncode == 0 and expect_failure:
            raise RuntimeError(
                f"`{' '.join(cmd)}` succeded unexpectedly:\n{res.stderr}\n{res.stdout}"
            )
        elif res.returncode != 0 and not expect_failure:
            raise RuntimeError(
                f"`{' '.join(cmd)}` failed unexpectedly with {res.returncode}:\n{res.stderr}\n{res.stdout}"
            )
        return res

    def overwrite_magic(self, new_magic):
        with open(self.schema, "r", encoding="utf-8") as f:
            patched_schema = "".join(
                re.sub(r"^magic:\s*0x[a-fA-F0-9]{8}\s*$", f"magic: {new_magic}\n", line)
                for line in f
            )
        return patched_schema

    def tlv_gen(self, outfile, magic=None, sign=None):
        param = ["--input-data", str(self.data)]
        if sign:
            param += ["--sign", str(sign)]
        if magic:
            param += ["/dev/stdin"]
        else:
            param += [str(self.schema)]
        param += [str(outfile)]
        ret = self.generator(param, input=self.overwrite_magic(magic) if magic else None)
        assert outfile.exists(), f"TLV {outfile} not created from {' '.join(param)}"
        return ret

    def tlv_read(self, binfile, magic=None, verify=None, expect_failure=False):
        param = ["--output-data", "/dev/null"]
        if verify:
            param += ["--verify", str(verify)]
        if magic:
            param += ["/dev/stdin"]
        else:
            param += [str(self.schema)]
        param += [str(binfile)]
        ret = self.generator(
            param,
            input=self.overwrite_magic(magic) if magic else None,
            expect_failure=expect_failure,
        )
        return ret

    def corrupt(self, fnin, fnout, fix_crc=False):
        try:
            from crcmod.predefined import mkPredefinedCrcFun
        except ModuleNotFoundError:
            pytest.skip("test skipped due to missing dependency python-crcmod")
            return

        _crc32_mpeg = mkPredefinedCrcFun("crc-32-mpeg")

        with open(fnin, "r+b") as f:
            data = bytearray(f.read())
        data[0x20] ^= 1
        if fix_crc:
            crc_raw = _crc32_mpeg(data[:-4])
            crc = struct.pack(">I", crc_raw)
            data[-4:] = crc
        with open(fnout, "wb") as f:
            f.write(data)

    def __init__(self, testfs):
        self.dir = Path(testfs)
        self.scripts_dir = Path("scripts/bareboxtlv-generator")
        self.data = self.scripts_dir / "data-example.yaml"
        self.schema = self.scripts_dir / "schema-example.yaml"
        self.generator_py = self.scripts_dir / "bareboxtlv-generator.py"
        self.privkey_rsa = Path("crypto/fit-4096-development.key")
        self.pubkey_rsa = Path("crypto/fit-4096-development.crt")
        self.privkey_ecdsa = Path("crypto/fit-ecdsa-development.key")
        self.pubkey_ecdsa = Path("crypto/fit-ecdsa-development.crt")
        self.unsigned_bin = self.dir / "unsigned.tlv"
        self.corrupted_bin = self.dir / "unsigned_corrupted.tlv"
        self.signed_bin = self.dir / "signed.tlv"
        self.ecdsa_signed_bin = self.dir / "ecdsa-signed.tlv"
        self.tampered_bin = self.dir / "signed-tampered.tlv"
        self.tampered_ecdsa_bin = self.dir / "ecdsa-signed-tampered.tlv"


@pytest.fixture(scope="module")
def tlv_testdata(testfs):
    if _crc32_mpeg is None:
        pytest.skip("missing crcmod dependency")

    t = _TLV_Testdata(testfs)

    t.tlv_gen(t.unsigned_bin)
    t.tlv_gen(t.signed_bin, sign=t.privkey_rsa, magic="0x61bb95f3")
    t.tlv_gen(t.ecdsa_signed_bin, sign=t.privkey_ecdsa, magic="0x61bb95f3")

    t.corrupt(t.unsigned_bin, t.corrupted_bin)
    t.corrupt(t.signed_bin, t.tampered_bin, fix_crc=True)
    t.corrupt(t.ecdsa_signed_bin, t.tampered_ecdsa_bin, fix_crc=True)

    return t


def test_tlv_generator(tlv_testdata):
    t = tlv_testdata
    out_yaml = t.dir / "out.yaml"

    t.tlv_read(t.unsigned_bin)
    t.tlv_read(t.signed_bin, verify=t.pubkey_rsa, magic="0x61bb95f3")
    t.tlv_read(t.ecdsa_signed_bin, verify=t.pubkey_ecdsa, magic="0x61bb95f3")

    t.tlv_read(t.corrupted_bin, expect_failure=True)
    t.tlv_read(t.tampered_bin, verify=t.pubkey_rsa, magic="0x61bb95f3", expect_failure=True)
    t.tlv_read(t.tampered_ecdsa_bin, verify=t.pubkey_ecdsa, magic="0x61bb95f3", expect_failure=True)


@pytest.fixture(scope="module")
def tlv_cmdtest(barebox_config, tlv_testdata):
    skip_disabled(barebox_config, "CONFIG_CMD_TLV")
    skip_disabled(barebox_config, "CONFIG_CRYPTO_BUILTIN_DEVELOPMENT_KEYS")

    class _TLV_Cmdtest:
        def __init__(self, tlv_testdata):
            self.t = tlv_testdata
            with open(tlv_testdata.data, "r", encoding="utf-8") as f:
                self.yaml_lines = [
                    l.strip() for l in f if l.strip() and not l.strip().startswith("#")
                ]

        def test(self, barebox, fn, fail=False):
            cmd = f"tlv /mnt/9p/testfs/{fn}"
            stdout, stderr, exitcode = barebox.run(cmd, timeout=2)
            if fail:
                assert exitcode != 0
                return
            elif exitcode != 0:
                raise RuntimeError(f"`{cmd}` failed with exitcode {exitcode}:\n{stderr}\n{stdout}")

            # work around a corner case of 9pfs printing here (after a failed network test?)
            tlv_offset = next((i for i, line in enumerate(stdout) if line.startswith("tlv")), None)
            tlv_lines = stdout[tlv_offset + 1 : -1]

            assert len(self.yaml_lines) == len(tlv_lines), (
                f"YAML and TLV output line count mismatch for {fn}"
            )

            for yline, tline in zip(self.yaml_lines, tlv_lines):
                m = re.match(r'^\s*([^=]+) = "(.*)";$', tline)
                assert m, f"malformed tlv line: {tline}"
                tkey, tval = m.group(1), m.group(2)
                m = re.match(r'^([^:]+):\s*(?:"([^"]*)"\s*|(.*))$', yline)
                assert m, f"malformed yaml line: {yline}"
                ykey, yval = m.group(1), m.group(2) or m.group(3)
                assert ykey == tkey, f"key mismatch: {ykey} != {tkey}"
                assert str(yval) == str(tval), f"value mismatch for {ykey}: {yval} != {tval}"

    return _TLV_Cmdtest(tlv_testdata)


def test_tlv_cmd_unsigned(barebox, barebox_config, tlv_cmdtest):
    skip_disabled(barebox_config, "CONFIG_CRYPTO_RSA")
    tlv_cmdtest.test(barebox, tlv_cmdtest.t.unsigned_bin.name)


def test_tlv_cmd_signed(barebox, barebox_config, tlv_cmdtest):
    skip_disabled(barebox_config, "CONFIG_CRYPTO_RSA")
    tlv_cmdtest.test(barebox, tlv_cmdtest.t.signed_bin.name)


def test_tlv_cmd_ecdsa_signed(barebox, barebox_config, tlv_cmdtest):
    skip_disabled(barebox_config, "CONFIG_CRYPTO_ECDSA")
    tlv_cmdtest.test(barebox, tlv_cmdtest.t.ecdsa_signed_bin.name)


def test_tlv_cmd_corrupted(barebox, barebox_config, tlv_cmdtest):
    skip_disabled(barebox_config, "CONFIG_CRYPTO_RSA")
    tlv_cmdtest.test(barebox, tlv_cmdtest.t.corrupted_bin.name, fail=True)


def test_tlv_cmd_tampered(barebox, barebox_config, tlv_cmdtest):
    skip_disabled(barebox_config, "CONFIG_CRYPTO_RSA")
    tlv_cmdtest.test(barebox, tlv_cmdtest.t.tampered_bin.name, fail=True)


def test_tlv_cmd_ecdsa_tampered(barebox, barebox_config, tlv_cmdtest):
    skip_disabled(barebox_config, "CONFIG_CRYPTO_ECDSA")
    tlv_cmdtest.test(barebox, tlv_cmdtest.t.tampered_ecdsa_bin.name, fail=True)
