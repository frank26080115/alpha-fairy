#!/usr/bin/env python3

import socket
import struct
import ptpcamera
import ptpcodes, ptpsonycodes
import fairyutilssim as fairyutils

PTPSTATE_GETDEVINFO            = 8
PTPSTATE_GETSTORAGEIDS         = 10
PTPSTATE_SDIOCONNECT1          = 12
PTPSTATE_SDIOCONNECT2          = 14
PTPSTATE_SDIOGETEXTDEVICEINFO  = 16
PTPSTATE_SDIOCONNECT3          = 18
PTPSTATE_SDIOGETEXTDEVICEINFO2 = 20

PTP_ALL_INIT_STEPS = [
        [PTPSTATE_GETDEVINFO,            "GetDevInfo",            ptpcodes.OPCODE_GetDeviceInfo,   None],
        [PTPSTATE_GETSTORAGEIDS,         "GetStorageIDs",         ptpcodes.OPCODE_GetStorageIDs,   None],
        [PTPSTATE_SDIOCONNECT1,          "SDIOConnect1",          ptpsonycodes.OPCODE_SDIOConnect, [1, 0, 0]],
        [PTPSTATE_SDIOCONNECT2,          "SDIOConnect2",          ptpsonycodes.OPCODE_SDIOConnect, [2, 0, 0]],
        [PTPSTATE_SDIOGETEXTDEVICEINFO,  "SDIOGetExtDeviceInfo",  ptpsonycodes.OPCODE_SDIOGetExtDeviceInfo, [0x12C, 0, 0]], # this is 0xC8 on the a6600
        [PTPSTATE_SDIOCONNECT3,          "SDIOConnect3",          ptpsonycodes.OPCODE_SDIOConnect, [3, 0, 0]],
        [PTPSTATE_SDIOGETEXTDEVICEINFO2, "SDIOGetExtDeviceInfo2", ptpsonycodes.OPCODE_SDIOGetExtDeviceInfo, [0x12C, 0, 0]],
    ]

INTERESTED_PROPERTIES = [
    ptpsonycodes.PROPCODE_FocusFound,
    ptpsonycodes.PROPCODE_FocusMode,
    ptpsonycodes.PROPCODE_Recording,
    ptpsonycodes.PROPCODE_FocusArea,
]

class PtpSonyAlphaCamera(ptpcamera.PtpCamera):
    def __init__(self, sock1, sock2, hostapp=None):
        ptpcamera.PtpCamera.__init__(self, sock1, sock2, hostapp=hostapp)
        self.prop_dict = {}
        self.init_seq = PTP_ALL_INIT_STEPS

    def data_stream(self, chunk):
        if self.properties_pending:
            if chunk is not None and len(chunk) > 0:
                self.databuff.extend(chunk)

    def update_property(self, prop_code, data_type, data_chunk, data_size):
        show_change = True
        if data_type > 0x000A:
            return
        if len(INTERESTED_PROPERTIES) > 0 and prop_code not in INTERESTED_PROPERTIES:
            return
        x = decode_chunk_to_int(data_type, data_chunk)
        if show_change:
            change_code = ""
            if prop_code not in self.prop_dict:
                change_code = "NEW"
            elif self.prop_dict[prop_code] != x:
                change_code = "CHG"
            if len(change_code) > 0:
                print("DevProp %s [%s]: %s (%u)" % (change_code, ptpcodes.get_name(prop_code), decoder_print_hex(data_type, data_chunk, 1), x))
        self.prop_dict[prop_code] = x

    def decode_pkt(self, pkt):
        (plen,) = struct.unpack("<I", pkt[0:4])
        (pkttype,) = struct.unpack("<I", pkt[4:8])
        if pkttype == ptpcodes.PKTTYPE_OPERRESP and self.state >= ptpcamera.PTPSTATE_CHECKEVENTS:
            self.state &= 0xFFFFFFFE
            if self.properties_pending:
                self.decode_properties()
        elif pkttype == ptpcodes.PKTTYPE_ENDDATA:
            if self.properties_pending:
                self.decode_properties()
        elif pkttype == ptpcodes.PKTTYPE_EVENT:
            (eventcode,) = struct.unpack("<H", pkt[8:10])
            if eventcode == ptpsonycodes.EVENTCODE_PropertyChanged:
                self.need_check_props = True
        ptpcamera.PtpCamera.decode_pkt(self, pkt)

    def decode_properties(self):
        self.properties_pending = False
        if len(self.databuff) <= 0:
            return
        decode_properties(self, self.databuff)

    def check_dev_props(self):
        self.need_check_props = False
        self.properties_pending = True
        self.check_props_time = fairyutils.time_millis()
        self.send_operation_request_packet(ptpsonycodes.OPCODE_GetAllDevicePropData)

    def task(self):
        if self.state >= ptpcamera.PTPSTATE_CHECKEVENTS:
            if self.need_check_props and self.state == ptpcamera.PTPSTATE_CHECKEVENTS:
                self.check_dev_props()
            else:
                now = fairyutils.time_millis()
                if (now - self.check_props_time) > 500:
                    self.need_check_props = True
        ptpcamera.PtpCamera.task(self)

    def wait_while_busy(self, min_time = 0, timeout = 1000, check_focus = False):
        t_start = fairyutils.time_millis()
        t_now = t_start
        while ((self.state & 1) != 0 and (t_now - t_start) < timeout) or (t_now - t_start) < min_time:
            t_now = fairyutils.time_millis()
            self.task()
            if check_focus:
                if ptpsonycodes.PROPCODE_FocusFound in self.prop_dict:
                    if self.prop_dict[ptpsonycodes.PROPCODE_FocusFound] == ptpsonycodes.FOCUSSTATUS_FOCUSED:
                        break

    def is_manuallyfocused(self):
        is_mf = False
        if ptpsonycodes.PROPCODE_FocusMode in self.prop_dict:
            curval = self.prop_dict[ptpsonycodes.PROPCODE_FocusMode]
            is_mf = curval == 1
        return is_mf

    def is_movierecording(self):
        is_rec = False
        if ptpsonycodes.PROPCODE_Recording in self.prop_dict:
            curval = self.prop_dict[ptpsonycodes.PROPCODE_Recording]
            is_rec = curval != 0
        return is_rec

    def is_spotfocus(self):
        if self.is_manuallyfocused():
            return False
        if ptpsonycodes.PROPCODE_FocusArea in self.prop_dict:
            curval = self.prop_dict[ptpsonycodes.PROPCODE_FocusArea]
            if curval == ptpsonycodes.FOCUSAREA_ZONE or curval == ptpsonycodes.FOCUSAREA_TRACKING_ZONE or curval == ptpsonycodes.FOCUSAREA_MOVEABLE_SMALL  or curval == ptpsonycodes.FOCUSAREA_MOVEABLE_MEDIUM or curval == ptpsonycodes.FOCUSAREA_MOVEABLE_LARGE or curval == ptpsonycodes.FOCUSAREA_MOVEABLE_EXPAND or curval == ptpsonycodes.FOCUSAREA_TRACKING_MOVEABLE_SMALL or curval == ptpsonycodes.FOCUSAREA_TRACKING_MOVEABLE_MEDIUM or curval == ptpsonycodes.FOCUSAREA_TRACKING_MOVEABLE_LARGE  or curval == ptpsonycodes.FOCUSAREA_TRACKING_MOVEABLE_EXPAND:
                return True
        return False

    def ptpcmd(self, opcode, param_1=None, param_2=None,
                #param_3=None,
                #param_4=None,
                #param_5=None,
                data=None):
        params = []
        if param_1 is not None:
            params.append(param_1)
        if param_2 is not None:
            params.append(param_2)
        #if param_3 is not None:
        #    params.append(param_3)
        #if param_4 is not None:
        #    params.append(param_4)
        #if param_5 is not None:
        #    params.append(param_5)
        self.send_operation_request_packet(opcode, payload32 = params, payload8 = data)

    def ptpcmd_AutoFocus(self, onoff):
        self.wait_while_busy()
        self.ptpcmd(ptpsonycodes.OPCODE_SetControlDeviceB, param_1 = ptpsonycodes.PROPCODE_AutoFocus, data = [2 if onoff else 1, 0])

    def ptpcmd_ShutterOperate(self, openclose):
        self.wait_while_busy()
        self.ptpcmd(ptpsonycodes.OPCODE_SetControlDeviceB, param_1 = ptpsonycodes.PROPCODE_Capture, data = [2 if openclose else 1, 0])

    def ptpcmd_ManualFocusStep(self, step):
        self.wait_while_busy()
        self.ptpcmd(ptpsonycodes.OPCODE_SetControlDeviceB, param_1 = ptpsonycodes.PROPCODE_ManualFocusStep, data = struct.pack("<h", step))

    def ptpcmd_FocusPointSet(self, x, y):
        self.wait_while_busy()
        self.ptpcmd(ptpsonycodes.OPCODE_SetControlDeviceB, param_1 = ptpsonycodes.PROPCODE_FocusPointSet2, data = struct.pack("<HH", y, x))

    def shoot(self, t = 250):
        self.wait_while_busy()
        self.ptpcmd_ShutterOperate(True)
        self.wait_while_busy(min_time = t)
        self.ptpcmd_ShutterOperate(False)
        self.wait_while_busy()

    def ptpcmd_MovieRecord(self, onoff):
        is_rec = self.is_movierecording()
        if (onoff and is_rec) or (not onoff and not is_rec):
            return
        self.wait_while_busy()
        self.ptpcmd(ptpsonycodes.OPCODE_SetControlDeviceB, param_1 = ptpsonycodes.PROPCODE_Movie, data = [2 if onoff else 1, 0])

    def ptpcmd_MovieRecordToggle(self):
        is_rec = self.is_movierecording()
        self.ptpcmd_MovieRecord(not is_rec)

    def ptpcmd_ManualFocusMode(self, onoff):
        is_mf = self.is_manuallyfocused()
        if (onoff and is_mf) or (not onoff and not is_mf):
            return
        self.wait_while_busy()
        self.ptpcmd(ptpsonycodes.OPCODE_SetControlDeviceB, param_1 = ptpsonycodes.PROPCODE_ManualFocusMode, data = [2 if onoff else 1, 0])

    def ptpcmd_ManualFocusModeToggle(self):
        is_mf = self.is_manuallyfocused()
        self.ptpcmd_ManualFocusMode(not is_mf)

def decode_properties(sink, databuff, has_weird_form = True):
    datalen = len(databuff)
    prop_cnt = datalen # placeholder right now, will be read properly on first loop
    i = 0 # byte index within byte array
    j = 0 # index of property item
    while i < datalen and j < prop_cnt:
        if i == 0:
            (prop_cnt,) = struct.unpack("<I", databuff[0:4])
            decoder_print("device properties count: %d , len = %d" % (prop_cnt, datalen))
            i += 8
            continue
        j += 1

        (prop_code,) = struct.unpack("<H", databuff[i:i+2])
        i += 2
        #decoder_print("\nPROP[%u][idx %u][PC 0x%04X]" % (j, i, prop_code))
        decoder_print("\nPROP[%u][idx %u][PC %s]" % (j, i, ptpcodes.get_name(prop_code)))
        (data_type,) = struct.unpack("<H", databuff[i:i+2])
        i += 2
        if prop_code == 0:
            decoder_print(" unknown property code 0")
            i += 4
            continue
        if data_type == 0:
            decoder_print(" unknown data type 0")
            i += 4
            continue

        if databuff[i] > 1:
            decoder_print(" warn: get/set wrong 0x%02X " % databuff[i])
        i += 2 # skips the get/set visibility byte

        if data_type <= 0x0A:
            decoder_print("[DT 0x%X]" % (data_type))

        dsz = datatype_to_bytes(data_type)

        data_chunk = None

        if data_type <= 0x000A: # simple integers
            i += dsz # skips the default value
            data_chunk = databuff[i : i + dsz]
            decoder_print(decoder_print_hex(data_type, data_chunk, 1))
            i += dsz # skips the data value
        elif data_type == 0xFFFF: # string type
            i -= 2
            if databuff[i] == 0x01 and databuff[i + 1] == 0x01:
                i += 2
                (strlen,) = struct.unpack("<H", databuff[i:i+2])
                i += 2
            elif databuff[i] == 0x00 and databuff[i + 1] == 0x02 and databuff[i + 2] == 0x00:
                # I don't understand this either
                i += 3
                strlen = databuff[i]
                i += 1
            decoder_print("[STR %u]" % (strlen))
            if strlen > 0:
                data_chunk = databuff[i : i + (strlen * 2)]
                data_str = data_chunk.decode("utf-16")
                decoder_print("\"%s\"" % data_str)
                i += strlen * 2
            else:
                decoder_print(" weird empty string ")
                i += 0
        elif (data_type & 0x4000) != 0: # array type
            (elecnt,) = struct.unpack("<I", databuff[i:i+4])
            i += 4
            decoder_print("[ARR %u]" % (elecnt))
            if elecnt > 0:
                data_chunk = databuff[i : i + (elecnt * dsz)]
                decoder_print(decoder_print_hex(data_type, data_chunk, elecnt))
            i += dsz * elecnt

        if sink is not None:
            sink.update_property(prop_code, data_type, data_chunk, dsz)

        formflag = databuff[i]
        i += 1
        if formflag == 0:
            continue
        elif formflag == 0x01: # range
            rng_min = fairyutils.bytearray_get_uint_n(databuff, i, dsz)
            i += dsz
            rng_max = fairyutils.bytearray_get_uint_n(databuff, i, dsz)
            i += dsz
            rng_stp = fairyutils.bytearray_get_uint_n(databuff, i, dsz)
            i += dsz
            decoder_print(" [FRM RNG %d-%d x %d]" % (rng_min, rng_max, rng_stp))
            continue
        elif formflag == 0x02: # enumeration
            frm_start_idx = i
            (elecnt,) = struct.unpack("<H", databuff[i:i+2])
            i += 2
            i += dsz * elecnt
            frm_end_idx = i
            decoder_print(" [FRM ENUM %d]" % (elecnt))
            form_chunk_1 = databuff[frm_start_idx:frm_end_idx]
            form_chunk_2 = databuff[frm_end_idx:frm_end_idx + len(form_chunk_1)]
            # the weird form format allows for a second set of enums that have a different length
            # to detect if the camera is using this weird form format, we need to see if it has at least one with a repeated form
            if has_weird_form == False and all(x == y for x, y in zip(form_chunk_1, form_chunk_2)):
                i += len(form_chunk_1)
                has_weird_form = True
            elif has_weird_form:
                # the weird form format allows for a second set of enums that have a different length
                (elecnt,) = struct.unpack("<H", databuff[i:i+2])
                i += 2
                i += dsz * elecnt
            continue
    decoder_print("\nEND OF PROPERTIES\n") # new line

def decoder_print(s):
    #print(s, end="")
    pass

def decoder_print_hex(data_type, data_chunk, elecnt):
    dlen = len(data_chunk)
    dsz = datatype_to_bytes(data_type)
    s = ""
    i = 0
    j = 0
    k = 0
    while j < elecnt and i < dlen:
        s += " 0x"
        k = 0
        while k < dsz:
            d = data_chunk[i + dsz - 1 - k]
            s += "%02X" % d
            k += 1
        i += dsz
        j += 1
    return s

def datatype_to_bytes(data_type):
    dsz = 1
    data_type_4bit = data_type & 0x0F
    if data_type_4bit == 0x01 or data_type_4bit == 0x02:
        dsz = 1
    elif data_type_4bit == 0x03 or data_type_4bit == 0x04:
        dsz = 2
    elif data_type_4bit == 0x05 or data_type_4bit == 0x06:
        dsz = 4
    elif data_type_4bit == 0x07 or data_type_4bit == 0x08:
        dsz = 8
    elif data_type_4bit == 0x09 or data_type_4bit == 0x0A:
        dsz = 16
    return dsz

def decode_chunk_to_int(data_type, data_chunk):
    cnt = datatype_to_bytes(data_type)
    x = fairyutils.bytearray_get_uint_n(data_chunk, 0, cnt)
    if (data_type & 1) != 0: # signed
        if x >= (2 ** ((8 * cnt) - 1)):
            x -= (2 ** ((8 * cnt)))
    return x
