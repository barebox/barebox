# SPDX-License-Identifier: GPL-2.0-only

import pytest
import re

# --- Essential Configuration Check ---

# This list should contain the CONFIG strings for all commands and features
# that are absolutely essential for the NVMEM test suite to run.
ESSENTIAL_CONFIGS = [
    "CONFIG_CMD_NVMEM",       # For the 'nvmem' command itself
    "CONFIG_CMD_MD",          # For the 'md' (memory display) command
    "CONFIG_CMD_MW",          # For the 'mw' (memory write) command
    "CONFIG_CMD_FLASH",       # For 'protect' and 'unprotect' commands
    "CONFIG_NVMEM_RMEM",      # For creating dynamic rmem NVMEM devices
    "CONFIG_FS_DEVFS"         # For /dev/ device character file paths
]


@pytest.fixture(scope="session", autouse=True)
def check_essential_barebox_configs(barebox_config):
    """
    Session-scoped autouse fixture to check for essential Barebox CONFIGs.
    If any essential CONFIG is not enabled in 'barebox_config',
    this fixture will skip all tests in the current session.
    """
    missing_configs = []
    if not barebox_config:
        pytest.skip(
            "Skipping all NVMEM tests: Barebox config not available."
        )
        return

    for config_opt in ESSENTIAL_CONFIGS:
        if config_opt not in barebox_config:
            missing_configs.append(config_opt)

    if missing_configs:
        pytest.skip(
            "Skipping all NVMEM tests: Essential Barebox CONFIGs missing: "
            f"{', '.join(missing_configs)}"
        )


# --- Individual Test Skip Helper ---
def skip_disabled(barebox_config, *config_options_needed):
    if not barebox_config:
        pytest.skip(
            "Test skipped: Barebox config not available for this test."
        )
        return
    missing_for_this_test = []
    for config_opt in config_options_needed:
        if config_opt not in barebox_config:
            missing_for_this_test.append(config_opt)
    if missing_for_this_test:
        pytest.skip(
            "Test skipped, requires specific Barebox CONFIGs: "
            f"{', '.join(missing_for_this_test)}"
        )


# --- Constants ---
NVMEM_MAX_DYNAMIC_SIZE = 1024 * 1024
TEST_ENV_VAR_NAME = "TESTNVMEMDEV"
DEFAULT_DEVICE_SIZE = 1024


# --- Helper Functions ---
def string_to_hex_bytes_for_mw(s: str) -> str:
    return " ".join([f"0x{ord(c):02x}" for c in s])


def parse_md_output(output_lines: list[str],
                    access_size_char: str = 'b') -> str:
    hex_data = []

    unit_hex_len = 0
    if access_size_char == 'b':
        unit_hex_len = 2
    elif access_size_char == 'w':
        unit_hex_len = 4
    elif access_size_char == 'l':
        unit_hex_len = 8
    elif access_size_char == 'q':
        unit_hex_len = 16  # For 64-bit quad
    else:
        return ""

    hex_part_regex_str = r"^[0-9a-fA-F]+:\s+((?:[0-9a-fA-F]{" + \
                         str(unit_hex_len) + r"}\s*)+)"
    hex_part_regex = re.compile(hex_part_regex_str)

    for i, line in enumerate(output_lines):
        match = hex_part_regex.match(line)
        if match:
            captured_group1 = match.group(1)
            data_part = "".join(captured_group1.split())
            hex_data.append(data_part)

    full_hex_string = "".join(hex_data)
    return full_hex_string.lower()


def parse_md_word_value(output_lines: list[str]) -> int | None:  # For 16-bit
    hex_string = parse_md_output(output_lines, 'w')
    if hex_string and len(hex_string) == 4:  # 16-bit word is 4 hex chars
        try:
            return int(hex_string, 16)
        except ValueError:
            return None
    return None


def parse_md_long_value(output_lines: list[str]) -> int | None:  # For 32-bit
    hex_string = parse_md_output(output_lines, 'l')
    if hex_string and len(hex_string) == 8:
        try:
            return int(hex_string, 16)
        except ValueError:
            return None
    return None


def parse_md_bytes_to_string(output_lines: list[str],
                             expected_num_bytes: int) -> str | None:
    hex_string = parse_md_output(output_lines, 'b')

    expected_hex_len = expected_num_bytes * 2
    if len(hex_string) > expected_hex_len:
        hex_string = hex_string[:expected_hex_len]

    if hex_string and len(hex_string) == expected_hex_len:
        try:
            if len(hex_string) % 2 != 0:
                return None

            bytes_obj = bytes.fromhex(hex_string)
            decoded_string = bytes_obj.decode('ascii', errors='replace')
            return decoded_string
        except ValueError:
            return None
    return None


# --- Pytest Fixtures (test-specific setup) ---
@pytest.fixture(scope="function")
def created_nvmem_device_name(barebox, barebox_config):
    """
    Creates a new dynamic NVMEM rmem device and yields its actual name
    suitable for /dev/ path construction (e.g., "rmem0").
    This fixture assumes sequential execution and that it can reliably
    determine the newly created device by diffing `nvmem` output.
    """
    stdout_before, _, _ = barebox.run('nvmem')
    rmem_devices_before = {
        line.strip() for line in stdout_before if line.strip().startswith('rmem')
    }

    create_cmd = f'nvmem -c {DEFAULT_DEVICE_SIZE}'
    barebox.run_check(create_cmd)

    stdout_after, _, _ = barebox.run('nvmem')
    rmem_devices_after = {
        line.strip() for line in stdout_after if line.strip().startswith('rmem')
    }

    newly_created_devices = rmem_devices_after - rmem_devices_before
    if not newly_created_devices:
        pytest.fail("Failed to identify newly created NVMEM device: "
                    "No new 'rmem' devices found in 'nvmem' list.")

    actual_device_name_listed = ""
    if len(newly_created_devices) > 1:
        for dev_name in newly_created_devices:
            if re.match(r"rmem\d+0$", dev_name):
                actual_device_name_listed = dev_name
                break
        if not actual_device_name_listed:
            # Fallback if no "rmemX0" found among multiple new devices
            actual_device_name_listed = list(newly_created_devices)[0]
    else:
        actual_device_name_listed = list(newly_created_devices)[0]

    # Derive the cdev name (e.g., "rmem0") from the listed name (e.g., "rmem00")
    cdev_name_match = re.match(r"(rmem\d+)0$", actual_device_name_listed)
    if cdev_name_match:
        final_cdev_name = cdev_name_match.group(1)
    else:
        # If it doesn't end with '0' (e.g. it's already 'rmemX')
        cdev_name_match_direct = re.match(r"(rmem\d+)$",
                                          actual_device_name_listed)
        if not cdev_name_match_direct:
            pytest.fail(
                f"Could not derive cdev base name from listed name "
                f"'{actual_device_name_listed}' using patterns (rmemX)0 or (rmemX)."
            )
        final_cdev_name = cdev_name_match_direct.group(1)
    yield final_cdev_name


# --- Test Functions ---

# == NVMEM Command Tests ==
def test_nvmem_list_empty_or_static(barebox, barebox_config):
    """
    Tests the `nvmem` command without arguments.
    It should successfully execute and list any currently available NVMEM devices
    (which might include static ones or be empty if none are configured/created).

    Manual Reproduction:
    1. In Barebox shell, run: `nvmem`
    2. Observe the output. The command should not error.
    """
    _, _, returncode = barebox.run('nvmem')
    assert returncode == 0, "`nvmem` command failed"


def test_nvmem_create_dynamic_device(barebox, barebox_config):
    """
    Tests creating a dynamic NVMEM rmem device using `nvmem -c <size>`.
    It verifies that after creation, a new rmem device appears in the `nvmem` list.

    Manual Reproduction:
    1. In Barebox shell, run: `nvmem` (to see initial list)
    2. Run: `nvmem -c 1k`
    3. Run: `nvmem` (again, to see the new device, e.g., "rmem00")
    """
    stdout_before, _, _ = barebox.run('nvmem')
    rmem_devices_before = {
        line.strip() for line in stdout_before if line.strip().startswith('rmem')
    }
    barebox.run_check('nvmem -c 1k')  # Create a 1KB device
    stdout_after, _, returncode = barebox.run('nvmem')
    assert returncode == 0, "`nvmem` command failed after creating a device"
    rmem_devices_after = {
        line.strip() for line in stdout_after if line.strip().startswith('rmem')
    }
    new_devices = rmem_devices_after - rmem_devices_before
    assert len(new_devices) == 1, (
        f"Expected 1 new rmem device, found {len(new_devices)}. "
        f"New: {new_devices}"
    )
    new_device_name = list(new_devices)[0]
    assert re.match(r"rmem\d+0?$", new_device_name), (
        f"New device name '{new_device_name}' does not match expected "
        "'rmemX' or 'rmemX0' pattern."
    )


def test_nvmem_create_dynamic_device_with_var(barebox, barebox_config):
    """
    Tests `nvmem -c <size> -v <varname>` functionality.
    It verifies:
    1. The specified environment variable is set to the created device's cdev name (e.g., "rmem1").
    2. The device (e.g., "rmem1" or "rmem10") is listed by the `nvmem` command.
    This test uses a compound command to ensure the `echo` sees the variable set by `nvmem -v`.

    Manual Reproduction:
    1. In Barebox shell, run: `nvmem` (to see current rmem device count, e.g., rmem0 exists)
    2. Run: `nvmem -c 2k -v MYVAR` (this should create, e.g., rmem1)
    3. Run: `echo ${MYVAR}` (should output, e.g., "rmem1")
    4. Run: `nvmem` (should list the new device, e.g., "rmem10")
    """
    var_to_set_in_barebox = "VTESTVAR_COMPOUND"

    stdout_list_b, _, _ = barebox.run('nvmem')
    # Count existing 'rmem' entries to predict the next index.
    num_current_rmem = sum(1 for line in stdout_list_b if line.strip().startswith('rmem'))
    expected_dev_name_in_var = f"rmem{num_current_rmem}"  # e.g., rmem0, rmem1

    # Compound command to create device and echo the variable in the same shell context
    compound_cmd = (
        f'nvmem -c 2k -v {var_to_set_in_barebox} && '
        f'echo ${{{var_to_set_in_barebox}}}'
    )

    stdout_compound_raw = barebox.run_check(compound_cmd)

    assert expected_dev_name_in_var in stdout_compound_raw

    # Verify the device is listed by `nvmem`
    stdout_list_a, _, returncode = barebox.run('nvmem')
    assert returncode == 0

    # The 'nvmem' list might show "rmemX0" while the var stores "rmemX".
    # The pattern checks for this.
    list_pattern_check = expected_dev_name_in_var
    pattern = re.compile(f"^{re.escape(list_pattern_check)}0?$")
    assert any(pattern.match(line.strip()) for line in stdout_list_a), (
        f"Device corresponding to '{list_pattern_check}' (pattern: '{pattern.pattern}') "
        f"not found in `nvmem` list. List was: {stdout_list_a}"
    )


def test_nvmem_create_invalid_size_zero(barebox, barebox_config):
    """
    Tests `nvmem -c 0` (creating a device with zero size).
    This operation should fail and the command should return a non-zero exit status
    and print an error message.

    Manual Reproduction:
    1. In Barebox shell, run: `nvmem -c 0`
    2. Observe the error message (e.g., "Error: Invalid size '0'...")
    3. Run: `echo $?` (should be non-zero)
    """
    stdout, _, returncode = barebox.run('nvmem -c 0')
    assert returncode != 0, "`nvmem -c 0` should fail (non-zero return code)"
    assert any("Error: Invalid size '0'" in line for line in stdout), \
        "Expected error message for zero size not found in stdout."


def test_nvmem_create_invalid_size_too_large(barebox, barebox_config):
    """
    Tests `nvmem -c <size>` where size exceeds NVMEM_MAX_DYNAMIC_SIZE.
    This operation should fail, return a non-zero exit status, and print an error.

    Manual Reproduction:
    1. In Barebox shell, run: `nvmem -c 2000000` (assuming 2MB > NVMEM_MAX_SIZE)
    2. Observe the error message (e.g., "Error: Size '...' exceeds maximum...")
    3. Run: `echo $?` (should be non-zero)
    """
    invalid_size = NVMEM_MAX_DYNAMIC_SIZE + 1
    stdout, stderr, returncode = barebox.run(f'nvmem -c {invalid_size}')

    assert returncode != 0, (
        f"`nvmem -c {invalid_size}` was expected to fail (non-zero returncode), "
        f"but got {returncode}."
    )
    assert any(f"Error: Size '{invalid_size}' exceeds maximum" in line
               for line in stdout), (
        f"Expected error message for excessive size not found in stdout. "
        f"Stdout was: {stdout!r}"
    )


def test_nvmem_option_v_without_c(barebox, barebox_config):
    """
    Tests using `nvmem -v <varname>` without the required `-c <size>` option.
    This should be a usage error, returning a non-zero exit status and an error message.

    Manual Reproduction:
    1. In Barebox shell, run: `nvmem -v somevar`
    2. Observe the error message (e.g., "Error: -v <VARNAME> option requires -c <SIZE>...")
    3. Run: `echo $?` (should be non-zero)
    """
    stdout, _, returncode = barebox.run('nvmem -v somevarname')
    assert returncode != 0, "`nvmem -v` without `-c` should fail"
    assert any("Error: -v <VARNAME> option requires -c <SIZE>" in line
               for line in stdout), \
        "Expected error for missing -c with -v not found."


def test_nvmem_help_on_h_option(barebox, barebox_config):
    """
    Tests `nvmem -h` for displaying help/usage information.
    Typically, help options result in a non-zero exit status after printing usage.

    Manual Reproduction:
    1. In Barebox shell, run: `nvmem -h`
    2. Observe the usage information.
    3. Run: `echo $?` (often non-zero for help options)
    """
    stdout, _, returncode = barebox.run('nvmem -h')
    assert returncode != 0, "`nvmem -h` should return non-zero"
    assert any("Usage: nvmem" in line for line in stdout) or \
           any("Options:" in line for line in stdout), \
           "Help output (Usage:/Options:) not found for `nvmem -h`."


# == CDEV Read/Write Tests (using mw and md) ==
def test_nvmem_cdev_mw_md_long(barebox, barebox_config,
                               created_nvmem_device_name):
    """
    Tests writing a 32-bit long value to an NVMEM device using `mw -l`
    and reading it back using `md -l` for verification.

    Manual Reproduction (assuming rmem0 is created):
    1. Create device: `nvmem -c 1k` (note the device name, e.g., rmem0)
    2. Write: `mw -d /dev/rmem0 -l 0x170 0x12345678`
    3. Read: `md -s /dev/rmem0 -l 0x170+4` (verify output shows 12345678)
    """
    actual_dev_name = created_nvmem_device_name

    if not actual_dev_name:
        pytest.fail("Fixture created_nvmem_device_name returned empty. Cannot proceed.")

    dev_path = f"/dev/{actual_dev_name}"

    test_val_hex = "0x12345678"
    test_addr_hex = "0x170"

    mw_cmd = f'mw -d {dev_path} -l {test_addr_hex} {test_val_hex}'
    try:
        barebox.run_check(mw_cmd)
    except Exception as e:
        pytest.fail(f"mw command '{mw_cmd}' failed: {e}")

    md_cmd = f'md -s {dev_path} -l {test_addr_hex}+4'
    stdout_md = barebox.run_check(md_cmd)

    read_val_int = parse_md_long_value(stdout_md)
    assert read_val_int is not None, (
        f"Failed to parse long value from md output: {stdout_md}"
    )
    assert read_val_int == int(test_val_hex, 16), \
        "Read long value does not match written value."


def test_nvmem_cdev_mw_md_bytes_string(barebox, barebox_config, created_nvmem_device_name):
    """
    Tests writing a string as a sequence of bytes to an NVMEM device using `mw -b`
    and reading it back using `md -b`, then verifying the string content.

    Manual Reproduction (assuming rmem0 is created):
    1. Create device: `nvmem -c 1k`
    2. Write "Test": `mw -d /dev/rmem0 -b 0x40 0x54 0x65 0x73 0x74`
    3. Read: `md -s /dev/rmem0 -b 0x40+4` (verify ASCII part shows "Test")
    """
    actual_dev_name = created_nvmem_device_name
    if not actual_dev_name:
        pytest.fail("Fixture created_nvmem_device_name returned empty")
    dev_path = f"/dev/{actual_dev_name}"

    test_string = "BareboxNVMEMTest"  # Length 16
    hex_bytes_for_mw_cmd = string_to_hex_bytes_for_mw(test_string)
    num_bytes = len(test_string)
    test_addr_hex = "0x40"

    mw_cmd = f'mw -d {dev_path} -b {test_addr_hex} {hex_bytes_for_mw_cmd}'
    try:
        barebox.run_check(mw_cmd)
    except Exception as e:
        pytest.fail(f"mw command failed in test_nvmem_cdev_mw_md_bytes_string: {e}")

    md_cmd = f'md -s {dev_path} -b {test_addr_hex}+{num_bytes}'
    stdout_md = barebox.run_check(md_cmd)

    read_string = parse_md_bytes_to_string(stdout_md, num_bytes)

    assert read_string is not None, (
        f"Failed to parse byte string from md output: {stdout_md}"
    )
    assert read_string == test_string, \
        "Read byte string does not match written string."


def test_nvmem_cdev_mw_md_long_bytes_string_wrapped(barebox, barebox_config,
                                                    created_nvmem_device_name):
    """
    Tests writing a longer string (34 bytes) to an NVMEM device,
    which is likely to cause `md -b` output to wrap over multiple lines.
    Verifies that the parser correctly handles wrapped output.

    Manual Reproduction (assuming rmem0 is created):
    1. Create device: `nvmem -c 1k`
    2. Write 34-byte string (e.g., "ThisIsALongerTestStringForNVMEM012"):
       `mw -d /dev/rmem0 -b 0x80 0x54 0x68 ... 0x32` (full hex string)
    3. Read: `md -s /dev/rmem0 -b 0x80+34` (observe wrapped output and verify ASCII)
    """
    actual_dev_name = created_nvmem_device_name
    if not actual_dev_name:
        pytest.fail("Fixture created_nvmem_device_name returned empty")
    dev_path = f"/dev/{actual_dev_name}"

    test_string = "ThisIsALongerTestStringForNVMEM012"
    assert len(test_string) == 34

    hex_bytes_for_mw_cmd = string_to_hex_bytes_for_mw(test_string)
    num_bytes = len(test_string)
    test_addr_hex = "0x80"

    mw_cmd = f'mw -d {dev_path} -b {test_addr_hex} {hex_bytes_for_mw_cmd}'
    try:
        barebox.run_check(mw_cmd)
    except Exception as e:
        pytest.fail(f"mw command failed in test_nvmem_cdev_mw_md_long_bytes_string_wrapped: {e}")

    md_cmd = f'md -s {dev_path} -b {test_addr_hex}+{num_bytes}'
    stdout_md = barebox.run_check(md_cmd)

    read_string = parse_md_bytes_to_string(stdout_md, num_bytes)

    assert read_string is not None, (
        f"Failed to parse byte string from md output (wrapped test): {stdout_md}"
    )
    assert read_string == test_string, \
        "Read byte string (wrapped test) does not match written string."


# --- New tests for different parameters ---
def test_nvmem_cdev_mw_md_word(barebox, barebox_config, created_nvmem_device_name):
    """
    Tests writing a 16-bit word value to an NVMEM device using `mw -w`
    and reading it back using `md -w`.

    Manual Reproduction (assuming rmem0 is created):
    1. Create device: `nvmem -c 1k`
    2. Write: `mw -d /dev/rmem0 -w 0xB0 0xABCD`
    3. Read: `md -s /dev/rmem0 -w 0xB0+2` (verify output shows ABCD)
    """
    actual_dev_name = created_nvmem_device_name
    if not actual_dev_name:
        pytest.fail("Fixture created_nvmem_device_name returned empty")
    dev_path = f"/dev/{actual_dev_name}"

    test_val_hex = "0xABCD"
    test_addr_hex = "0xB0"

    mw_cmd = f'mw -d {dev_path} -w {test_addr_hex} {test_val_hex}'
    try:
        barebox.run_check(mw_cmd)
    except Exception as e:
        pytest.fail(f"mw -w command failed: {e}")

    md_cmd = f'md -s {dev_path} -w {test_addr_hex}+2'
    stdout_md = barebox.run_check(md_cmd)

    read_val_int = parse_md_word_value(stdout_md)

    assert read_val_int is not None, (
        f"Failed to parse word value from md output: {stdout_md}"
    )
    assert read_val_int == int(test_val_hex, 16), \
        "Read word value does not match written value."


def test_nvmem_cdev_mw_md_long_swapped(barebox, barebox_config, created_nvmem_device_name):
    """
    Tests writing a 32-bit long value with byte swapping (`mw -l -x`)
    and verifies the content by reading back with and without byte swap (`md -l` and `md -l -x`).

    Manual Reproduction (assuming rmem0 is created):
    1. Create device: `nvmem -c 1k`
    2. Write with swap: `mw -d /dev/rmem0 -l -x 0xC0 0x12345678`
    3. Read without swap: `md -s /dev/rmem0 -l 0xC0+4` (should show 78563412)
    4. Read with swap: `md -s /dev/rmem0 -l -x 0xC0+4` (should show 12345678)
    """
    actual_dev_name = created_nvmem_device_name
    if not actual_dev_name:
        pytest.fail("Fixture created_nvmem_device_name returned empty")
    dev_path = f"/dev/{actual_dev_name}"

    original_val_hex = "0x12345678"
    swapped_val_on_device_hex = "0x78563412"
    test_addr_hex = "0xC0"

    mw_cmd = f'mw -d {dev_path} -l -x {test_addr_hex} {original_val_hex}'
    try:
        barebox.run_check(mw_cmd)
    except Exception as e:
        pytest.fail(f"mw -l -x command failed: {e}")

    md_cmd_no_swap = f'md -s {dev_path} -l {test_addr_hex}+4'
    stdout_md_no_swap = barebox.run_check(md_cmd_no_swap)
    read_val_no_swap = parse_md_long_value(stdout_md_no_swap)
    assert read_val_no_swap is not None, \
        f"Failed to parse md (no swap) output: {stdout_md_no_swap}"
    assert read_val_no_swap == int(swapped_val_on_device_hex, 16), \
        "Reading without swap did not yield the byte-swapped value."

    md_cmd_with_swap = f'md -s {dev_path} -l -x {test_addr_hex}+4'
    stdout_md_with_swap = barebox.run_check(md_cmd_with_swap)
    read_val_with_swap = parse_md_long_value(stdout_md_with_swap)
    assert read_val_with_swap is not None, \
        f"Failed to parse md -x (with swap) output: {stdout_md_with_swap}"
    assert read_val_with_swap == int(original_val_hex, 16), \
        "Reading with swap did not yield the original value."


# == Protection Mechanism Tests ==
def test_nvmem_protect_then_write_fail(barebox, barebox_config,
                                       created_nvmem_device_name):
    """
    Tests NVMEM protection:
    1. Writes an initial value to a memory region.
    2. Protects that region.
    3. Attempts to write a new value to the protected region (should fail).
    4. Verifies that the region still contains the original value.

    Manual Reproduction (assuming rmem0 is created):
    1. Create device: `nvmem -c 1k`
    2. Write initial: `mw -d /dev/rmem0 -l 0x10 0x11223344`
    3. Protect: `protect /dev/rmem0 0x10+4`
    4. Attempt write (fail): `mw -d /dev/rmem0 -l 0x10 0xAABBCCDD` (observe error)
    5. Verify: `md -s /dev/rmem0 -l 0x10+4` (should still be 11223344)
    """
    actual_dev_name = created_nvmem_device_name
    if not actual_dev_name:
        pytest.fail("Fixture created_nvmem_device_name returned empty")
    dev_path = f"/dev/{actual_dev_name}"

    addr_hex = "0x10"
    length = 4
    val_before_hex = "0x11223344"
    val_attempt_hex = "0xAABBCCDD"
    try:
        barebox.run_check(
            f'mw -d {dev_path} -l {addr_hex} {val_before_hex}'
        )
    except Exception as e:
        pytest.fail(f"Initial mw command failed in test_nvmem_protect_then_write_fail: {e}")

    stdout_protect, _, ret_protect = barebox.run(
        f'protect {dev_path} {addr_hex}+{length}'
    )
    assert ret_protect == 0, "Protect command failed."

    stdout_mw, stderr_mw, returncode_mw = barebox.run(
        f'mw -d {dev_path} -l {addr_hex} {val_attempt_hex}'
    )
    assert returncode_mw != 0, "mw to protected area should fail."
    full_output_mw = "".join(stdout_mw) + "".join(stderr_mw)
    assert "overlaps with protected" in full_output_mw or \
           "Read-only file system" in full_output_mw, \
        "Expected error/warning for writing to protected area not found."

    md_cmd_after = f'md -s {dev_path} -l {addr_hex}+{length}'
    stdout_md_after = barebox.run_check(md_cmd_after)

    read_val_after_int = parse_md_long_value(stdout_md_after)

    if read_val_after_int is None:
        pytest.fail(
            f"Failed to parse 'md' output after failed write attempt. "
            f"Raw 'md' output was: {stdout_md_after!r}"
        )
    assert read_val_after_int == int(val_before_hex, 16), \
        "Value in protected area changed after failed write attempt."


def test_nvmem_unprotect_then_write_ok(barebox, barebox_config,
                                       created_nvmem_device_name):
    """
    Tests NVMEM unprotection:
    1. Writes an initial value and protects a region.
    2. Unprotects the region.
    3. Writes a new value to the (now unprotected) region (should succeed).
    4. Verifies the new value is present.

    Manual Reproduction (assuming rmem0 is created):
    1. Create device: `nvmem -c 1k`
    2. Write initial: `mw -d /dev/rmem0 -l 0x20 0xDEADBEEF`
    3. Protect: `protect /dev/rmem0 0x20+4`
    4. Unprotect: `unprotect /dev/rmem0 0x20 4`
    5. Write new: `mw -d /dev/rmem0 -l 0x20 0x12345678`
    6. Verify: `md -s /dev/rmem0 -l 0x20+4` (should be 12345678)
    """
    actual_dev_name = created_nvmem_device_name
    if not actual_dev_name:
        pytest.fail("Fixture created_nvmem_device_name returned empty")
    dev_path = f"/dev/{actual_dev_name}"

    addr_hex = "0x20"
    length = 4
    val_initial_hex = "0xDEADBEEF"
    val_after_unprotect_hex = "0x12345678"
    try:
        barebox.run_check(
            f'mw -d {dev_path} -l {addr_hex} {val_initial_hex}'
        )
        barebox.run_check(f'protect {dev_path} {addr_hex}+{length}')
    except Exception as e:
        pytest.fail(f"Setup mw or protect failed in test_nvmem_unprotect_then_write_ok: {e}")

    stdout_unprotect, _, ret_unprotect = barebox.run(
        f'unprotect {dev_path} {addr_hex} {length}'
    )
    assert ret_unprotect == 0, "Unprotect command failed."

    try:
        barebox.run_check(
            f'mw -d {dev_path} -l {addr_hex} {val_after_unprotect_hex}'
        )
    except Exception as e:
        pytest.fail(f"Final mw command failed in test_nvmem_unprotect_then_write_ok: {e}")

    stdout_md_final = barebox.run_check(
        f'md -s {dev_path} -l {addr_hex}+{length}'
    )
    read_val_final_int = parse_md_long_value(stdout_md_final)
    assert read_val_final_int == int(val_after_unprotect_hex, 16), \
        "Value after unprotect and write is incorrect."


def test_nvmem_protect_write_outside_range_ok(barebox, barebox_config,
                                              created_nvmem_device_name):
    """
    Tests that writing outside a protected range is successful,
    while the protected range itself remains inaccessible.

    Manual Reproduction (assuming rmem0 is created):
    1. Create device: `nvmem -c 1k`
    2. Write initial to protected area: `mw -d /dev/rmem0 -l 0x30 0x00000000`
    3. Protect: `protect /dev/rmem0 0x30+4`
    4. Write outside: `mw -d /dev/rmem0 -l 0x50 0xABCDEF01`
    5. Verify outside: `md -s /dev/rmem0 -l 0x50+4` (should be ABCDEF01)
    6. Attempt write to protected (fail): `mw -d /dev/rmem0 -l 0x30 0xFFFFFFFF`
    7. Verify protected unchanged: `md -s /dev/rmem0 -l 0x30+4` (should be 00000000)
    """
    actual_dev_name = created_nvmem_device_name
    if not actual_dev_name:
        pytest.fail(f"Fixture created_nvmem_device_name returned empty")
    dev_path = f"/dev/{actual_dev_name}"

    protected_addr_hex = "0x30"
    protected_len = 4
    initial_val_prot_hex = "0x00000000"
    write_addr_outside_hex = "0x50"
    val_to_write_outside_hex = "0xABCDEF01"
    try:
        barebox.run_check(
            f'mw -d {dev_path} -l {protected_addr_hex} '
            f'{initial_val_prot_hex}'
        )
        barebox.run_check(
            f'protect {dev_path} {protected_addr_hex}+{protected_len}'
        )
        barebox.run_check(
            f'mw -d {dev_path} -l {write_addr_outside_hex} '
            f'{val_to_write_outside_hex}'
        )
    except Exception as e:
        pytest.fail(f"Setup command failed in test_nvmem_protect_write_outside_range_ok: {e}")

    stdout_md_out = barebox.run_check(
        f'md -s {dev_path} -l {write_addr_outside_hex}+4'
    )
    read_val_out_int = parse_md_long_value(stdout_md_out)
    assert read_val_out_int == int(val_to_write_outside_hex, 16), \
        "Value written outside protected range is incorrect."
    _, _, ret_mw_prot = barebox.run(
        f'mw -d {dev_path} -l {protected_addr_hex} 0xFFFFFFFF'
    )
    assert ret_mw_prot != 0, "Write to protected area should have failed."
    stdout_md_prot = barebox.run_check(
        f'md -s {dev_path} -l {protected_addr_hex}+{protected_len}'
    )
    read_val_prot_int = parse_md_long_value(stdout_md_prot)
    assert read_val_prot_int == int(initial_val_prot_hex, 16), \
        "Protected area content changed or was not initially set."


def test_nvmem_write_span_unprotected_protected(barebox, barebox_config,
                                                created_nvmem_device_name):
    """
    Tests writing a sequence of bytes (`mw -b`) that spans from an unprotected
    area into a protected area. Expects the write to fail for bytes entering
    the protected region, but bytes in the unprotected part might be written.

    Manual Reproduction (assuming rmem0 is created):
    1. Create device: `nvmem -c 1k`
    2. Protect e.g. `0x10` to `0x13`: `protect /dev/rmem0 0x10+4`
    3. Attempt to write 4 bytes `AA BB CC DD` starting at `0x0E`:
       `mw -d /dev/rmem0 -b 0x0E 0xAA 0xBB 0xCC 0xDD`
       (Expect `AA` at `0x0E`, `BB` at `0x0F` to be written. `CC` at `0x10` fails.)
    4. Verify: `md -s /dev/rmem0 -b 0x0E+4` (should show `AA BB 00 00` if area was zeroed)
    """
    actual_dev_name = created_nvmem_device_name
    if not actual_dev_name:
        pytest.fail("Fixture created_nvmem_device_name returned empty")
    dev_path = f"/dev/{actual_dev_name}"

    prot_start_addr_val = 0x10
    prot_start_addr_hex = f"0x{prot_start_addr_val:x}"
    prot_len = 4
    try:
        barebox.run_check(
            f'protect {dev_path} {prot_start_addr_hex}+{prot_len}'
        )
    except Exception as e:
        pytest.fail(f"Protect command failed in test_nvmem_write_span_unprotected_protected: {e}")

    write_start_addr_val = prot_start_addr_val - 2
    write_start_addr_hex = f"0x{write_start_addr_val:x}"
    hex_vals_to_write = "0xAA 0xBB 0xCC 0xDD"
    write_span_len = 4
    stdout_mw, stderr_mw, returncode_mw = barebox.run(
        f'mw -d {dev_path} -b {write_start_addr_hex} '
        f'{hex_vals_to_write}'
    )
    assert returncode_mw != 0, "mw spanning protected boundary should fail."
    full_output_mw = "".join(stdout_mw) + "".join(stderr_mw)
    assert "overlaps with protected" in full_output_mw or \
           "Read-only file system" in full_output_mw, \
        "Expected error/warning for spanning write not found."
    stdout_md_verify = barebox.run_check(
        f'md -s {dev_path} -b {write_start_addr_hex}+{write_span_len}'
    )
    read_hex_verify = parse_md_output(stdout_md_verify, 'b')
    assert read_hex_verify == "aabb0000", \
        "Data in spanned region is not as expected after partial write."


def test_nvmem_write_at_start_and_end_exact(barebox, barebox_config,
                                            created_nvmem_device_name):
    """
    Tests writing exactly at the start (offset 0) and filling up to the
    exact end of the NVMEM device. Also tests that writing 1 byte past
    the end fails.

    Manual Reproduction (assuming rmem0 is created, size 1024 bytes):
    1. Create device: `nvmem -c 1024`
    2. Write at start: `mw -d /dev/rmem0 -l 0x0 0x1A2B3C4D`
    3. Verify: `md -s /dev/rmem0 -l 0x0+4`
    4. Write at end (last 4 bytes): `mw -d /dev/rmem0 -l 0x3FC 0x5E6F7A8B` (1020 = 0x3FC)
    5. Verify: `md -s /dev/rmem0 -l 0x3FC+4`
    6. Attempt write past end: `mw -d /dev/rmem0 -b 0x400 0xFF` (should fail)
    """
    actual_dev_name = created_nvmem_device_name
    if not actual_dev_name:
        pytest.fail("Fixture created_nvmem_device_name returned empty")
    dev_path = f"/dev/{actual_dev_name}"

    device_size = DEFAULT_DEVICE_SIZE
    val_start_hex = "0x1A2B3C4D"
    addr_start_hex = "0x0"
    try:
        barebox.run_check(
            f'mw -d {dev_path} -l {addr_start_hex} {val_start_hex}'
        )
    except Exception as e:
        pytest.fail(f"mw to start of device failed in: {e}")

    stdout_md_start = barebox.run_check(
        f'md -s {dev_path} -l {addr_start_hex}+4'
    )
    assert parse_md_long_value(stdout_md_start) == int(val_start_hex, 16), \
        "Value at device start is incorrect."
    val_end_hex = "0x5E6F7A8B"
    addr_end_val = device_size - 4
    addr_end_hex = f"0x{addr_end_val:x}"
    try:
        barebox.run_check(
            f'mw -d {dev_path} -l {addr_end_hex} {val_end_hex}'
        )
    except Exception as e:
        pytest.fail(f"mw to end of device failed: {e}")

    stdout_md_end = barebox.run_check(
        f'md -s {dev_path} -l {addr_end_hex}+4'
    )
    assert parse_md_long_value(stdout_md_end) == int(val_end_hex, 16), \
        "Value at device end is incorrect."
    addr_past_end_val = device_size
    addr_past_end_hex = f"0x{addr_past_end_val:x}"
    _, _, ret_mw_past = barebox.run(
        f'mw -d {dev_path} -b {addr_past_end_hex} 0xFF'
    )
    assert ret_mw_past != 0, "mw write past end should fail."


def test_nvmem_protect_merge_overlapping(barebox, barebox_config,
                                         created_nvmem_device_name):
    """
    Tests if two overlapping `protect` calls result in a single, correctly
    merged protected range. The log message from `rmem.c` should reflect
    the final merged range.

    Manual Reproduction (assuming rmem0 is created):
    1. Create device: `nvmem -c 1k`
    2. Protect [0x10, len 4]: `protect /dev/rmem0 0x10+4`
    3. Protect [0x12, len 4]: `protect /dev/rmem0 0x12+4`
       (observe log, should show merged range, e.g., [0x10, len 6])
    4. Attempt writes to 0x10 and 0x14 (should fail).
    """
    actual_dev_name = created_nvmem_device_name
    if not actual_dev_name:
        pytest.fail("Fixture created_nvmem_device_name returned empty")
    dev_path = f"/dev/{actual_dev_name}"

    addr1_hex, len1 = "0x10", 4
    stdout_p1, _, ret_p1 = barebox.run(
        f'protect {dev_path} {addr1_hex}+{len1}'
    )
    assert ret_p1 == 0, "First protect call failed."

    addr2_hex, len2 = "0x12", 4
    stdout_p2, _, ret_p2 = barebox.run(
        f'protect {dev_path} {addr2_hex}+{len2}'
    )
    assert ret_p2 == 0, "Second (overlapping) protect call failed."
    _, _, ret_mw_0x10 = barebox.run(f'mw -d {dev_path} -b 0x10 0xFF')
    assert ret_mw_0x10 != 0, "Write to start of merged range (0x10) fail."
    _, _, ret_mw_0x14 = barebox.run(f'mw -d {dev_path} -b 0x14 0xFF')
    assert ret_mw_0x14 != 0, "Write to middle of merged range (0x14) fail."
    barebox.run_check(f'mw -d {dev_path} -b 0x0F 0xAA')
    barebox.run_check(f'mw -d {dev_path} -b 0x16 0xBB')


def test_nvmem_mw_multiple_bytes_single_command(barebox, barebox_config,
                                                created_nvmem_device_name):
    """
    Tests `mw -b` with multiple hexadecimal byte values provided in a single command line,
    e.g., `mw -d /dev/rmem0 -b 0x90 0x4F 0x4B`.

    Manual Reproduction (assuming rmem0 is created):
    1. Create device: `nvmem -c 1k`
    2. Write "OK": `mw -d /dev/rmem0 -b 0x90 0x4F 0x4B`
    3. Verify: `md -s /dev/rmem0 -b 0x90+2` (should show "OK")
    """
    actual_dev_name = created_nvmem_device_name
    if not actual_dev_name:
        pytest.fail("Fixture created_nvmem_device_name returned empty")
    dev_path = f"/dev/{actual_dev_name}"

    hex_bytes_for_mw_cmd = "0x4F 0x4B"
    num_bytes = 2
    test_addr_hex = "0x90"
    try:
        barebox.run_check(
            f'mw -d {dev_path} -b {test_addr_hex} {hex_bytes_for_mw_cmd}'
        )
    except Exception as e:
        pytest.fail(f"mw command failed in test_nvmem_mw_multiple_bytes_single_command: {e}")

    stdout_md = barebox.run_check(
        f'md -s {dev_path} -b {test_addr_hex}+{num_bytes}'
    )
    read_hex_string = parse_md_output(stdout_md, 'b')
    expected_hex_string = "4f4b"
    assert read_hex_string == expected_hex_string, (
        f"Read hex string '{read_hex_string}' does not match expected "
        f"'{expected_hex_string}'."
    )


def test_nvmem_unprotect_noop_if_not_protected(barebox, barebox_config,
                                               created_nvmem_device_name):
    """
    Tests that calling `unprotect` on a device with no active protections
    is a no-op and succeeds. It should still log the standard "Unprotected range..."
    message for the whole device.

    Manual Reproduction (assuming rmem0 is created):
    1. Create device: `nvmem -c 1k` (ensure no protections are set on it)
    2. Run: `unprotect /dev/rmem0 0x0 1024`
       Expected: Command succeeds, prints "Unprotected range [0x0, len 1024]"
    """
    actual_dev_name = created_nvmem_device_name
    if not actual_dev_name:
        pytest.fail("Fixture created_nvmem_device_name returned empty")
    dev_path = f"/dev/{actual_dev_name}"

    device_size = DEFAULT_DEVICE_SIZE
    stdout_unprot, _, ret_unprot = barebox.run(
        f'unprotect {dev_path} 0x0 {device_size}'
    )
    assert ret_unprot == 0, "Unprotect should succeed if no ranges protected."
