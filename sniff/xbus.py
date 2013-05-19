from datetime import datetime
import xbus_types
import re
from meteo import *
#convert usb string to array of ints

def UsbBuffToArray(buff):
    bytes = buff.rstrip().split(' ')
    out = []
    cnt = 0
    for hbyte in bytes:
        try:
            out.append(int(hbyte,16))
        except:
            print "failed to convert bytestr %s - cnt %d"%(hbyte,cnt)
        cnt = cnt + 1
    return out


def _type_signed(tname):
        if tname.find('uint') == 0:
            return False
        elif tname.find('int') == 0:
            return True

typere = re.compile('([^\[]+)(\[(\d+)\])?')


def _type_arr(typename):
    mobj = typere.match(typename)
    if not mobj:
        return 0
    if not mobj.group(3):
        return 0
    return int(mobj.group(3))


def _stype_len(typename):
    mobj = typere.match(typename)
    if not mobj:
        return 0

    arr = _type_arr(typename)
    tname = mobj.group(1)
    
    if tname.find('8') == len(tname) -1:
        unit =  1
    elif tname.find('16') == len(tname) -2:
        unit =  2
    elif tname.find('32') == len(tname) -2:
        unit =  4
    else:
        unit = 0

    return unit
def _type_len(typename):
    unit = _stype_len(typename)
    if unit == 0:
        return 0
    arr = _type_arr(typename)
    if arr > 0:
        unit = unit * arr
    return unit

def _makeunsigned(nbytes,sl):
    val = 0
    if nbytes > len(sl):
        print "_makeunsigned error : sl %s"%str(sl)
    for x in range(0,nbytes):
        ind = nbytes - 1 - x;
        val += sl[x] * int(pow(2,8*x))
    return val
                           
def _makesigned(nbytes,sl):
    minusmax = nbytes * pow(2,8)/2;
    val =  _makeunsigned(nbytes,sl);
    if val > minusmax:
        val = -minuxmax + (val - minusmax)
    return val
                                 
def _makeint(signed,nbytes,sl):
        if signed == False:
            return _makeunsigned(nbytes,sl)
        else:
            return _makesigned(nbytes,sl)

def ObjBuffLen(objdef):
    length = 0
    for attr in objdef.keys():
        typename = objdef[attr]
        length += _type_len(typename)
    return length


def intarrtostr(intarr):
    st = ''
    for elem in intarr:
        if elem == 0:
            break 
        st += chr(elem)
    return st

class UsbRxBuffToObj:
    def __init__(self,iarr,objdef):
        index = 0
        for attr in objdef.keys():
            typename = objdef[attr]
            signed =  _type_signed(typename);
            nbytes = _type_len(typename)
            array = _type_arr(typename)

            if array == 0:
                end = index + nbytes
                sl = iarr[index:end]
                val =  _makeint(signed,nbytes,sl)
                setattr(self,attr, val)
                index = end
            else:
                nbytes = _stype_len(typename)
                #print "Making array for %s %s"%(attr,typename)
                setattr(self,attr, []) 
                for i in range(0,array):
                    end = index + nbytes
                    sl = iarr[index:end]
                    val =  _makeint(signed,nbytes,sl)
                    getattr(self,attr).append(val)
                    index = end


class XbLocMsg:
    def __init__(self,buff):
        iarr = UsbBuffToArray(buff)
        tcode = iarr[1] & 0x3f

        iarr = iarr[2:]
        if tcode == xbus_types.XB_PKT_GETTIME:
            self.type = tcode
            objdef =  xbus_types._xb_timedate_def
            xbtd = UsbRxBuffToObj(iarr,objdef)
            self.error = False
            self.datetime = datetime(year=xbtd.year+2000,month=xbtd.mon,day=xbtd.day,
                                 hour=xbtd.hour,minute=xbtd.min,second=xbtd.sec)
        elif tcode == xbus_types.XB_PKT_GETSTATS:
            self.type = tcode
            #print "GETSTATS - buff is "
            #print buff
            objdef =  xbus_types._xb_stats_def
            xobj = UsbRxBuffToObj(iarr,objdef)
            self.error = False
            self.stats = xobj

        elif tcode == xbus_types.XB_PKT_INFO:
            self.type = tcode
            #print "GETSTATS - buff is "
            #print buff
            objdef =  xbus_types._xb_info_def
            xobj = UsbRxBuffToObj(iarr,objdef)
            self.error = False
            self.info = xobj
            self.devicename =  intarrtostr(xobj.devicename)
            self.version =  intarrtostr(xobj.version)


            
        else:
            #print "unsupported tcode %02X"%tcode
            self.error = True
            self.type = 0
    def stats_str(self):
        buff = ""
        for attr in xbus_types._xb_stats_def.keys():
            try:
                buff += "%s : %s "%(attr,str(getattr(self.stats,attr)))
            except:
                print "stats_str error : attr %s  val %s"%\
                      (attr,str(getattr(self.stats,attr)))
        return buff
            
    def __str__(self):
        if self.type == xbus_types.XB_PKT_GETTIME:
            return "xblocal time message "+str(self.datetime)
        elif  self.type == xbus_types.XB_PKT_GETSTATS:
            return self.stats_str()
        elif  self.type == xbus_types.XB_PKT_INFO:
            return " devicename : %s   version : %s"%(self.devicename,self.version)


        else:
            return "no 11"
def adc_to_vcc(adc):
    return 2.0 * 2.5 * adc/4096.0




        

class XbMsg:
    def __init__(self,buff):
        iarr = UsbBuffToArray(buff)
        arrlen = iarr[0]
        self._buff = buff
        self.xbus_dest= iarr[1]
        self.RXRSSI = iarr[len(iarr)-2];
        self.RXLQI = iarr[len(iarr)-1] & 0x7f;
        self.sysdate = datetime.now().date()
        self.systime = datetime.now().time()
        self.vcc = 0.0
        objstart = 2
        iarr = iarr[objstart:]
        objdef = xbus_types._xb_hdr_def
        self.hdr = UsbRxBuffToObj(iarr,objdef)
        self.hdr.type = self.hdr.type & 0x3f
        objstart = ObjBuffLen(objdef)
        self.error = False
        if self.hdr.type < xbus_types.XB_PKT_NORMSTART or self.hdr.type == xbus_types.XB_PKT_GETTIME:
            iarr = iarr[objstart:]
            objdef = xbus_types._xb_timedate_def
            try:
                xbtd = UsbRxBuffToObj(iarr,objdef)
            except:
                print "XbMsg : tdconversion error for %s buff %s"%(iarr,buff)

            try:
                self.datetime = datetime(year=xbtd.year+2000,month=xbtd.mon,day=xbtd.day,
                                 hour=xbtd.hour,minute=xbtd.min,second=xbtd.sec)
                self.date = self.datetime.date()
                self.time = self.datetime.time()
                
            except:
                print "xbmsg td error td = %s"%xbtd.__dict__
                print buff
                self.error = True


        
        objstart = ObjBuffLen(objdef)
        iarr = iarr[objstart:]

        if self.hdr.type == xbus_types.XB_PKT_GETSTATS:
            objdef = xbus_types._xb_stats_def
            stats = UsbRxBuffToObj(iarr,objdef)
            self.stats = stats
        elif self.hdr.type == xbus_types.XB_PKT_TEMP:
            objdef = xbus_types._xb_temp_def
            temp = UsbRxBuffToObj(iarr,objdef)
            self.temp = temp
            self.temperature = float(temp.wholec) + float(temp.fract)/100.0
            self.adc = temp.adc
            self.vcc = adc_to_vcc(self.adc)
        elif self.hdr.type == xbus_types.XB_PKT_METEO1:
            objdef = xbus_types._xb_meteo1_def
            self.meteo1 = UsbRxBuffToObj(iarr,objdef)
            #self.temp = temp
            #self.temperature = float(temp.wholec) + float(temp.fract)/100.0
            self.adc = self.meteo1.adc
            self.vcc = adc_to_vcc(self.adc)
            self.kpa = mpa_kpa(self.meteo1.baro_press_adc,self.meteo1.baro_temp_adc)
            self.temperature = sht_temp(self.meteo1.sens_temp_adc)
            self.rh = sht_rh(self.meteo1.sens_humid_adc)
            #print "XBMSG temp: adc %X vcc %f"%(self.adc,self.vcc)
        elif self.hdr.type == xbus_types.XB_PKT_METEO1_COEFFS:
            objdef = xbus_types._xb_meteo1_coeffs_def
            meteo1_coeffs = UsbRxBuffToObj(iarr,objdef)
            #self.temp = temp
            #self.temperature = float(temp.wholec) + float(temp.fract)/100.0
            self.meteo1_coeffs = meteo1_coeffs
            self.a0_raw = meteo1_coeffs.a0
            self.b1_raw = meteo1_coeffs.b1
            self.b2_raw = meteo1_coeffs.b2
            self.c12_raw = meteo1_coeffs.c12
            
            self.a0 = coeff_a0(meteo1_coeffs.a0)
            self.b1 = coeff_b1(meteo1_coeffs.b1)
            self.b2 = coeff_b2(meteo1_coeffs.b2)
            self.c12 = coeff_c12(meteo1_coeffs.c12)
        elif self.hdr.type == xbus_types.XB_PKT_DBG_UINT8:
            objdef = xbus_types._xb_dbg_uint8_def
            ob = UsbRxBuffToObj(iarr,objdef)
            self.data = ob.data
        elif self.hdr.type == xbus_types.XB_PKT_DBG_CHR32:
            objdef = xbus_types._xb_dbg_chr32_def
            ob = UsbRxBuffToObj(iarr,objdef)
            self.data = intarrtostr(ob.data)         
          
        elif self.hdr.type == xbus_types.XB_PKT_ADC2:
            objdef = xbus_types._xb_adc_def
            ob = UsbRxBuffToObj(iarr,objdef)
            self.adc = ob.adc
            self.vcc = adc_to_vcc(self.adc)
            
        elif self.hdr.type == xbus_types.XB_PKT_INFO:
            objdef = xbus_types._xb_info_def
            ob = UsbRxBuffToObj(iarr,objdef)
            self.devicename = intarrtostr(ob.devicename)
            self.version  = intarrtostr(ob.version)
        
    def stats_str(self):
        buff = ""
        for attr in xbus_types._xb_stats_def.keys():
            try:
                buff +="%s : %s "%(attr,str(getattr(self.stats,attr)))
            except:
                print "stats_str error : attr %s  val %s"%\
                      (attr,str(getattr(self.stats,attr)))
        return buff
    def lqi_str(self):
        return " RSSI:%02X LQI:%02X "%(self.RXRSSI,self.RXLQI)
    def info_str(self):
        return "devname : %s  version : %s"%(self.devicename,self.version)
    def temp_str(self):
        return " %d.%02d C  vcc : %.2f"%(self.temp.wholec,self.temp.fract,self.vcc)
    def meteo1_str(self):
        return " %.2f C ,  %.2f %% RH , Pressure %.2f kPa , vcc %.2f "%(self.temperature,self.rh,self.kpa,self.vcc)
    def dbg_uint8_str(self):
        str1 = "data "
        for i in range(0,8):
            str1 += "%04X "%self.data[i]
        return str1
    def dbg_chr32_str(self):
        return str(self.data)
        str1 = ""
        for item in self.data:
            str1 += chr(item)
        return str1
     
    def meteo1_coeffs_str(self):
        return "a0 %X b1 %X b2 %X c12 %X"%(self.a0_raw,
                                           self.b1_raw,
                                           self.b2_raw,
                                           self.c12_raw)
    def _type_str(self):
        if self.hdr.type ==  xbus_types.XB_PKT_TEMP:
            return "PKT_TEMP "+self.temp_str()
        elif self.hdr.type ==  xbus_types.XB_PKT_METEO1:
            return "PKT_METEO1 "+self.meteo1_str()
        elif self.hdr.type ==  xbus_types.XB_PKT_METEO1_COEFFS:
            return "PKT_METEO1_COEFFS "+self.meteo1_coeffs_str()
        elif self.hdr.type ==  xbus_types.XB_PKT_DBG_UINT8:
            return "XB_PKT_DBG_UINT8 "+self.dbg_uint8_str()
        elif self.hdr.type ==  xbus_types.XB_PKT_DBG_CHR32:
            return "XB_PKT_DBG_CHR32 "+self.dbg_chr32_str()
        else:
            return "msg type:%2X"%self.hdr.type
    def head_str(self):
        return  "%s dev:%2X msgid:%02X %s "%(self._type_str(),self.hdr.source,self.hdr.msgid,self.lqi_str())
    def __str__(self):
        if self.hdr.type == xbus_types.XB_PKT_GETTIME:
            try:
                return self.head_str()+str(self.datetime)
            except:
                return "xb str error time buff is "+self._buff+\
                       "\n"+str(self.__dict__)+\
                       "\n"+str(self.hdr.__dict__)
        elif  self.hdr.type == xbus_types.XB_PKT_GETSTATS:            
            return self.stats_str()
            #return "xb stats "+str(self.stats.__dict__)        
        elif  self.hdr.type == xbus_types.XB_PKT_INFO:            
            return self.head_str()+" "+self.info_str()
            #return "xb stats "+str(self.stats.__dict__)        
        elif self.hdr.type < xbus_types.XB_PKT_NORMSTART:
            rstr = self.head_str()
            try:
                rstr = rstr + str(self.datetime)
            except:
                return "XbMsg.str error - no datetime"
            
            return rstr
        else:
            return self.head_str()

        
'''
print xbus_types._xb_hdr_def

st1 = '11 01 19 00 02 10 00 0B 06 31 00 0F 0D 11 08 0C 3C 00'

obj = XbMsg(st1)

print obj
print obj.__dict__
'''

'''

iarr = UsbBuffToArray(st1)
print iarr
arrlen = iarr[0]
dest = iarr[1]
print "dest is %d"%dest
iarr = iarr[2:]
print iarr
IArrToObj(iarr,xbus_types._xb_hdr_def)
'''
