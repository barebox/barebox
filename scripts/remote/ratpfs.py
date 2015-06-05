#!/usr/bin/env python2
# -*- coding: utf-8 -*-

from __future__ import absolute_import, division, print_function

import logging
import os
import stat
import struct
from enum import IntEnum

from .messages import BBPacketFS, BBPacketFSReturn

class RatpFSType(IntEnum):
    invalid = 0
    mount_call = 1
    mount_return = 2
    readdir_call = 3
    readdir_return = 4
    stat_call = 5
    stat_return = 6
    open_call = 7
    open_return = 8
    read_call = 9
    read_return = 10
    write_call = 11
    write_return = 12
    close_call = 13
    close_return = 14
    truncate_call = 15
    truncate_return = 16


class RatpFSError(ValueError):
    pass


class RatpFSPacket(object):
    def __init__(self, type=RatpFSType.invalid, payload="", raw=None):
        if raw is not None:
            type, = struct.unpack('!B', raw[:1])
            self.type = RatpFSType(type)
            self.payload = raw[1:]
        else:
            self.type = type
            self.payload = payload

    def __repr__(self):
        s = "%s(" % self.__class__.__name__
        s += "TYPE=%i," % self.type
        s += "PAYLOAD=%s)" % repr(self.payload)
        return s

    def pack(self):
        return struct.pack('!B', int(self.type))+self.payload


class RatpFSServer(object):
    def __init__(self, path=None):
        self.path = path
        if path:
            self.path = os.path.abspath(os.path.expanduser(path))
        self.next_handle = 1  # 0 is invalid
        self.files = {}
        self.mounted = False
        logging.info("exporting: %s", self.path)

    def _alloc_handle(self):
        handle = self.next_handle
        self.next_handle += 1
        return handle

    def _resolve(self, path):
        components = path.split('/')
        components = [x for x in components if x and x != '..']
        return os.path.join(self.path, *components)

    def handle_stat(self, path):

        try:
            logging.info("path: %r", path)
            path = self._resolve(path)
            logging.info("path1: %r", path)
            s = os.stat(path)
        except OSError as e:
            return struct.pack('!BI', 0, e.errno)
        if stat.S_ISREG(s.st_mode):
            return struct.pack('!BI', 1, s.st_size)
        elif stat.S_ISDIR(s.st_mode):
            return struct.pack('!BI', 2, s.st_size)
        else:
            return struct.pack('!BI', 0, 0)

    def handle_open(self, params):
        flags, = struct.unpack('!I', params[:4])
        flags = flags & (os.O_RDONLY | os.O_WRONLY | os.O_RDWR | os.O_CREAT |
                         os.O_TRUNC)
        path = params[4:]
        try:
            f = os.open(self._resolve(path), flags, 0666)
        except OSError as e:
            return struct.pack('!II', 0, e.errno)
        h = self._alloc_handle()
        self.files[h] = f
        size = os.lseek(f, 0, os.SEEK_END)
        return struct.pack('!II', h, size)

    def handle_read(self, params):
        h, pos, size = struct.unpack('!III', params)
        f = self.files[h]
        os.lseek(f, pos, os.SEEK_SET)
        size = min(size, 4096)
        return os.read(f, size)

    def handle_write(self, params):
        h, pos = struct.unpack('!II', params[:8])
        payload = params[8:]
        f = self.files[h]
        pos = os.lseek(f, pos, os.SEEK_SET)
        assert os.write(f, payload) == len(payload)
        return ""

    def handle_readdir(self, path):
        res = ""
        for x in os.listdir(self._resolve(path)):
            res += x+'\0'
        return res

    def handle_close(self, params):
        h, = struct.unpack('!I', params[:4])
        os.close(self.files.pop(h))
        return ""

    def handle_truncate(self, params):
        h, size = struct.unpack('!II', params)
        f = self.files[h]
        os.ftruncate(f, size)
        return ""

    def handle(self, bbcall):
        assert isinstance(bbcall, BBPacketFS)
        logging.debug("bb-call: %s", bbcall)
        fscall = RatpFSPacket(raw=bbcall.payload)
        logging.info("fs-call: %s", fscall)

        if not self.path:
            logging.warning("no filesystem exported")
            fsreturn = RatpFSPacket(type=RatpFSType.invalid)
        elif fscall.type == RatpFSType.mount_call:
            self.mounted = True
            fsreturn = RatpFSPacket(type=RatpFSType.mount_return)
        elif not self.mounted:
            logging.warning("filesystem not mounted")
            fsreturn = RatpFSPacket(type=RatpFSType.invalid)
        elif fscall.type == RatpFSType.readdir_call:
            payload = self.handle_readdir(fscall.payload)
            fsreturn = RatpFSPacket(type=RatpFSType.readdir_return,
                                    payload=payload)
        elif fscall.type == RatpFSType.stat_call:
            payload = self.handle_stat(fscall.payload)
            fsreturn = RatpFSPacket(type=RatpFSType.stat_return,
                                    payload=payload)
        elif fscall.type == RatpFSType.open_call:
            payload = self.handle_open(fscall.payload)
            fsreturn = RatpFSPacket(type=RatpFSType.open_return,
                                    payload=payload)
        elif fscall.type == RatpFSType.read_call:
            payload = self.handle_read(fscall.payload)
            fsreturn = RatpFSPacket(type=RatpFSType.read_return,
                                    payload=payload)
        elif fscall.type == RatpFSType.write_call:
            payload = self.handle_write(fscall.payload)
            fsreturn = RatpFSPacket(type=RatpFSType.write_return,
                                    payload=payload)
        elif fscall.type == RatpFSType.close_call:
            payload = self.handle_close(fscall.payload)
            fsreturn = RatpFSPacket(type=RatpFSType.close_return,
                                    payload=payload)
        elif fscall.type == RatpFSType.truncate_call:
            payload = self.handle_truncate(fscall.payload)
            fsreturn = RatpFSPacket(type=RatpFSType.truncate_return,
                                    payload=payload)
        else:
            raise RatpFSError()

        logging.info("fs-return: %s", fsreturn)
        bbreturn = BBPacketFSReturn(payload=fsreturn.pack())
        logging.debug("bb-return: %s", bbreturn)
        return bbreturn
