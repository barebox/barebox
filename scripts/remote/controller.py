#!/usr/bin/env python2
# -*- coding: utf-8 -*-

from __future__ import absolute_import, division, print_function

import struct
import logging
import sys
import os
from threading import Thread
from Queue import Queue, Empty
from .ratpfs import RatpFSServer
from .messages import *
from .ratp import RatpError

try:
    from time import monotonic
except:
    from .missing import monotonic


def unpack(data):
    p_type, = struct.unpack("!H", data[:2])
    logging.debug("unpack: %r data=%r", p_type, repr(data))
    if p_type == BBType.command:
        logging.debug("received: command")
        return BBPacketCommand(raw=data)
    elif p_type == BBType.command_return:
        logging.debug("received: command_return")
        return BBPacketCommandReturn(raw=data)
    elif p_type == BBType.consolemsg:
        logging.debug("received: consolemsg")
        return BBPacketConsoleMsg(raw=data)
    elif p_type == BBType.ping:
        logging.debug("received: ping")
        return BBPacketPing(raw=data)
    elif p_type == BBType.pong:
        logging.debug("received: pong")
        return BBPacketPong(raw=data)
    elif p_type == BBType.getenv_return:
        logging.debug("received: getenv_return")
        return BBPacketGetenvReturn(raw=data)
    elif p_type == BBType.fs:
        logging.debug("received: fs")
        return BBPacketFS(raw=data)
    elif p_type == BBType.fs_return:
        logging.debug("received: fs_return")
        return BBPacketFSReturn(raw=data)
    elif p_type == BBType.md:
        logging.debug("received: md")
        return BBPacketMd(raw=data)
    elif p_type == BBType.md_return:
        logging.debug("received: md_return")
        return BBPacketMdReturn(raw=data)
    elif p_type == BBType.mw:
        logging.debug("received: mw")
        return BBPacketMw(raw=data)
    elif p_type == BBType.mw_return:
        logging.debug("received: mw_return")
        return BBPacketMwReturn(raw=data)
    elif p_type == BBType.reset:
        logging.debug("received: reset")
        return BBPacketReset(raw=data)
    elif p_type == BBType.i2c_read:
        logging.debug("received: i2c_read")
        return BBPacketI2cRead(raw=data)
    elif p_type == BBType.i2c_read_return:
        logging.debug("received: i2c_read_return")
        return BBPacketI2cReadReturn(raw=data)
    elif p_type == BBType.i2c_write:
        logging.debug("received: i2c_write")
        return BBPacketI2cWrite(raw=data)
    elif p_type == BBType.i2c_write_return:
        logging.debug("received: i2c_write_return")
        return BBPacketI2cWriteReturn(raw=data)
    elif p_type == BBType.gpio_get_value:
        logging.debug("received: gpio_get_value")
        return BBPacketGpioGetValue(raw=data)
    elif p_type == BBType.gpio_get_value_return:
        logging.debug("received: gpio_get_value_return")
        return BBPacketGpioGetValueReturn(raw=data)
    elif p_type == BBType.gpio_set_value:
        logging.debug("received: gpio_set_value")
        return BBPacketGpioSetValue(raw=data)
    elif p_type == BBType.gpio_set_value_return:
        logging.debug("received: gpio_set_value_return")
        return BBPacketGpioSetValueReturn(raw=data)
    elif p_type == BBType.gpio_set_direction:
        logging.debug("received: gpio_set_direction")
        return BBPacketGpioSetDirection(raw=data)
    elif p_type == BBType.gpio_set_direction_return:
        logging.debug("received: gpio_set_direction_return")
        return BBPacketGpioSetDirectionReturn(raw=data)
    else:
        logging.debug("received: UNKNOWN")
        return BBPacket(raw=data)


class Controller(Thread):
    def __init__(self, conn):
        Thread.__init__(self)
        self.daemon = True
        self.conn = conn
        self.fsserver = None
        self.rxq = None
        self.conn.connect(timeout=5.0)
        self._txq = Queue()
        self._stop = False
        self.fsserver = RatpFSServer()

    def _send(self, bbpkt):
        self.conn.send(bbpkt.pack())

    def _handle(self, bbpkt):
        if isinstance(bbpkt, BBPacketConsoleMsg):
            os.write(sys.stdout.fileno(), bbpkt.text)
        elif isinstance(bbpkt, BBPacketPong):
            print("pong",)
        elif isinstance(bbpkt, BBPacketFS):
            if self.fsserver != None:
                self._send(self.fsserver.handle(bbpkt))

    def _expect(self, bbtype, timeout=1.0):
        if timeout is not None:
            limit = monotonic()+timeout
        while timeout is None or limit > monotonic():
            pkt = self.conn.recv(0.1)
            if not pkt:
                continue
            bbpkt = unpack(pkt)
            if isinstance(bbpkt, bbtype):
                return bbpkt
            else:
                self._handle(bbpkt)

    def export(self, path):
        self.fsserver = RatpFSServer(path)

    def ping(self):
        self._send(BBPacketPing())
        r = self._expect(BBPacketPong)
        logging.info("Ping: %r", r)
        if not r:
            return 1
        else:
            print("pong")
            return 0

    def command(self, cmd):
        self._send(BBPacketCommand(cmd=cmd))
        r = self._expect(BBPacketCommandReturn, timeout=None)
        logging.info("Command: %r", r)
        return r.exit_code

    def getenv(self, varname):
        self._send(BBPacketGetenv(varname=varname))
        r = self._expect(BBPacketGetenvReturn)
        return r.text

    def md(self, path, addr, size):
        self._send(BBPacketMd(path=path, addr=addr, size=size))
        r = self._expect(BBPacketMdReturn)
        logging.info("Md return: %r", r)
        return (r.exit_code,r.data)

    def mw(self, path, addr, data):
        self._send(BBPacketMw(path=path, addr=addr, data=data))
        r = self._expect(BBPacketMwReturn)
        logging.info("Mw return: %r", r)
        return (r.exit_code,r.written)

    def i2c_read(self, bus, addr, reg, flags, size):
        self._send(BBPacketI2cRead(bus=bus, addr=addr, reg=reg, flags=flags, size=size))
        r = self._expect(BBPacketI2cReadReturn)
        logging.info("i2c read return: %r", r)
        return (r.exit_code,r.data)

    def i2c_write(self, bus, addr, reg, flags, data):
        self._send(BBPacketI2cWrite(bus=bus, addr=addr, reg=reg, flags=flags, data=data))
        r = self._expect(BBPacketI2cWriteReturn)
        logging.info("i2c write return: %r", r)
        return (r.exit_code,r.written)

    def gpio_get_value(self, gpio):
        self._send(BBPacketGpioGetValue(gpio=gpio))
        r = self._expect(BBPacketGpioGetValueReturn)
        logging.info("gpio get value return: %r", r)
        return r.value

    def gpio_set_value(self, gpio, value):
        self._send(BBPacketGpioSetValue(gpio=gpio, value=value))
        r = self._expect(BBPacketGpioSetValueReturn)
        logging.info("gpio set value return: %r", r)
        return 0

    def gpio_set_direction(self, gpio, direction, value):
        self._send(BBPacketGpioSetDirection(gpio=gpio, direction=direction, value=value))
        r = self._expect(BBPacketGpioSetDirectionReturn)
        logging.info("gpio set direction return: %r", r)
        return r.exit_code

    def reset(self, force):
        self._send(BBPacketReset(force=force))

    def close(self):
        self.conn.close()

    def run(self):
        assert self.rxq is not None
        try:
            while not self._stop:
                # receive
                pkt = self.conn.recv()
                if pkt:
                    bbpkt = unpack(pkt)
                    if isinstance(bbpkt, BBPacketConsoleMsg):
                        self.rxq.put((self, bbpkt.text))
                    else:
                        self._handle(bbpkt)
                # send
                try:
                    pkt = self._txq.get(block=False)
                except Empty:
                    pkt = None
                if pkt:
                    self._send(pkt)
        except RatpError as detail:
            print("Ratp error:", detail, file=sys.stderr);
            self.rxq.put((self, None))
            return

    def start(self, queue):
        assert self.rxq is None
        self.rxq = queue
        Thread.start(self)

    def stop(self):
        self._stop = True
        self.join()
        self._stop = False
        self.rxq = None

    def send_async(self, pkt):
        self._txq.put(pkt)

    def send_async_console(self, text):
        self._txq.put(BBPacketConsoleMsg(text=text))

    def send_async_ping(self):
        self._txq.put(BBPacketPing())


def main():
    import serial
    from .ratp import SerialRatpConnection
    url = "rfc2217://192.168.23.176:3002"
    port = serial.serial_for_url(url, 115200)
    conn = SerialRatpConnection(port)
    ctrl = Controller(conn)
    return ctrl

if __name__ == "__main__":
    C = main()
