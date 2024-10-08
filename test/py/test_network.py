import pytest

from labgrid import driver,Environment
import socket
import threading
import re
import warnings

# Setting the port to zero causes bind to choose a random ephermal port
TFTP_TEST_PORT = 0

def get_source_addr(destination_ip, destination_port):
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_socket.connect((destination_ip, destination_port))
    source_ip, _ = udp_socket.getsockname()
    return source_ip

def tftp_setup_socket(listen_addr, listen_port=0):
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_socket.bind((listen_addr, listen_port))
    udp_socket.settimeout(3)
    return udp_socket


def tftp_expect_notfound(filename, udp_socket):
    uname = filename.encode("ascii")
    messages = [
        b"\000\001" + uname + b"\000octet\000timeout\0005\000blksize\000512\000tsize\0000\000",
        b"\000\005\000\001File not found\000",
    ]
    data, addr = udp_socket.recvfrom(1024)
    udp_socket.sendto(messages[1], addr)
    udp_socket.close()
    assert data == messages[0]

def tftp_conversation(barebox, barebox_interface, guestaddr):

    listen_addr = "127.0.0.1"
    if not isinstance(barebox.console, driver.QEMUDriver):
        # sending an arbitrary udp package to determine the IP towards DUT
        listen_addr = get_source_addr(guestaddr, TFTP_TEST_PORT)
        barebox.run_check(f"eth0.serverip={listen_addr}")

    barebox.run_check("ping $eth0.serverip", timeout=2)

    tftp_socket=tftp_setup_socket(listen_addr, TFTP_TEST_PORT)
    tftp_port_used=tftp_socket.getsockname()[1]

    tftp_thread = threading.Thread(
        target=tftp_expect_notfound,
        name="tftp_expect_notfound",
        args=("a", tftp_socket),
    )
    tftp_thread.daemon = True
    tftp_thread.start()

    try:
        stdout, _, returncode = barebox.run(f"tftp -P {tftp_port_used} a", timeout=3)
        assert returncode != 0
    finally:
        # terminate a timed-out tftp
        barebox.console.sendcontrol("c")
        tftp_thread.join()
        barebox.run_check("ifdown eth0")


def test_barebox_network(barebox, env):
    # on DUTs without network feature, this is expected to fail
    # set xfail_strict=True to enforce specifying the network feature if available
    if not 'network' in env.get_target_features():
        pytest.xfail("network feature not specified")

    stdout = barebox.run_check("ifup -a")

    interfaces = []

    for line in stdout:
        # eth0: DHCP client bound to address 10.0.2.15
        m = re.search('(eth[0-9]+): DHCP client bound to address (.*)', line)
        if m is None:
            continue
        else:
            interfaces.append((m.group(1), m.group(2)))

    assert interfaces, "Network testing is only possible with at least one DHCP interface"

    for iface in interfaces:
        ifname = iface[0]
        ifaddr = iface[1]

        assert ifaddr != "0.0.0.0"
        assert ifaddr == barebox.run_check(f"echo ${ifname}.ipaddr")[0]

        # Attempt a conversation with the DUT, which needs to succeed only on one interface
        success = False

        try:
            tftp_conversation(barebox, ifname, ifaddr)
            success = True
            break
        except Exception as e:
            warnings.warn(f"Could not connect to DUT on {ifname} ({ifaddr}): {e}")

        if not success:
            pytest.fail("Could not converse with DUT on any of the found DHCP interfaces!")

