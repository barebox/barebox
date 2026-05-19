# SPDX-License-Identifier: GPL-2.0-only

import hashlib
import pytest
from .helper import skip_disabled, screendump


@pytest.fixture(autouse=True)
def check_fbtest_in_qemu(barebox, env, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_FBTEST")

    if 'qemu' not in env.get_target_features():
        pytest.skip("fbtest tests only possible with QEMU")

    _, _, ret = barebox.run("test -e /dev/fb0")
    if ret != 0:
        pytest.skip("no framebuffer device available")


def assert_solid_color(data, width, height, color):
    """Verify that sampled pixels in the lower half match the expected color."""
    sample_points = [
        (width // 2, height * 3 // 4),
        (width // 4, height * 3 // 4),
        (width * 3 // 4, height * 3 // 4),
        (width // 2, height - 2),
    ]

    r_exp = (color >> 16) & 0xff
    g_exp = (color >> 8) & 0xff
    b_exp = color & 0xff

    for x, y in sample_points:
        off = (y * width + x) * 3
        r, g, b = data[off], data[off + 1], data[off + 2]
        assert abs(r - r_exp) < 10, f"pixel ({x},{y}): R={r}, expected {r_exp}"
        assert abs(g - g_exp) < 10, f"pixel ({x},{y}): G={g}, expected {g_exp}"
        assert abs(b - b_exp) < 10, f"pixel ({x},{y}): B={b}, expected {b_exp}"


def test_fb_solid_color(barebox, barebox_config, strategy):
    color = 0xff0000
    barebox.run_check(f"fbtest -p solid -c {color:06x}")

    width, height, data = screendump(strategy.qemu)
    assert_solid_color(data, width, height, color)


def screendump_hash(qemu):
    """Capture a screenshot and return a hash of the pixel data."""
    _, _, data = screendump(qemu)
    return hashlib.sha256(data).hexdigest()


def test_fb_patterns_distinct_and_stable(barebox, barebox_config, strategy):
    patterns = ["solid", "geometry", "bars", "gradient"]

    # Render each pattern twice and collect hashes
    hashes = {p: [] for p in patterns}

    for run in range(2):
        for pattern in patterns:
            barebox.run_check(f"fbtest -p {pattern} -c ffffff")
            hashes[pattern].append(screendump_hash(strategy.qemu))

    # Same pattern must produce the same output across runs
    for pattern in patterns:
        assert hashes[pattern][0] == hashes[pattern][1], \
            f"pattern '{pattern}' produced different output across runs"

    # Different patterns must produce different output
    unique = set(hashes[p][0] for p in patterns)
    assert len(unique) == len(patterns), \
        f"expected {len(patterns)} distinct patterns, got {len(unique)}"
