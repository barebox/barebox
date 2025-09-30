# SPDX-License-Identifier: GPL-2.0-or-later

import pytest


def test_security_policies(barebox, env):
    if 'policies' not in env.get_target_features():
        pytest.skip('policies feature flag missing')

    assert 'Active Policy: devel' in barebox.run_check('sconfig')

    assert set(barebox.run_check('sconfig -l')) == \
           set(['devel', 'factory', 'lockdown', 'tamper'])

    assert barebox.run_check('varinfo global.bootm.verify') == \
        ['bootm.verify: available (type: enum) '
         '(values: "none", "hash", "signature", "available")']

    barebox.run_check('sconfig -s factory')
    assert 'Active Policy: factory' in barebox.run_check('sconfig')

    stdout = barebox.run_check('sconfig -v -s devel')
    assert set(['+SCONFIG_BOOT_UNSIGNED_IMAGES',
                '+SCONFIG_CMD_GO']) <= set(stdout)
    assert 'Active Policy: devel' in barebox.run_check('sconfig')

    stdout, _, rc = barebox.run('go')
    assert 'go - start application at address or file' in stdout
    assert 'go: Operation not permitted' not in stdout
    assert rc == 1

    stdout = barebox.run_check("""
    sconfig -v -s tamper; echo "POLICY=${security.policy}";
    sconfig +SCONFIG_CONSOLE_INPUT +SCONFIG_SHELL
    """)
    assert set(['-SCONFIG_BOOT_UNSIGNED_IMAGES',
                '-SCONFIG_RATP',
                '-SCONFIG_CMD_GO',
                'POLICY=tamper']) <= set(stdout)
    assert 'Active Policy: debug0' in barebox.run_check('sconfig')

    stdout, _, rc = barebox.run('go')
    assert 'go - start application at address or file' not in stdout
    assert 'go: Operation not permitted' in stdout
    assert rc == 127

    assert barebox.run_check('varinfo global.bootm.verify') == \
        ['bootm.verify: signature (type: enum)']
