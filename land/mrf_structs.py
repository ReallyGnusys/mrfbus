from ctypes import *
import datetime
from collections import OrderedDict


class MrfStruct(LittleEndianStructure):
    def fromstr(self, string):
        memmove(addressof(self), string, sizeof(self))
    def __len__(self):
        return sizeof(self)

    def __getitem__(self,attname):
        return getattr(self,attname)
    def attstr(self,attname):
        att = getattr(self,attname)

        #return "%s"%repr(att)
        if type(att) == type(""):
            return att
        if str(type(att)).find('c_ubyte_Array') > -1 :  #FIXME!
            atts = ""
            for i in att:
                if i != 0:
                    atts += "%c"%chr(i)
                else:
                    break
        elif str(type(att)).find('c_uint_Array') > -1:  #FIXME!
            atts = "["
            first = True
            for i in att:

                if not first:
                    atts +=","
                else:
                    first = False

                atts += "%u"%int(i)
            atts += "]"
        elif str(type(att)).find('PktTimeDate') > -1:  #FIXME!
            atts  = "%s"%repr(att)

        else:
            #print "att is "+str(type(att))
            atts = "%s"%hex(int(att))
        return atts
    def iter_fields(self):
        for field in self._fields_:
            yield field[0]
    def __repr__(self):
        s = '\n'
        for field in self._fields_:
            #att = getattr(self,field[0])

            atts = self.attstr(field[0])
            s += "%s %s\n"%(field[0],atts)
        return s
    def dic(self):
        dc = OrderedDict()
        for field in self._fields_:
            key = field[0]
            val = self[key]
            if str(type(val)).find('mrf_structs') > -1:
                val = self.attstr(key)
            if str(type(val)).find('Array') > -1:
                val = self.attstr(key)
            dc[key ] = val
        return dc

    def dic_set(self,dic):
        for attr in dic.keys():
            setattr(self,attr,dic[attr])
    def __eq__(self,other):
        if type(other) == type(None):
            return False
        for field in self._fields_:
            #att = getattr(self,field[0])
            #oatt = getattr(other,field[0])
            atts = self.attstr(field[0])
            oatts = other.attstr(field[0])
            if atts != oatts:
                print "cmp attname %s failed us %s other %s"%(field[0],atts,oatts)
                return False
        return True

    def __ne__(self,other):
        return not self.__eq__(other)

    def tbd_unmunge(self):
        print "mungeabletype is %s"%type(c_uint())
        for field in self._fields_:
            att = getattr(self,field[0])
            attype = field[1]
            print "got att %s  type %s"%(field[0],type(attype()))
            if type(attype()) == type(c_uint()):
                print "its for munging"
                nattr = att >> 16 + ((att & 0xffff) << 16)
                setattr(self,field[0],nattr)
        print repr(self)
    def load(self, bytes):
        fit = min(len(bytes), sizeof(self))
        memmove(addressof(self), bytes, fit)
    def dump(self):
        return buffer(self)[:]




# constants from somewhere..

MRFBUFFLEN = 128

# FIXME structs defined in mrf_sys_structs should be autogenerated here..

class PktHeader(MrfStruct):
    _fields_ = [
        ("length", c_uint8),
        ("hdest", c_uint8),
        ("netid", c_uint8),
        ("udest", c_uint8),
        ("type", c_uint8),
        ("hsrc", c_uint8),
        ("usrc", c_uint8),
        ("msgid", c_uint8)
    ]

class PktResp(MrfStruct):
    _fields_ = [
        ("type", c_uint8),
        ("msgid", c_uint8),
        ("rlen", c_uint8)
    ]
class PktDeviceInfo(MrfStruct):
    _fields_ = [
        ("dev_name", c_uint8*10),
        ("mrfid", c_uint8),
        ("netid", c_uint8),
        ("num_buffs", c_uint8),
        ("num_ifs", c_uint8)
    ]

class PktDeviceStatus(MrfStruct):
    _fields_ = [
        #("num_if", c_uint8),
        #("buffs_total", c_uint8),
        ("buffs_free", c_uint8),
        ("errors", c_uint8),
        ("tx_retries", c_uint16),
        #("pad2", c_uint16),
        ("rx_pkts", c_uint32),
        ("tx_pkts", c_uint32),
        ("tick_count", c_uint32)

    ]

class PktSysInfo(MrfStruct):
    _fields_ = [
        ("mrfbus_version", c_uint8*42),
        ("build", c_uint8*8),
        ("num_cmds", c_uint8),
        ("modified", c_uint8)
    ]

class PktTimeDate(MrfStruct):
    _fields_ = [
        ("sec",c_uint8),
        ("min", c_uint8),
        ("hour", c_uint8),
        ("day", c_uint8),
        ("mon", c_uint8),
        ("year", c_uint8)
        ]
    def __repr__(self):
        return "%02d:%02d:%02d %d/%d/%d"%(self.hour,self.min,self.sec,self.day,self.mon,self.year+2000)
    def set(self,dt):
        self.sec  = dt.second
        self.min  = dt.minute
        self.hour = dt.hour
        self.day  = dt.day
        self.mon  = dt.month
        self.year = dt.year - 2000
    def to_datetime(self):
        return datetime.datetime(year = self.year+2000,month=self.mon,day=self.day,hour=self.hour,minute=self.min,second=self.sec)


class PktCmdInfo(MrfStruct):
    _fields_ = [
        ("name",c_uint8*16),
        ("type", c_uint8),
        ("cflags", c_uint8),
        ("req_size", c_uint8),
        ("rsp_size", c_uint8)
        ]

class PktAppInfo(MrfStruct):
    _fields_ = [
        ("name", c_uint8*16),
        ("num_cmds", c_uint8)
    ]
class PktUint8(MrfStruct):
    _fields_ = [
        ("value", c_uint8)
    ]
class PktUint8_2(MrfStruct):
    _fields_ = [
        ("value", c_uint8*2)
    ]

class PktUint16(MrfStruct):
    _fields_ = [
        ("value", c_uint16)
    ]

class PktIfStats(MrfStruct):
    _fields_ = [
        ("rx_pkts", c_uint32),
        ("tx_pkts", c_uint32),
        ("tx_acks", c_uint32),
        ("tx_overruns", c_uint16),
        ("tx_retries", c_uint16),
        ("tx_retried", c_uint16),
        ("tx_errors", c_uint16),
        ("unexp_ack", c_uint16),
        ("rx_ndr", c_uint16),
        ("alloc_err", c_uint8),
        ("st_err", c_uint8)
    ]

class PktPingRes(MrfStruct):
    _fields_ = [
        ("to_rssi", c_uint8),
        ("to_lqi", c_uint8),
        ("from_rssi", c_uint8),
        ("from_lqi", c_uint8)
    ]

class PktNDR(MrfStruct):
    _fields_ = [
        ("ndr_code", c_uint8),
        ("msgid", c_uint8),
        ("hrsc", c_uint8),
        ("hdest", c_uint8)
    ]


# FIXME should be able to generate core command set arg and return templates from C here
mrf_cmd_ack = 0
mrf_cmd_retry = 1
mrf_cmd_resp = 2
mrf_cmd_device_info = 3
mrf_cmd_device_status = 4
mrf_cmd_sys_info = 5
mrf_cmd_if_stats = 6
mrf_cmd_get_time = 7
mrf_cmd_set_time = 8
mrf_cmd_buff_state = 9
mrf_cmd_cmd_info = 10
mrf_cmd_app_info = 11
mrf_cmd_app_cmd_info = 12
mrf_cmd_test_1 = 13
mrf_cmd_usr_struct = 14
mrf_cmd_usr_resp = 15
mrf_cmd_reset = 16
mrf_cmd_ping = 17
mrf_cmd_ndr = 18
MRF_NUM_SYS_CMDS = 19


## some app commands for the time being here.. ideally would be auto discovered codes
MRF_APP_CMD_BASE = 128  # from mrf_sys.h

mrf_cmd_spi_read = 129

MrfSysCmds = {

    mrf_cmd_resp : {
        'name' : "RESPONSE",
        'param': PktResp,
        'resp' : None
    },
    mrf_cmd_device_info :  {
        'name' : "DEVICE_INFO",
        'param': None,
        'resp': PktDeviceInfo
    },
    mrf_cmd_device_status :  {
        'name' : "DEVICE_STATUS",
        'param': None,
        'resp': PktDeviceStatus
    },
    mrf_cmd_sys_info :  {
        'name' : "SYS_INFO",
        'param': None,
        'resp': PktSysInfo
    },
    mrf_cmd_if_stats :  {
        'name' : "IF_STATS",
        'param': PktUint8,
        'resp': PktIfStats
    },
    mrf_cmd_get_time : {
        'name' : "GET_TIME",
        'param': None,
        'resp': PktTimeDate
    },
    mrf_cmd_set_time : {
        'name' : "SET_TIME",
        'param': PktTimeDate,
        'resp': PktTimeDate
    },
    mrf_cmd_cmd_info: {
        'name' : "CMD_INFO",
        'param': PktUint8,
        'resp': PktCmdInfo
    },
    mrf_cmd_app_info :  {
        'name' : "APP_INFO",
        'param': None,
        'resp': PktAppInfo
    },
    mrf_cmd_app_cmd_info: {
        'name' : "APP_CMD_INFO",
        'param': PktUint8,
        'resp': PktCmdInfo
    },
    mrf_cmd_usr_struct : {
        'name' : "USR_STRUCT",
        'param': PktResp,
        'resp' : None
    },
    mrf_cmd_usr_resp : {
        'name' : "USR_RESP",
        'param': PktResp,
        'resp' : None
    },
    mrf_cmd_reset : {
        'name' : "RESET",
        'param': None,
        'resp' : None
    },
    mrf_cmd_ping : {
        'name' : "PING",
        'param': None,
        'resp' : PktPingRes
    },

    mrf_cmd_ndr : {
        'name' : "NDR",
        'param': PktNDR,
        'resp' : None
    }
}





## common funcs


def mrf_decode_buff(rtype,rbytes, cmdset=MrfSysCmds):
    if rtype in cmdset.keys() and cmdset[rtype]['resp']:
        respobj = cmdset[rtype]['resp']()  # create an instance of the mrf_struct object
        #print "mrf_decode_buff got type  %s"%type(respobj)
        #respdat = bytes(resp)[len(hdr)+len(param):len(hdr)+len(param) + len(respobj)]
        respobj.load(rbytes)  ## and load with raw data
        return respobj
    return None
