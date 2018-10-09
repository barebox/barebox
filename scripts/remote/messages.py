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
    i2c_read = 15
    i2c_read_return = 16
    i2c_write = 17
    i2c_write_return = 18
    gpio_get_value = 19
    gpio_get_value_return = 20
    gpio_set_value = 21
    gpio_set_value_return = 22
    gpio_set_direction = 23
    gpio_set_direction_return = 24


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


class BBPacketI2cRead(BBPacket):
    def __init__(self, raw=None, bus=None, addr=None, reg=None, flags=None, size=None):
        self.bus = bus
        self.addr = addr
        self.reg = reg
        self.flags = flags
        self.size = size
        super(BBPacketI2cRead, self).__init__(BBType.i2c_read, raw=raw)

    def __repr__(self):
        return "BBPacketI2cRead(bus=0x%x,addr=0x%x,reg=0x%x,flags=0x%x,size=%u)" % (self.bus, self.addr, self.reg, self.flags, self.size)

    def _unpack_payload(self, payload):
        buffer_offset, self.bus, self.addr, self.reg, self.flags, self.size = struct.unpack("!HBBHBH", payload[:9])

    def _pack_payload(self):
        # header size is always 4 bytes (HH) and we have 9 bytes of fixed data (HBBHBH), so buffer offset is 13
        return struct.pack("!HBBHBH", 13, self.bus, self.addr, self.reg, self.flags, self.size)


class BBPacketI2cReadReturn(BBPacket):
    def __init__(self, raw=None, exit_code=None, data=None):
        self.exit_code = exit_code
        self.data = data
        super(BBPacketI2cReadReturn, self).__init__(BBType.i2c_read_return, raw=raw)

    def __repr__(self):
        return "BBPacketI2cReadReturn(exit_code=%i, data=%s)" % (self.exit_code, binascii.hexlify(self.data))

    def _unpack_payload(self, payload):
        buffer_offset, self.exit_code, data_size, data_offset = struct.unpack("!HLHH", payload[:10])
        # header size is always 4 bytes (HH), so adjust the absolute data offset without the header size
        absolute_data_offset = buffer_offset + data_offset - 4
        self.data = payload[absolute_data_offset:absolute_data_offset + data_size]

    def _pack_payload(self):
        # header size is always 4 bytes (HH) and we have 10 bytes of fixed data (HLHH), so buffer offset is 14
        return struct.pack("!HLHH%ds" % len(self.data), 14, self.exit_code, len(self.data), 0, self.data)
        return self.text


class BBPacketI2cWrite(BBPacket):
    def __init__(self, raw=None, bus=None, addr=None, reg=None, flags=None, data=None):
        self.bus = bus
        self.addr = addr
        self.reg = reg
        self.flags = flags
        self.data = data
        super(BBPacketI2cWrite, self).__init__(BBType.i2c_write, raw=raw)

    def __repr__(self):
        return "BBPacketI2cWrite(bus=0x%x,addr=0x%x,reg=0x%x,flags=0x%x,data=%r)" % (self.bus, self.addr, self.reg, self.flags, self.data)

    def _unpack_payload(self, payload):
        buffer_offset, self.bus, self.addr, self.reg, self.flags, data_size, data_offset = struct.unpack("!HBBHBHH", payload[:11])
        # header size is always 4 bytes (HH), so adjust the absolute data offset without the header size
        absolute_data_offset = buffer_offset + data_offset - 4
        self.data = payload[absolute_data_offset:absolute_data_offset+data_size]

    def _pack_payload(self):
        # header size is always 4 bytes (HH) and we have 11 bytes of fixed data (HBBHBHH), so buffer offset is 15
        data_size = len(self.data)
        return struct.pack("!HBBHBHH%ds" % data_size, 15, self.bus, self.addr, self.reg, self.flags, data_size, 0, self.data)


class BBPacketI2cWriteReturn(BBPacket):
    def __init__(self, raw=None, exit_code=None, written=None):
        self.exit_code = exit_code
        self.written = written
        super(BBPacketI2cWriteReturn, self).__init__(BBType.i2c_write_return, raw=raw)

    def __repr__(self):
        return "BBPacketI2cWriteReturn(exit_code=%i, written=%i)" % (self.exit_code, self.written)

    def _unpack_payload(self, payload):
        buffer_offset, self.exit_code, self.written = struct.unpack("!HLH", payload[:8])

    def _pack_payload(self):
        # header size is always 4 bytes (HH) and we have 8 bytes of fixed data (HLH), so buffer offset is 14
        return struct.pack("!HLH", 12, self.exit_code, self.written)


class BBPacketGpioGetValue(BBPacket):
    def __init__(self, raw=None, gpio=None):
        self.gpio = gpio
        super(BBPacketGpioGetValue, self).__init__(BBType.gpio_get_value, raw=raw)

    def __repr__(self):
        return "BBPacketGpioGetValue(gpio=%u)" % (self.gpio)

    def _unpack_payload(self, payload):
        self.gpio = struct.unpack("!L", payload[:4])

    def _pack_payload(self):
        return struct.pack("!L", self.gpio)


class BBPacketGpioGetValueReturn(BBPacket):
    def __init__(self, raw=None, value=None):
        self.value = value
        super(BBPacketGpioGetValueReturn, self).__init__(BBType.gpio_get_value_return, raw=raw)

    def __repr__(self):
        return "BBPacketGpioGetValueReturn(value=%u)" % (self.value)

    def _unpack_payload(self, payload):
        self.value = struct.unpack("!B", payload[:1])

    def _pack_payload(self):
        return struct.pack("!B", self.value)


class BBPacketGpioSetValue(BBPacket):
    def __init__(self, raw=None, gpio=None, value=None):
        self.gpio = gpio
        self.value = value
        super(BBPacketGpioSetValue, self).__init__(BBType.gpio_set_value, raw=raw)

    def __repr__(self):
        return "BBPacketGpioSetValue(gpio=%u,value=%u)" % (self.gpio, self.value)

    def _unpack_payload(self, payload):
        self.gpio = struct.unpack("!LB", payload[:5])

    def _pack_payload(self):
        return struct.pack("!LB", self.gpio, self.value)


class BBPacketGpioSetValueReturn(BBPacket):
    def __init__(self, raw=None, value=None):
        self.value = value
        super(BBPacketGpioSetValueReturn, self).__init__(BBType.gpio_set_value_return, raw=raw)

    def __repr__(self):
        return "BBPacketGpioSetValueReturn()"


class BBPacketGpioSetDirection(BBPacket):
    def __init__(self, raw=None, gpio=None, direction=None, value=None):
        self.gpio = gpio
        self.direction = direction
        self.value = value
        super(BBPacketGpioSetDirection, self).__init__(BBType.gpio_set_direction, raw=raw)

    def __repr__(self):
        return "BBPacketGpioSetDirection(gpio=%u,direction=%u,value=%u)" % (self.gpio, self.direction, self.value)

    def _unpack_payload(self, payload):
        self.gpio = struct.unpack("!LBB", payload[:6])

    def _pack_payload(self):
        return struct.pack("!LBB", self.gpio, self.direction, self.value)


class BBPacketGpioSetDirectionReturn(BBPacket):
    def __init__(self, raw=None, exit_code=None):
        self.exit_code = exit_code
        super(BBPacketGpioSetDirectionReturn, self).__init__(BBType.gpio_set_direction_return, raw=raw)

    def __repr__(self):
        return "BBPacketGpioSetDirectionReturn(exit_code=%u)" % (self.exit_code)

    def _unpack_payload(self, payload):
        self.exit_code = struct.unpack("!L", payload[:4])

    def _pack_payload(self):
        return struct.pack("!L", self.exit_code)
