#!/usr/bin/env python3

import socket
import struct

import fairyutils

import ptpcodes, ptpsonycodes

PTPSTATE_INIT                  = 0
PTPSTATE_INIT_WAIT_CMD_ACK     = 1
PTPSTATE_INIT_EVENT_REQ        = 2
PTPSTATE_INIT_WAIT_EVENT_ACK   = 3
PTPSTATE_OPEN_SESSION          = 4
PTPSTATE_OPEN_SESSION_WAIT     = 5
PTPSTATE_SESSION_INIT          = 6
PTPSTATE_CHECKEVENTS           = 30
PTPSTATE_SAVEIMG               = 40

PTPRESPCODE_OK             = 0x2001

class PtpIpReader:
    def __init__(self, sock, parent):
        self.socket = sock
        self.rxbuff = bytearray()
        self.rx_lasttime = 0
        self.pending_bytes_total = 0
        self.pending_bytes_transaction = 0
        self.parent = parent
        self.init_seq = [] # PTP_ALL_INIT_STEPS

    def poll(self):
        try:
            x = self.socket.recv(1024*4)
            if x is not None and len(x) > 0:
                self.rx_lasttime = fairyutils.time_millis()
                self.poll_inner(x)
                tries = 3
                while len(self.rxbuff) > 4 and tries > 0:
                    self.poll_inner(None)
                    tries -= 1
        except socket.timeout as ex:
            pass
        except Exception as ex:
            print("ERROR in check_rx, exception: %s" % ex)
            raise ex
        now = fairyutils.time_millis()
        if (now - self.rx_lasttime) >= 2000 and len(self.rxbuff) > 0: # timeout, not a full packet according to the header, but we should process it anyways
            self.poll_inner(None, ignore_length = True)

    def poll_inner(self, x, can_recurse = True, ignore_length = False):
        if x is not None and len(x) > 0:
            self.rxbuff.extend(x)
        if self.pending_bytes_total > 0:
            if self.pending_bytes_transaction == 0:
                (rlen,) = struct.unpack("<I", self.rxbuff[0:4])
                blen = len(self.rxbuff)
                if rlen <= blen:
                    (pkttype,) = struct.unpack("<I", self.rxbuff[4:8])
                    if pkttype == ptpcodes.PKTTYPE_DATA:
                        self.pending_bytes_transaction = rlen - 12
                        self.rxbuff = self.rxbuff[12:]
                        # fall through to next if statement
                    elif pkttype == ptpcodes.PKTTYPE_ENDDATA or pkttype == ptpcodes.PKTTYPE_CANCELDATA:
                        self.pending_bytes_transaction = 0
                        self.pending_bytes_total = 0
                        pkt = self.rxbuff[0:rlen]
                        self.parent.decode_pkt(pkt)
                        self.rxbuff = self.rxbuff[rlen:]
                    else:
                        pkt = self.rxbuff[0:rlen]
                        self.rxbuff = self.rxbuff[rlen:]
                        self.parent.decode_pkt(pkt)
                elif ignore_length:
                    self.parent.data_stream(self.rxbuff)
                    self.pending_bytes_transaction = 0
                    self.pending_bytes_total = 0
                    self.rxbuff = bytearray()
                else:
                    # incomplete chunk
                    pass
            blen = len(self.rxbuff)
            if self.pending_bytes_transaction > 0:
                if self.pending_bytes_transaction <= blen:
                    payload = self.rxbuff[:self.pending_bytes_transaction]
                    self.parent.data_stream(payload)
                    self.rxbuff = self.rxbuff[self.pending_bytes_transaction:]
                    self.pending_bytes_total -= self.pending_bytes_transaction
                    self.pending_bytes_transaction = 0
                else:
                    self.parent.data_stream(self.rxbuff)
                    self.pending_bytes_transaction -= blen
                    self.pending_bytes_total -= blen
                    self.rxbuff = bytearray()
        elif len(self.rxbuff) > 4:
            (rlen,) = struct.unpack("<I", self.rxbuff[0:4])
            if len(self.rxbuff) >= rlen:
                pkt = self.rxbuff[0:rlen]
                self.rxbuff = self.rxbuff[rlen:]
                self.parent.decode_pkt(pkt)
            elif ignore_length and rlen > len(self.rxbuff):
                self.parent.decode_pkt(self.rxbuff)
                self.rxbuff = bytearray()

class PtpCamera:

    def __init__(self, sock1, sock2, hostapp=None):
        self.socket = sock1
        self.socket_evt = sock2
        self.reader = PtpIpReader(sock1, self)
        self.reader_evt = PtpIpReader(sock2, self)
        self.databuff = bytearray()
        self.session_id = None
        self.session_opened = False
        self.transaction_id = None
        self.connection_num = None
        self.responder_guid = None
        self.responder_friendly_name = None
        self.responder_protocol_ver = None
        self.properties_pending = False
        self.operation_pending = False
        self.state = PTPSTATE_INIT
        self.substate = 0
        self.host_app = hostapp

    def init_cmd_req(self, guid = None, friendly_name = "ALPHA-FAIRY", get_resp = True):
        barr = bytearray()
        barr.extend(struct.pack("<I", 0)) # length field to be populated later
        barr.extend(struct.pack("<I", ptpcodes.PKTTYPE_INITCMDREQ))
        if guid is None:
            guid = fairyutils.guid()
        barr.extend(guid)
        # note: micropython doesn't support utf-16 encoding for bytearray constructor
        try:
            name_arr = bytearray(friendly_name)
        except:
            name_arr = bytearray(friendly_name, encoding="utf-8")
        for i in name_arr:
            barr.extend([i, 0x00])
        barr.extend([0x00, 0x00]) # null terminate the friendly_name
        barr.extend([0x00, 0x00, 0x01, 0x00]) # uint32 binary protocol version
        barr = packet_setlength(barr)
        self.socket.send(barr)
        if self.transaction_id is None:
            self.transaction_id = 0
        else:
            self.transaction_id += 1

    def decode_init_cmd_ack(self, barr, start_idx = 8):
        (blen,) = struct.unpack("<I", barr[0:4])
        (self.connection_num,) = struct.unpack("<I", barr[start_idx:start_idx+4])
        start_idx += 4
        self.responder_guid = bytearray()
        i = 0
        j = start_idx + i
        while i < 16 and j < len(barr):
            self.responder_guid.append(barr[j])
            i += 1
            j += 1
        strarr = bytearray()
        while j < len(barr) and i < (blen - 2):
            b2 = barr[j : j + 2]
            if b2[0] == 0 and b2[1] == 0:
                self.responder_friendly_name = strarr.decode("utf-16")
            else:
                strarr.extend(b2)
            j += 2
        if j == blen - 2:
            self.responder_protocol_ver = barr[j:j+1]
        print("PTP decoded init cmd ack from \"%s\"" % self.responder_friendly_name)

    def init_event_req(self, conn_num = None):
        if conn_num is None:
            if self.connection_num is None:
                self.connection_num = 1
            conn_num = self.connection_num
        barr = bytearray()
        barr.extend(struct.pack("<I", 0)) # length field to be populated later
        barr.extend(struct.pack("<I", ptpcodes.PKTTYPE_INITEVENTREQ))
        barr.extend(struct.pack("<I", conn_num))
        barr = packet_setlength(barr)
        self.socket_evt.send(barr)
        if self.transaction_id is None:
            self.transaction_id = 0
        else:
            self.transaction_id += 1

    def open_session(self, sess_id = None):
        if sess_id is None and self.session_id is not None:
            sess_id = self.session_id + 1
        elif sess_id is None and self.session_id is None:
            self.session_id = 1
            sess_id = 1
        elif sess_id is not None:
            pass
        elif self.session_id is not None:
            self.session_id += 1

        barr = bytearray()
        barr.extend(struct.pack("<I", 0)) # length field to be populated later
        barr.extend(struct.pack("<I", ptpcodes.PKTTYPE_OPERREQ))
        barr.extend(struct.pack("<I", 1)) # data-phase
        barr.extend(struct.pack("<H", ptpcodes.OPCODE_OpenSession))
        if self.transaction_id is None:
            self.transaction_id = 1
        else:
            self.transaction_id += 1
        barr.extend(struct.pack("<I", self.transaction_id))
        barr.extend(struct.pack("<I", self.session_id))
        barr = packet_setlength(barr)
        self.socket.send(barr)
        self.transaction_id += 1
        self.session_opened = False

    def send_operation_request_packet(self, opcode, payload32 = None, payload8 = None, dataphase = 1):
        barr = bytearray()
        barr.extend(struct.pack("<I", 0)) # length field to be populated later
        barr.extend(struct.pack("<I", ptpcodes.PKTTYPE_OPERREQ))
        barr.extend(struct.pack("<I", dataphase))
        barr.extend(struct.pack("<H", opcode))
        barr.extend(struct.pack("<I", self.transaction_id))
        if payload32 is not None:
            for i in payload32:
                barr.extend(struct.pack("<I", i))
        barr = packet_setlength(barr)
        self.socket.send(barr)
        if payload8 is not None:
            self.send_data(payload8)
        self.transaction_id += 1
        self.state |= 1

    def send_data(self, data):
        barr = bytearray()
        barr.extend(struct.pack("<I", 0)) # length field to be populated later
        barr.extend(struct.pack("<I", ptpcodes.PKTTYPE_STARTDATA))
        barr.extend(struct.pack("<I", self.transaction_id))
        barr.extend(struct.pack("<Q", len(data)))
        barr = packet_setlength(barr)
        self.socket.send(barr)
        barr = bytearray()
        barr.extend(struct.pack("<I", 0)) # length field to be populated later
        barr.extend(struct.pack("<I", ptpcodes.PKTTYPE_DATA))
        barr.extend(struct.pack("<I", self.transaction_id))
        for i in data:
            barr.extend(struct.pack("B", i))
        barr = packet_setlength(barr)
        self.socket.send(barr)
        barr = bytearray()
        barr.extend(struct.pack("<I", 0)) # length field to be populated later
        barr.extend(struct.pack("<I", ptpcodes.PKTTYPE_ENDDATA))
        barr.extend(struct.pack("<I", self.transaction_id))
        barr = packet_setlength(barr)
        self.socket.send(barr)

    def send_probe_resp(self):
        barr = bytearray()
        barr.extend(struct.pack("<I", 0))
        barr.extend(struct.pack("<I", ptpcodes.PKTTYPE_PROBERESP))
        barr = packet_setlength(barr)
        self.socket.send(barr)

    def decode_pkt(self, pkt):
        (plen,) = struct.unpack("<I", pkt[0:4])
        (pkttype,) = struct.unpack("<I", pkt[4:8])
        if pkttype == ptpcodes.PKTTYPE_INITCMDACK and (self.state == PTPSTATE_INIT or self.state == PTPSTATE_INIT_WAIT_CMD_ACK):
            self.decode_init_cmd_ack(pkt)
            self.state = PTPSTATE_INIT_EVENT_REQ
        elif pkttype == ptpcodes.PKTTYPE_INITEVENTACK and (self.state == PTPSTATE_INIT_EVENT_REQ or self.state == PTPSTATE_INIT_WAIT_EVENT_ACK):
            self.state = PTPSTATE_OPEN_SESSION
            print("PTP init: event ack done")
        elif pkttype == ptpcodes.PKTTYPE_OPERRESP and self.state == PTPSTATE_OPEN_SESSION_WAIT:
            respok, respcode = packet_resp_is_ok(pkt)
            if respok:
                self.state = PTPSTATE_SESSION_INIT
                self.substate = 0
                print("PTP init: session opened %u" % self.session_id)
            else:
                self.state = PTPSTATE_ERROR
                print("ERROR: OpenSession response not ok 0x%04X" % respcode)
        elif pkttype == ptpcodes.PKTTYPE_OPERRESP and self.state >= PTPSTATE_SESSION_INIT + 1 and self.state < PTPSTATE_CHECKEVENTS:
            respok, respcode = packet_resp_is_ok(pkt)
            if respok or respcode in ptpcodes.RESPCODE_OKish:
                self.state += 1
                self.substate += 1
                print("PTP init: step %u state %u" % (self.substate, self.state))
            else:
                print("ERROR: response not ok 0x%04X, state = %d" % (respcode, self.state))
            self.state &= 0xFFFFFFFE
        elif pkttype == ptpcodes.PKTTYPE_OPERRESP and self.state >= PTPSTATE_CHECKEVENTS:
            self.state &= 0xFFFFFFFE
        elif pkttype == ptpcodes.PKTTYPE_STARTDATA:
            (self.reader.pending_bytes_total,) = struct.unpack("<Q", pkt[12:12+8])
            self.databuff = bytearray()
            pass
        elif pkttype == ptpcodes.PKTTYPE_ENDDATA:
            pass
        elif pkttype == ptpcodes.PKTTYPE_EVENT:
            (eventcode,) = struct.unpack("<H", pkt[8:10])
            #print("EVENT: code 0x%04X" % eventcode)
            if self.host_app is not None:
                self.host_app.handle_ptp_event(eventcode)
            pass
        elif pkttype == ptpcodes.PKTTYPE_DATA:
            pass
        elif pkttype == ptpcodes.PKTTYPE_PROBEREQ:
            self.send_probe_resp()
        else:
            print("UNKNOWN PTP pkt type 0x%04X" % pkttype)

    def data_stream(self, chunk):
        # inherited and overridden
        pass

    def wait_while_busy(self, min_time = 0, timeout = 1000):
        t_start = fairyutils.time_millis()
        t_now = t_start
        while ((self.state & 1) != 0 and (t_now - t_start) < timeout) or (t_now - t_start) < min_time:
            t_now = fairyutils.time_millis()
            self.task()

    def is_ready(self):
        return self.state >= PTPSTATE_CHECKEVENTS

    def task(self):
        if self.state == PTPSTATE_INIT:
            self.init_cmd_req()
            print("PTP init: init cmd requested")
            self.state = PTPSTATE_INIT_WAIT_CMD_ACK
        elif self.state == PTPSTATE_INIT_EVENT_REQ:
            self.init_event_req()
            print("PTP init: events requested")
            self.state = PTPSTATE_INIT_WAIT_EVENT_ACK
        elif self.state == PTPSTATE_OPEN_SESSION:
            self.open_session()
            print("PTP init: opening session")
            self.state = PTPSTATE_OPEN_SESSION_WAIT
        elif self.state == PTPSTATE_CHECKEVENTS:
            pass
        elif self.state >= PTPSTATE_SESSION_INIT and (self.state & 1) == 0:
            if self.substate < len(self.init_seq):
                step = self.init_seq[self.substate]
                self.state = step[0] + 1
                print("PTP init step \"%s\"" % step[1])
                self.send_operation_request_packet(step[2], payload32=step[3])
            else:
                self.state = PTPSTATE_CHECKEVENTS
                print("PTP init done, check event loop")
                if self.host_app is not None:
                    self.host_app.finish_ptp_init()
        self.reader.poll()
        self.reader_evt.poll()

    def close(self):
        try:
            self.socket.close()
        except:
            pass
        try:
            self.socket_evt.close()
        except:
            pass

def packet_resp_is_ok(pkt):
    (pkttype,) = struct.unpack("<I", pkt[4:8])
    if pkttype != ptpcodes.PKTTYPE_OPERRESP:
        return False, None
    if len(pkt) <= 8:
        return True, None
    (respcode,) = struct.unpack("<H", pkt[8:10])
    return (respcode == ptpcodes.RESPCODE_OK), respcode

def packet_setlength(barr, start_idx = 0):
    r = struct.pack("<I", len(barr))
    barr[start_idx] = r[0]
    barr[start_idx + 1] = r[1]
    barr[start_idx + 2] = r[2]
    barr[start_idx + 3] = r[3]
    return barr
