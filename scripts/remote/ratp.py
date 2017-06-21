#!/usr/bin/env python2
# -*- coding: utf-8 -*-

from __future__ import absolute_import, division, print_function

import crcmod
import logging
import struct
from enum import Enum
from time import sleep

try:
    from time import monotonic
except:
    from .missing import monotonic

csum_func = crcmod.predefined.mkCrcFun('xmodem')


class RatpState(Enum):
    listen = "listen"  # 1
    syn_sent = "syn-sent"  # 2
    syn_received = "syn-received"  # 3
    established = "established"  # 4
    fin_wait = "fin-wait"  # 5
    last_ack = "last-ack"  # 6
    closing = "closing"  # 7
    time_wait = "time-wait"  # 8
    closed = "closed"  # 9


class RatpInvalidHeader(ValueError):
    pass


class RatpInvalidPayload(ValueError):
    pass


class RatpError(ValueError):
    pass


class RatpPacket(object):

    def __init__(self, data=None, flags=''):
        self.payload = None
        self.synch = 0x01
        self._control = 0
        self.length = 0
        self.csum = 0
        self.c_syn = False
        self.c_ack = False
        self.c_fin = False
        self.c_rst = False
        self.c_sn = 0
        self.c_an = 0
        self.c_eor = False
        self.c_so = False
        if data:
            (self.synch, self._control, self.length, self.csum) = \
                struct.unpack('!BBBB', data)
            if self.synch != 0x01:
                raise RatpInvalidHeader("invalid synch octet (%x != %x)" %
                                        (self.synch, 0x01))
            csum = (self._control + self.length + self.csum) & 0xff
            if csum != 0xff:
                raise RatpInvalidHeader("invalid csum octet (%x != %x)" %
                                        (csum, 0xff))
            self._unpack_control()
        elif flags:
            if 'S' in flags:
                self.c_syn = True
            if 'A' in flags:
                self.c_ack = True
            if 'F' in flags:
                self.c_fin = True
            if 'R' in flags:
                self.c_rst = True
            if 'E' in flags:
                self.c_eor = True

    def __repr__(self):
        s = "RatpPacket("
        if self.c_syn:
            s += "SYN,"
        if self.c_ack:
            s += "ACK,"
        if self.c_fin:
            s += "FIN,"
        if self.c_rst:
            s += "RST,"
        s += "SN=%i,AN=%i," % (self.c_sn, self.c_an)
        if self.c_eor:
            s += "EOR,"
        if self.c_so:
            s += "SO,DATA=%i)" % self.length
        else:
            s += "DATA=%i)" % self.length
        return s

    def _pack_control(self):
        self._control = 0 | \
            self.c_syn << 7 | \
            self.c_ack << 6 | \
            self.c_fin << 5 | \
            self.c_rst << 4 | \
            self.c_sn << 3 | \
            self.c_an << 2 | \
            self.c_eor << 1 | \
            self.c_so << 0

    def _unpack_control(self):
        self.c_syn = bool(self._control & 1 << 7)
        self.c_ack = bool(self._control & 1 << 6)
        self.c_fin = bool(self._control & 1 << 5)
        self.c_rst = bool(self._control & 1 << 4)
        self.c_sn = bool(self._control & 1 << 3)
        self.c_an = bool(self._control & 1 << 2)
        self.c_eor = bool(self._control & 1 << 1)
        self.c_so = bool(self._control & 1 << 0)

    def pack(self):
        self._pack_control()
        self.csum = 0
        self.csum = (self._control + self.length + self.csum)
        self.csum = (self.csum & 0xff) ^ 0xff
        return struct.pack('!BBBB', self.synch, self._control, self.length,
                           self.csum)

    def unpack_payload(self, payload):
        (c_recv,) = struct.unpack('!H', payload[-2:])
        c_calc = csum_func(payload[:-2])
        if c_recv != c_calc:
            raise RatpInvalidPayload("bad checksum (%04x != %04x)" %
                                     (c_recv, c_calc))
        self.payload = payload[:-2]

    def pack_payload(self):
        c_calc = csum_func(self.payload)
        return self.payload+struct.pack('!H', c_calc)


class RatpConnection(object):
    def __init__(self):
        self._state = RatpState.closed
        self._passive = True
        self._input = b''
        self._s_sn = 0
        self._r_sn = 0
        self._retrans = None
        self._retrans_counter = None
        self._retrans_deadline = None
        self._r_mdl = None
        self._s_mdl = 0xff
        self._rx_buf = [] # reassembly buffer
        self._rx_queue = []
        self._tx_queue = []
        self._rtt_alpha = 0.8
        self._rtt_beta = 2.0
        self._srtt = 0.2
        self._rto_min, self._rto_max = 0.2, 1
        self._tx_timestamp = None
        self.total_retransmits = 0
        self.total_crc_errors = 0

    def _update_srtt(self, rtt):
        self._srtt = (self._rtt_alpha * self._srtt) + \
                     ((1.0 - self._rtt_alpha) * rtt)
        logging.info("SRTT: %r", self._srtt)

    def _get_rto(self):
        return min(self._rto_max,
                   max(self._rto_min, self._rtt_beta * self._srtt))

    def _write(self, pkt):

        if pkt.payload or pkt.c_so or pkt.c_syn or pkt.c_rst or pkt.c_fin:
            self._s_sn = pkt.c_sn
            if not self._retrans:
                self._retrans = pkt
                self._retrans_counter = 0
            else:
                self.total_retransmits += 1
                self._retrans_counter += 1
                if self._retrans_counter > 10:
                    raise RatpError("Maximum retransmit count exceeded")
            self._retrans_deadline = monotonic()+self._get_rto()

        logging.info("Write: %r", pkt)

        self._write_raw(pkt.pack())
        if pkt.payload:
            self._write_raw(pkt.pack_payload())
        self._tx_timestamp = monotonic()

    def _check_rto(self):
        if self._retrans is None:
            return

        if self._retrans_deadline < monotonic():
            logging.debug("Retransmit...")
            self._write(self._retrans)

    def _check_time_wait(self):
        if not self._state == RatpState.time_wait:
            return

        remaining = self._time_wait_deadline - monotonic()
        if remaining < 0:
            self._state = RatpState.closed
        else:
            logging.debug("Time-Wait: %.2f remaining" % remaining)
            sleep(min(remaining, 0.1))

    def _read(self):
        if len(self._input) < 4:
            self._input += self._read_raw(4-len(self._input))
        if len(self._input) < 4:
            return

        try:
            pkt = RatpPacket(data=self._input[:4])
        except RatpInvalidHeader as e:
            logging.info("%r", e)
            self._input = self._input[1:]
            return

        self._input = self._input[4:]

        logging.info("Read: %r", pkt)

        if pkt.c_syn or pkt.c_rst or pkt.c_so or pkt.c_fin:
            return pkt

        if pkt.length == 0:
            return pkt

        while len(self._input) < pkt.length+2:
            self._input += self._read_raw()

        try:
            pkt.unpack_payload(self._input[:pkt.length+2])
        except RatpInvalidPayload as e:
            self.total_crc_errors += 1
            return
        finally:
            self._input = self._input[pkt.length+2:]

        return pkt

    def _close(self):
        pass

    def _a(self, r):
        logging.info("A")

        if r.c_rst:
            return True

        if r.c_ack:
            s = RatpPacket(flags='R')
            s.c_sn = r.c_an
            self._write(s)
            return False

        if r.c_syn:
            self._r_mdl = r.length

            s = RatpPacket(flags='SA')
            s.c_sn = 0
            s.c_an = (r.c_sn + 1) % 2
            s.length = self._s_mdl
            self._write(s)
            self._state = RatpState.syn_received
            return False

        return False

    def _b(self, r):
        logging.info("B")

        if r.c_ack and r.c_an != (self._s_sn + 1) % 2:
            if r.c_rst:
                return False
            else:
                s = RatpPacket(flags='R')
                s.c_sn = r.c_an
                self._write(s)
                return False

        if r.c_rst:
            if r.c_ack:
                self._retrans = None
                # FIXME: delete the TCB
                self._state = RatpState.closed
                return False
            else:
                return False

        if r.c_syn:
            if r.c_ack:
                self._r_mdl = r.length
                self._retrans = None
                self._r_sn = r.c_sn
                s = RatpPacket(flags='A')
                s.c_sn = r.c_an
                s.c_an = (r.c_sn + 1) % 2
                self._write(s)
                self._state = RatpState.established
                return False
            else:
                self._retrans = None
                s = RatpPacket(flags='SA')
                s.c_sn = 0
                s.c_an = (r.c_sn + 1) % 2
                s.length = self._s_mdl
                self._write(s)
                self._state = RatpState.syn_received
                return False

        return False

    def _c1(self, r):
        logging.info("C1")

        if r.c_sn != self._r_sn:
            return True

        if r.c_rst or r.c_fin:
            return False

        s = RatpPacket(flags='A')
        s.c_sn = r.c_an
        s.c_an = (r.c_sn + 1) % 2
        self._write(s)
        return False

    def _c2(self, r):
        logging.info("C2")

        if r.c_sn != self._r_sn:
            return True

        if r.c_rst or r.c_fin:
            return False

        if r.c_syn:
            s = RatpPacket(flags='RA')
            s.c_sn = r.c_an
            s.c_an = (r.c_sn + 1) % 2
            self._write(s)
            self._retrans = None
            # FIXME: inform the user "Error: Connection reset"
            self._state = RatpState.closed
            return False

        logging.info("C2: duplicate packet")
        s = RatpPacket(flags='A')
        s.c_sn = r.c_an
        s.c_an = (r.c_sn + 1) % 2
        self._write(s)

        return False

    def _d1(self, r):
        logging.info("D1")

        if not r.c_rst:
            return True

        if self._passive:
            self._retrans = None
            self._state = RatpState.listen
            return False
        else:
            self._retrans = None

            self._state = RatpState.closed
            raise RatpError("Connection refused")

    def _d2(self, r):
        logging.info("D2")

        if not r.c_rst:
            return True

        self._retrans = None

        self._state = RatpState.closed

        raise RatpError("Connection reset")

    def _d3(self, r):
        logging.info("C3")

        if not r.c_rst:
            return True

        self._state = RatpState.closed
        return False

    def _e(self, r):
        logging.info("E")

        if not r.c_syn:
            return True

        self._retrans = None
        s = RatpPacket(flags='R')
        if r.c_ack:
            s.c_sn = r.c_an
        else:
            s.c_sn = 0
        self._write(s)
        self._state = RatpState.closed
        raise RatpError("Connection reset")

    def _f1(self, r):
        logging.info("F1")

        if not r.c_ack:
            return False

        if r.c_an == (self._s_sn + 1) % 2:
            return True

        if self._passive:
            self._retrans = None
            s = RatpPacket(flags='R')
            s.c_sn = r.c_an
            self._write(s)
            self._state = RatpState.listen
            return False
        else:
            self._retrans = None
            s = RatpPacket(flags='R')
            s.c_sn = r.c_an
            self._write(s)
            self._state = RatpState.closed
            raise RatpError("Connection refused")

    def _f2(self, r):
        logging.info("F2")

        if not r.c_ack:
            return False

        if r.c_an == (self._s_sn + 1) % 2:
            if self._retrans:
                self._retrans = None
                self._update_srtt(monotonic()-self._tx_timestamp)
                # FIXME: inform the user with an "Ok" if a buffer has been
                # entirely acknowledged.  Another packet containing data may
                # now be sent.
            return True

        return True

    def _f3(self, r):
        logging.info("F3")

        if not r.c_ack:
            return False

        if r.c_an == (self._s_sn + 1) % 2:
            return True

        return True

    def _g(self, r):
        logging.info("G")

        if not r.c_rst:
            return False

        self._retrans = None
        if r.c_ack:
            s = RatpPacket(flags='R')
            s.c_sn = r.c_an
            self._write(s)
        else:
            s = RatpPacket(flags='RA')
            s.c_sn = r.c_an
            s.c_an = (r.c_sn + 1) % 2
            self._write(s)

        return False

    def _h1(self, r):
        logging.info("H1")
        self._state = RatpState.established
        return self._common_i1(r)

    def _h2(self, r):
        logging.info("H2")

        if not r.c_fin:
            return True

        if self._retrans is not None:
            # FIXME: inform the user "Warning: Data left unsent.", "Connection closing."
            self._retrans = None
        s = RatpPacket(flags='FA')
        s.c_sn = r.c_an
        s.c_an = (r.c_sn + 1) % 2
        self._write(s)
        self._state = RatpState.last_ack
        raise RatpError("Connection closed by remote")

    def _h3(self, r):
        logging.info("H3")

        if not r.c_fin:
            # Our fin was lost, rely on retransmission
            return False

        if (r.length and not r.c_syn and not r.c_rst and not r.c_fin) or r.c_so:
            self._retrans = None
            s = RatpPacket(flags='RA')
            s.c_sn = r.c_an
            s.c_an = (r.c_sn + 1) % 2
            self._write(s)
            self._state = RatpState.closed
            raise RatpError("Connection reset")

        if r.c_an == (self._s_sn + 1) % 2:
            self._retrans = None
            s = RatpPacket(flags='A')
            s.c_sn = r.c_an
            s.c_an = (r.c_sn + 1) % 2
            self._write(s)
            self._time_wait_deadline = monotonic() + self._get_rto()
            self._state = RatpState.time_wait
            return False
        else:
            self._retrans = None
            s = RatpPacket(flags='A')
            s.c_sn = r.c_an
            s.c_an = (r.c_sn + 1) % 2
            self._write(s)
            self._state = RatpState.closing
            return False

    def _h4(self, r):
        logging.info("H4")

        if r.c_an == (self._s_sn + 1) % 2:
            self._retrans = None
            self._time_wait_deadline = monotonic() + self._get_rto()
            self._state = RatpState.time_wait
            return False

        return False

    def _h5(self, r):
        logging.info("H5")

        if r.c_an == (self._s_sn + 1) % 2:
            self._time_wait_deadline = monotonic() + self._get_rto()
            self._state = RatpState.time_wait
            return False

        return False

    def _h6(self, r):
        logging.info("H6")

        if not r.c_ack:
            return False

        if not r.c_fin:
            return False

        self._retrans = None
        s = RatpPacket(flags='A')
        s.c_sn = r.c_an
        s.c_an = (r.c_sn + 1) % 2
        self._write(s)
        self._time_wait_deadline = monotonic() + self._get_rto()
        return False

    def _common_i1(self, r):
        if r.c_so:
            self._r_sn = r.c_sn
            self._rx_buf.append(chr(r.length))
        elif r.length and not r.c_syn and not r.c_rst and not r.c_fin:
            self._r_sn = r.c_sn
            self._rx_buf.append(r.payload)
        else:
            return False

        # reassemble
        if r.c_eor:
            logging.info("Reassembling %i frames", len(self._rx_buf))
            self._rx_queue.append(''.join(self._rx_buf))
            self._rx_buf = []

        s = RatpPacket(flags='A')
        s.c_sn = r.c_an
        s.c_an = (r.c_sn + 1) % 2
        self._write(s)
        return False

    def _i1(self, r):
        logging.info("I1")
        return self._common_i1(r)

    def _machine(self, pkt):
        logging.info("State: %r", self._state)
        if self._state == RatpState.listen:
            self._a(pkt)
        elif self._state == RatpState.syn_sent:
            self._b(pkt)
        elif self._state == RatpState.syn_received:
            self._c1(pkt) and \
                self._d1(pkt) and \
                self._e(pkt) and \
                self._f1(pkt) and \
                self._h1(pkt)
        elif self._state == RatpState.established:
            self._c2(pkt) and \
                self._d2(pkt) and \
                self._e(pkt) and \
                self._f2(pkt) and \
                self._h2(pkt) and \
                self._i1(pkt)
        elif self._state == RatpState.fin_wait:
            self._c2(pkt) and \
                self._d2(pkt) and \
                self._e(pkt) and \
                self._f3(pkt) and \
                self._h3(pkt)
        elif self._state == RatpState.last_ack:
            self._c2(pkt) and \
                self._d3(pkt) and \
                self._e(pkt) and \
                self._f3(pkt) and \
                self._h4(pkt)
        elif self._state == RatpState.closing:
            self._c2(pkt) and \
                self._d3(pkt) and \
                self._e(pkt) and \
                self._f3(pkt) and \
                self._h5(pkt)
        elif self._state == RatpState.time_wait:
            self._d3(pkt) and \
                self._e(pkt) and \
                self._f3(pkt) and \
                self._h6(pkt)
        elif self._state == RatpState.closed:
            self._g(pkt)

    def wait(self, deadline):
        while deadline is None or deadline > monotonic():
            pkt = self._read()
            if pkt:
                self._machine(pkt)
            else:
                self._check_rto()
                self._check_time_wait()
            if not self._retrans or self._rx_queue:
                return

    def wait1(self, deadline):
        while deadline is None or deadline > monotonic():
            pkt = self._read()
            if pkt:
                self._machine(pkt)
            else:
                self._check_rto()
                self._check_time_wait()
            if not self._retrans:
                return

    def listen(self):
        logging.info("LISTEN")
        self._state = RatpState.listen

    def connect(self, timeout=5.0):
        deadline = monotonic() + timeout
        logging.info("CONNECT")
        self._retrans = None
        syn = RatpPacket(flags='S')
        syn.length = self._s_mdl
        self._write(syn)
        self._state = RatpState.syn_sent
        self.wait(deadline)

    def send_one(self, data, eor=True, timeout=1.0):
        deadline = monotonic() + timeout
        logging.info("SEND_ONE (len=%i, eor=%r)", len(data), eor)
        assert self._state == RatpState.established
        assert self._retrans is None
        snd = RatpPacket(flags='A')
        snd.c_eor = eor
        snd.c_sn = (self._s_sn + 1) % 2
        snd.c_an = (self._r_sn + 1) % 2
        snd.length = len(data)
        snd.payload = data
        self._write(snd)
        self.wait1(deadline=None)

    def send(self, data, timeout=1.0):
        logging.info("SEND (len=%i)", len(data))
        while len(data) > 255:
            self.send_one(data[:255], eor=False, timeout=timeout)
            data = data[255:]
        self.send_one(data, eor=True, timeout=timeout)

    def recv(self, timeout=1.0):
        deadline = monotonic() + timeout

        assert self._state == RatpState.established
        if self._rx_queue:
            return self._rx_queue.pop(0)
        self.wait(deadline)
        if self._rx_queue:
            return self._rx_queue.pop(0)

    def close(self, timeout=1.0):
        deadline = monotonic() + timeout
        logging.info("CLOSE")
        if self._state == RatpState.established or self._state == RatpState.syn_received:
            fin = RatpPacket(flags='FA')
            fin.c_sn = (self._s_sn + 1) % 2
            fin.c_an = (self._r_sn + 1) % 2
            self._write(fin)
            self._state = RatpState.fin_wait
        while deadline > monotonic() and not self._state == RatpState.time_wait:
            self.wait(deadline)
        while self._state == RatpState.time_wait:
            self.wait(None)
        if self._state == RatpState.closed:
            logging.info("CLOSE: success")
        else:
            logging.info("CLOSE: failure")


    def abort(self):
        logging.info("ABORT")

    def status(self):
        logging.info("STATUS")
        return self._state


class SerialRatpConnection(RatpConnection):
    def __init__(self, port):
        super(SerialRatpConnection, self).__init__()
        self.__port = port
        self.__port.timeout = 0.01
        self.__port.writeTimeout = None
        self.__port.flushInput()

    def _write_raw(self, data):
        if data:
            logging.debug("-> %r", bytearray(data))
        return self.__port.write(data)

    def _read_raw(self, size=1):
        data = self.__port.read(size)
        if data:
            logging.debug("<- %r", bytearray(data))
        return data
