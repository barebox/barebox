#!/usr/bin/env python2
# -*- coding: utf-8 -*-

from __future__ import absolute_import, division, print_function

import struct


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
