# SPDX-License-Identifier: GPL-2.0-only

from .helper import skip_disabled


def test_cmd_readlink(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_READLINK", "CONFIG_CMD_MKDIR")

    # Create test directory structure
    barebox.run_check('mkdir -p /tmp/readlink-test/subdir')
    barebox.run_check('cd /tmp/readlink-test')

    stdout = barebox.run_check('readlink -f .')
    assert stdout == ["/tmp/readlink-test"]

    # Test relative path resolution
    stdout = barebox.run_check('readlink -f ./subdir')
    assert stdout == ["/tmp/readlink-test/subdir"]

    # Test with variable storage
    stdout = barebox.run_check('readlink -f ./subdir mypath && echo $mypath')
    assert stdout == ["/tmp/readlink-test/subdir"]

    stdout = barebox.run_check('readlink -f ./subdir/something mypath && echo $mypath')
    assert stdout == ["/tmp/readlink-test/subdir/something"]

    # Test with variable storage (positional argument)
    stdout = barebox.run_check('readlink -f subdir mypath && echo $mypath')
    assert stdout == ["/tmp/readlink-test/subdir"]

    # Test with absolute path (should return as-is)
    stdout = barebox.run_check('readlink -f /tmp')
    assert stdout == ["/tmp"]

    # Cleanup
    barebox.run_check('rm -r /tmp/readlink-test')


def test_cmd_readlink_nonexistent(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_READLINK", "CONFIG_CMD_MKDIR")

    # Create test directory structure
    barebox.run_check('mkdir -p /tmp/readlink-test2/existing')
    barebox.run_check('cd /tmp/readlink-test2')

    # Test -f with non-existent final component (should work)
    stdout = barebox.run_check('readlink -f existing/nonexistent')
    assert stdout == ["/tmp/readlink-test2/existing/nonexistent"]

    # Test -f with nested non-existent final component
    stdout = barebox.run_check('readlink -f ./existing/also-nonexistent')
    assert stdout == ["/tmp/readlink-test2/existing/also-nonexistent"]

    # Test -e with non-existent final component (should fail)
    _, _, returncode = barebox.run('readlink -e existing/nonexistent')
    assert returncode != 0

    # Test -e with existing component (should work)
    stdout = barebox.run_check('readlink -e existing')
    assert stdout == ["/tmp/readlink-test2/existing"]

    # Test -f with non-existent parent directory (should fail)
    _, _, returncode = barebox.run('readlink -f nonexistent-parent/file')
    assert returncode != 0

    # Test -e with non-existent parent directory (should fail)
    _, _, returncode = barebox.run('readlink -e nonexistent-parent/file')
    assert returncode != 0

    # Cleanup
    barebox.run_check('rm -r /tmp/readlink-test2')


def test_cmd_readlink_symlinks(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_READLINK", "CONFIG_CMD_MKDIR", "CONFIG_CMD_LN", "CONFIG_CMD_ECHO")

    # Create test directory structure with symlinks
    barebox.run_check('mkdir -p /tmp/readlink-test3/real/deep')
    barebox.run_check('cd /tmp/readlink-test3')
    barebox.run_check('echo -o real/file.txt content')

    # Create various symlinks
    barebox.run_check('ln real link-to-real')
    barebox.run_check('ln real/file.txt link-to-file')
    barebox.run_check('ln link-to-real link-to-link')
    barebox.run_check('ln nonexistent broken-link')

    # Test basic readlink (no -f/-e, just read symlink value)
    stdout = barebox.run_check('readlink link-to-real')
    assert stdout == ["real"]

    stdout = barebox.run_check('readlink link-to-file')
    assert stdout == ["real/file.txt"]

    # Test -f following single symlink
    stdout = barebox.run_check('readlink -f link-to-real')
    assert stdout == ["/tmp/readlink-test3/real"]

    stdout = barebox.run_check('readlink -f link-to-file')
    assert stdout == ["/tmp/readlink-test3/real/file.txt"]

    # Test -f following multiple levels of symlinks
    stdout = barebox.run_check('readlink -f link-to-link')
    assert stdout == ["/tmp/readlink-test3/real"]

    # Test -e following symlinks (should work when target exists)
    stdout = barebox.run_check('readlink -e link-to-real')
    assert stdout == ["/tmp/readlink-test3/real"]

    stdout = barebox.run_check('readlink -e link-to-file')
    assert stdout == ["/tmp/readlink-test3/real/file.txt"]

    # Test -e with broken symlink (should fail)
    _, _, returncode = barebox.run('readlink -e broken-link')
    assert returncode != 0

    # Test -f with broken symlink (canonicalizes the link path itself)
    stdout = barebox.run_check('readlink -f broken-link')
    assert stdout == ["/tmp/readlink-test3/broken-link"]

    # Test -f with symlink to non-existent file in existing subdirectory
    barebox.run_check('ln real/nonexistent link-to-nonexistent')
    stdout = barebox.run_check('readlink -f link-to-nonexistent')
    assert stdout == ["/tmp/readlink-test3/link-to-nonexistent"]

    # Test -e with symlink to non-existent file (should fail)
    _, _, returncode = barebox.run('readlink -e link-to-nonexistent')
    assert returncode != 0

    # Cleanup
    barebox.run_check('rm -r /tmp/readlink-test3')


def test_cmd_readlink_parent_refs(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_READLINK", "CONFIG_CMD_MKDIR")

    # Create test directory structure
    barebox.run_check('mkdir -p /tmp/readlink-test4/a/b/c')
    barebox.run_check('cd /tmp/readlink-test4/a/b/c')

    # Test parent directory references with -f
    stdout = barebox.run_check('readlink -f ..')
    assert stdout == ["/tmp/readlink-test4/a/b"]

    stdout = barebox.run_check('readlink -f ../..')
    assert stdout == ["/tmp/readlink-test4/a"]

    stdout = barebox.run_check('readlink -f ../../..')
    assert stdout == ["/tmp/readlink-test4"]

    # Test parent with additional path components
    stdout = barebox.run_check('readlink -f ../../../a/b')
    assert stdout == ["/tmp/readlink-test4/a/b"]

    # Test -f with non-existent path using parent refs
    stdout = barebox.run_check('readlink -f ../../nonexistent')
    assert stdout == ["/tmp/readlink-test4/a/nonexistent"]

    # Test -e with parent references (existing path)
    stdout = barebox.run_check('readlink -e ..')
    assert stdout == ["/tmp/readlink-test4/a/b"]

    # Test -e with parent references (non-existent final component)
    _, _, returncode = barebox.run('readlink -e ../../nonexistent')
    assert returncode != 0

    # Cleanup
    barebox.run_check('rm -r /tmp/readlink-test4')


def test_cmd_readlink_edge_cases(barebox, barebox_config):
    skip_disabled(barebox_config, "CONFIG_CMD_READLINK", "CONFIG_CMD_MKDIR")

    # Create test directory structure
    barebox.run_check('mkdir -p /tmp/readlink-test5/dir')
    barebox.run_check('cd /tmp/readlink-test5')

    # Test with trailing slash on existing directory
    stdout = barebox.run_check('readlink -f dir/')
    assert stdout == ["/tmp/readlink-test5/dir"]

    # Test with multiple slashes
    stdout = barebox.run_check('readlink -f ./dir//subpath')
    assert stdout == ["/tmp/readlink-test5/dir/subpath"]

    # Test self-reference
    stdout = barebox.run_check('readlink -f ./././dir')
    assert stdout == ["/tmp/readlink-test5/dir"]

    # Test -e with trailing slash on existing directory
    stdout = barebox.run_check('readlink -e dir/')
    assert stdout == ["/tmp/readlink-test5/dir"]

    # Test absolute path that's already canonical
    stdout = barebox.run_check('readlink -f /tmp/readlink-test5/dir')
    assert stdout == ["/tmp/readlink-test5/dir"]

    stdout = barebox.run_check('readlink -e /tmp/readlink-test5/dir')
    assert stdout == ["/tmp/readlink-test5/dir"]

    # Cleanup
    barebox.run_check('rm -r /tmp/readlink-test5')
