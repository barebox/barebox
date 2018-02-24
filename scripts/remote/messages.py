#!/usr/bin/env python2
# -*- coding: utf-8 -*-

from __future__ import absolute_import, division, print_function

import struct
import binascii


class BBType(object):
    command = 1
    command_return = 2
    consolemsg = 3
    ping = 4
    pong = 5
    getenv = 6
    getenv_return = 7
    fs = 8
    fs_return = 9
    md = 10
    md_return = 11
    mw = 12
    mw_return = 13
    reset = 14


class BBPacket(object):
    def __init__(self, p_type=0, p_flags=0, payload="", raw=None):
        self.p_type = p_type
        self.p_flags = p_flags
        if raw is not None:
            self.unpack(raw)
        else:
            self.payload = payload

    def __repr__(self):
        return "BBPacket(%i, %i)" % (self.p_type, self.p_flags)

    def _unpack_payload(self, data):
        self.payload = data

    def _pack_payload(self):
        return self.payload

    def unpack(self, data):
        self.p_type, self.p_flags = struct.unpack("!HH", data[:4])
        self._unpack_payload(data[4:])

    def pack(self):
        return struct.pack("!HH", self.p_type, self.p_flags) + \
            self._pack_payload()


class BBPacketCommand(BBPacket):
    def __init__(self, raw=None, cmd=None):
        self.cmd = cmd
        super(BBPacketCommand, self).__init__(BBType.command, raw=raw)

    def __repr__(self):
        return "BBPacketCommand(cmd=%r)" % self.cmd

    def _unpack_payload(self, payload):
        self.cmd = payload

    def _pack_payload(self):
        return self.cmd


class BBPacketCommandReturn(BBPacket):
    def __init__(self, raw=None, exit_code=None):
        self.exit_code = exit_code
        super(BBPacketCommandReturn, self).__init__(BBType.command_return,
                                                    raw=raw)

    def __repr__(self):
        return "BBPacketCommandReturn(exit_code=%i)" % self.exit_code

    def _unpack_payload(self, data):
        self.exit_code, = struct.unpack("!L", data[:4])

    def _pack_payload(self):
        return struct.pack("!L", self.exit_code)


class BBPacketConsoleMsg(BBPacket):
    def __init__(self, raw=None, text=None):
        self.text = text
        super(BBPacketConsoleMsg, self).__init__(BBType.consolemsg, raw=raw)

    def __repr__(self):
        return "BBPacketConsoleMsg(text=%r)" % self.text

    def _unpack_payload(self, payload):
        self.text = payload

    def _pack_payload(self):
        return self.text


class BBPacketPing(BBPacket):
    def __init__(self, raw=None):
        super(BBPacketPing, self).__init__(BBType.ping, raw=raw)

    def __repr__(self):
        return "BBPacketPing()"


class BBPacketPong(BBPacket):
    def __init__(self, raw=None):
        super(BBPacketPong, self).__init__(BBType.pong, raw=raw)

    def __repr__(self):
        return "BBPacketPong()"


class BBPacketGetenv(BBPacket):
    def __init__(self, raw=None, varname=None):
        self.varname = varname
        super(BBPacketGetenv, self).__init__(BBType.getenv, raw=raw)

    def __repr__(self):
        return "BBPacketGetenv(varname=%r)" % self.varname

    def _unpack_payload(self, payload):
        self.varname = payload

    def _pack_payload(self):
        return self.varname


class BBPacketGetenvReturn(BBPacket):
    def __init__(self, raw=None, text=None):
        self.text = text
        super(BBPacketGetenvReturn, self).__init__(BBType.getenv_return,
                                                   raw=raw)

    def __repr__(self):
        return "BBPacketGetenvReturn(varvalue=%s)" % self.text

    def _unpack_payload(self, payload):
        self.text = payload

    def _pack_payload(self):
        return self.text


class BBPacketFS(BBPacket):
    def __init__(self, raw=None, payload=None):
        super(BBPacketFS, self).__init__(BBType.fs, payload=payload, raw=raw)

    def __repr__(self):
        return "BBPacketFS(payload=%r)" % self.payload


class BBPacketFSReturn(BBPacket):
    def __init__(self, raw=None, payload=None):
        super(BBPacketFSReturn, self).__init__(BBType.fs_return, payload=payload, raw=raw)

    def __repr__(self):
        return "BBPacketFSReturn(payload=%r)" % self.payload


class BBPacketMd(BBPacket):
    def __init__(self, raw=None, path=None, addr=None, size=None):
        self.path = path
        self.addr = addr
        self.size = size
        super(BBPacketMd, self).__init__(BBType.md, raw=raw)

    def __repr__(self):
        return "BBPacketMd(path=%r,addr=0x%x,size=%u)" % (self.path, self.addr, self.size)

    def _unpack_payload(self, payload):
        buffer_offset, self.addr, self.size, path_size, path_offset = struct.unpack("!HHHHH", payload[:10])
        # header size is always 4 bytes (HH), so adjust the absolute data offset without the header size
        absolute_path_offset = buffer_offset + path_offset - 4
        self.path = payload[absolute_path_offset:absolute_path_offset+path_size]

    def _pack_payload(self):
        # header size is always 4 bytes (HH) and we have 10 bytes of fixed data (HHHHH), so buffer offset is 14
        return struct.pack("!HHHHH%ds" % len(self.path), 14, self.addr, self.size, len(self.path), 0, self.path)


class BBPacketMdReturn(BBPacket):
    def __init__(self, raw=None, exit_code=None, data=None):
        self.exit_code = exit_code
        self.data = data
        super(BBPacketMdReturn, self).__init__(BBType.md_return, raw=raw)

    def __repr__(self):
        return "BBPacketMdReturn(exit_code=%i, data=%s)" % (self.exit_code, binascii.hexlify(self.data))

    def _unpack_payload(self, payload):
        buffer_offset, self.exit_code, data_size, data_offset = struct.unpack("!HLHH", payload[:10])
        # header size is always 4 bytes (HH), so adjust the absolute data offset without the header size
        absolute_data_offset = buffer_offset + data_offset - 4
        self.data = payload[absolute_data_offset:absolute_data_offset + data_size]

    def _pack_payload(self):
        # header size is always 4 bytes (HH) and we have 10 bytes of fixed data (HLHH), so buffer offset is 14
        return struct.pack("!HLHH%ds" % len(self.data), 14, self.exit_code, len(self.data), 0, self.data)
        return self.text


class BBPacketMw(BBPacket):
    def __init__(self, raw=None, path=None, addr=None, data=None):
        self.path = path
        self.addr = addr
        self.data = data
        super(BBPacketMw, self).__init__(BBType.mw, raw=raw)

    def __repr__(self):
        return "BBPacketMw(path=%r,addr=0x%x,data=%r)" % (self.path, self.addr, self.data)

    def _unpack_payload(self, payload):
        buffer_offset, self.addr, path_size, path_offset, data_size, data_offset = struct.unpack("!HHHHHH", payload[:12])
        # header size is always 4 bytes (HH), so adjust the absolute data offset without the header size
        absolute_path_offset = buffer_offset + path_offset - 4
        self.path = payload[absolute_path_offset:absolute_path_offset+path_size]
        absolute_data_offset = buffer_offset + data_offset - 4
        self.data = payload[absolute_data_offset:absolute_data_offset+data_size]

    def _pack_payload(self):
        # header size is always 4 bytes (HH) and we have 12 bytes of fixed data (HHHHHH), so buffer offset is 16
        path_size = len(self.path)
        data_size = len(self.data)
        return struct.pack("!HHHHHH%ds%ds" % (path_size, path_size), 16, self.addr, path_size, 0, data_size, path_size, self.path, self.data)


class BBPacketMwReturn(BBPacket):
    def __init__(self, raw=None, exit_code=None, written=None):
        self.exit_code = exit_code
        self.written = written
        super(BBPacketMwReturn, self).__init__(BBType.mw_return, raw=raw)

    def __repr__(self):
        return "BBPacketMwReturn(exit_code=%i, written=%i)" % (self.exit_code, self.written)

    def _unpack_payload(self, payload):
        buffer_offset, self.exit_code, self.written = struct.unpack("!HLH", payload[:8])

    def _pack_payload(self):
        # header size is always 4 bytes (HH) and we have 8 bytes of fixed data (HLH), so buffer offset is 14
        return struct.pack("!HLH", 12, self.exit_code, self.written)


class BBPacketReset(BBPacket):
    def __init__(self, raw=None, force=None):
        self.force = force
        super(BBPacketReset, self).__init__(BBType.reset, raw=raw)

    def __repr__(self):
        return "BBPacketReset(force=%c)" % (self.force)

    def _unpack_payload(self, payload):
        self.force = struct.unpack("?", payload[:1])

    def _pack_payload(self):
        return struct.pack("?", self.force)
