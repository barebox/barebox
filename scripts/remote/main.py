#!/usr/bin/env python2

from __future__ import absolute_import, division, print_function

import sys
import os
import argparse
import binascii
import logging
from Queue import Queue
from .ratp import RatpError

try:
    import serial
except:
    print("error: No python-serial package found", file=sys.stderr)
    exit(2)


def versiontuple(v):
    return tuple(map(int, (v.split("."))))

if versiontuple(serial.VERSION) < (2, 7):
    print("warning: python-serial package is buggy in RFC2217 mode,",
          "consider updating to at least 2.7", file=sys.stderr)

from .ratp import SerialRatpConnection
from .controller import Controller
from .threadstdio import ConsoleInput


def get_controller(args):
    port = serial.serial_for_url(args.port, args.baudrate)
    conn = SerialRatpConnection(port)

    while True:
        try:
            ctrl = Controller(conn)
            break
        except (RatpError):
            if args.wait == True:
                pass
            else:
                raise

    return ctrl


def handle_run(args):
    ctrl = get_controller(args)
    ctrl.export(args.export)
    res = ctrl.command(' '.join(args.arg))
    if res:
        res = 1
    ctrl.close()
    return res


def handle_ping(args):
    ctrl = get_controller(args)
    res = ctrl.ping()
    if res:
        res = 1
    ctrl.close()
    return res


def handle_getenv(args):
    ctrl = get_controller(args)
    value = ctrl.getenv(' '.join(args.arg))
    if not value:
        res = 1
    else:
        print(value)
        res = 0
    ctrl.close()
    return res


def handle_md(args):
    ctrl = get_controller(args)
    (res,data) = ctrl.md(args.path, args.address, args.size)
    if res == 0:
        print(binascii.hexlify(data))
    ctrl.close()
    return res


def handle_mw(args):
    ctrl = get_controller(args)
    data=args.data
    if ((len(data) % 2) != 0):
        data="0"+data
    (res,written) = ctrl.mw(args.path, args.address, binascii.unhexlify(data))
    if res == 0:
        print("%i bytes written" % written)
    ctrl.close()
    return res


def handle_reset(args):
    ctrl = get_controller(args)
    ctrl.reset(args.force)
    ctrl.close()
    return 0


def handle_listen(args):
    port = serial.serial_for_url(args.port, args.baudrate)
    conn = SerialRatpConnection(port)
    conn.listen()
    while True:
        conn.wait(None)
    conn.close()


def handle_console(args):
    queue = Queue()
    ctrl = get_controller(args)
    ctrl.export(args.export)
    ctrl.start(queue)
    ctrl.send_async_console('\r')
    cons = ConsoleInput(queue, exit='\x14')  # CTRL-T
    cons.start()
    try:
        while True:
            src, data = queue.get(block=True)
            if src == cons:
                if data is None:  # shutdown
                    cons.join()
                    break
                elif data == '\x10':  # CTRL-P
                    ctrl.send_async_ping()
                else:
                    ctrl.send_async_console(data)
            elif src == ctrl:
                if data is None:  # shutdown
                    sys.exit(1)
                    break
                else:
                    os.write(sys.stdout.fileno(), data)
        ctrl.stop()
        ctrl.close()
    finally:
        print()
        print("total retransmits=%i crc-errors=%i" % (
            ctrl.conn.total_retransmits,
            ctrl.conn.total_crc_errors))

# Support base 10 or base 16 numbers automatically
def auto_int(x):
    return int(x, 0)

VERBOSITY = {
    0: logging.WARN,
    1: logging.INFO,
    2: logging.DEBUG,
    }

parser = argparse.ArgumentParser(prog='bbremote')
parser.add_argument('-v', '--verbose', action='count', default=0)
parser.add_argument('--port', type=str, default=os.environ.get('BBREMOTE_PORT', None))
parser.add_argument('--baudrate', type=int, default=os.environ.get('BBREMOTE_BAUDRATE', 115200))
parser.add_argument('--export', type=str, default=os.environ.get('BBREMOTE_EXPORT', None))
parser.add_argument('-w', '--wait', action='count', default=0)
subparsers = parser.add_subparsers(help='sub-command help')

parser_run = subparsers.add_parser('run', help="run a barebox command")
parser_run.add_argument('arg', nargs='+', help="barebox command to run")
parser_run.set_defaults(func=handle_run)

parser_ping = subparsers.add_parser('ping', help="test connection")
parser_ping.set_defaults(func=handle_ping)

parser_getenv = subparsers.add_parser('getenv', help="get a barebox environment variable")
parser_getenv.add_argument('arg', nargs='+', help="variable name")
parser_getenv.set_defaults(func=handle_getenv)

parser_md = subparsers.add_parser('md', help="run md command")
parser_md.add_argument('path', help="path")
parser_md.add_argument('address', type=auto_int, help="address")
parser_md.add_argument('size', type=auto_int, help="size")
parser_md.set_defaults(func=handle_md)

parser_mw = subparsers.add_parser('mw', help="run mw command")
parser_mw.add_argument('path', help="path")
parser_mw.add_argument('address', type=auto_int, help="address")
parser_mw.add_argument('data', help="data")
parser_mw.set_defaults(func=handle_mw)

parser_reset = subparsers.add_parser('reset', help="run reset command")
parser_reset_force = parser_reset.add_mutually_exclusive_group(required=False)
parser_reset_force.add_argument('--force', dest='force', action='store_true')
parser_reset_force.add_argument('--no-force', dest='force', action='store_false')
parser_reset.set_defaults(func=handle_reset,force=False)

parser_listen = subparsers.add_parser('listen', help="listen for an incoming connection")
parser_listen.set_defaults(func=handle_listen)

parser_console = subparsers.add_parser('console', help="connect to the console")
parser_console.set_defaults(func=handle_console)

args = parser.parse_args()
logging.basicConfig(level=VERBOSITY[args.verbose],
                    format='%(levelname)-8s %(module)-8s %(funcName)-16s %(message)s')
try:
    res = args.func(args)
    exit(res)
except RatpError as detail:
    print("Ratp error:", detail, file=sys.stderr);
    exit(127)
except KeyboardInterrupt:
    print("\nInterrupted", file=sys.stderr);
    exit(1)
#try:
#    res = args.func(args)
#except Exception as e:
#    print("error: failed to establish connection: %s" % e, file=sys.stderr)
#    exit(2)
