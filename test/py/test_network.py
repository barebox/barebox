import pytest

from labgrid import driver,Environment
import socket
import threading

TFTP_TEST_PORT = 6968

def get_source_addr(destination_ip, destination_port):
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_socket.connect((destination_ip, destination_port))
    source_ip, _ = udp_socket.getsockname()
    return source_ip

def expect_tftp_notfound(filename, listen_port, listen_addr):
    uname = filename.encode("ascii")
    messages = [
        b"\000\001" + uname + b"\000octet\000timeout\0005\000blksize\000512\000tsize\0000\000",
        b"\000\005\000\001File not found\000",
    ]
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_socket.bind((listen_addr, listen_port))
    udp_socket.settimeout(3)
    data, addr = udp_socket.recvfrom(1024)
    udp_socket.sendto(messages[1], addr)
    udp_socket.close()
    assert data == messages[0]

def test_barebox_network(barebox, env):
    # on duts without network feature, this is expected to fail
    # set xfail_strict=True to enforce specifying the network feature if available
    if not 'network' in env.get_target_features():
        pytest.xfail("network feature not specified")

    barebox.run_check("ifup eth0")
    guestaddr = barebox.run_check("echo $eth0.ipaddr")[0]
    assert guestaddr != "0.0.0.0"

    listen_addr = "127.0.0.1"
    if not isinstance(barebox.console, driver.QEMUDriver):
        # sending an arbitrary udp package to determine the IP towards dut
        listen_addr = get_source_addr(guestaddr, TFTP_TEST_PORT)
        barebox.run_check(f"eth0.serverip={listen_addr}")

    barebox.run_check("ping $eth0.serverip", timeout=2)

    tftp_thread = threading.Thread(
        target=expect_tftp_notfound,
        name="expect_tftp_notfound",
        args=("a", TFTP_TEST_PORT, listen_addr),
    )
    tftp_thread.daemon = True
    tftp_thread.start()
    
    try:
        stdout, _, returncode = barebox.run(f"tftp -P {TFTP_TEST_PORT} a", timeout=3)
        assert returncode == 127
    except:
        raise
    finally:
        # terminate a timed-out ftpf
        barebox.console.sendcontrol("c")

    tftp_thread.join()
    barebox.run_check("ifdown eth0")

