from ctypes import *


class MrfStruct(LittleEndianStructure):
    def fromstr(self, string):
        memmove(addressof(self), string, sizeof(self))
    def __len__(self):
        return sizeof(self)

    def __repr__(self):
        s = ''
        for field in self._fields_:
            s += "%s %s\n"%(field[0],str(getattr(self,field[0])))
        return s
    def load(self, bytes):
        fit = min(len(bytes), sizeof(self))
        memmove(addressof(self), bytes, fit)
    def dump(self):
        return buffer(self)[:]

MRF_CMD_CODE = {
    0 : {
        'name' : "ACK",
        'param': None,
        'resp' : None
    }
}
    
    


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


mrf_cmd_ack = 0,
mrf_cmd_retry = 1,
mrf_cmd_resp = 2,
mrf_cmd_device_info = 3,
mrf_cmd_device_status = 4,
mrf_cmd_sys_info = 5,
mrf_cmd_if_stats = 6,
mrf_cmd_get_time = 7,
mrf_cmd_set_time = 8,
mrf_cmd_buff_state = 9,
mrf_cmd_cmd_info = 10,
mrf_cmd_app_info = 11,
mrf_cmd_app_cmd_info = 12,
mrf_cmd_test_1 = 13, 
mrf_cmd_test_2 = 14,
mrf_cmd_usr_resp = 15,
MRF_NUM_SYS_CMDS = 16

