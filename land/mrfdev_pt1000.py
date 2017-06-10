
from mrf_sens import MrfSens, MrfDev
from datetime import datetime
import ctypes
from mrf_structs import *
from core_tests import mrf_cmd_app_test
from math import sqrt



class PktSpiDebug(MrfStruct):
    _fields_ = [
        ("spi_rx_int_cnt", c_uint16),
        ("spi_tx_int_cnt", c_uint16),
        ("spi_rx_bytes", c_uint16),
        ("spi_tx_bytes", c_uint16),
        ("spi_rxov", c_uint16),
        ("spi_rx_queue_level", c_uint16),
        ("spi_tx_queue_level", c_uint16),
        ("spi_rx_queue_data_avail", c_uint8),
        ("spi_tx_queue_data_avail", c_uint8),
        ("spi_rxq_qip", c_uint8),
        ("spi_rxq_qop", c_uint8),
        ("spi_rxq_items", c_uint8),
        ("spi_rxq_push_errors", c_uint8),
        ("spi_rxq_pop_errors", c_uint8),
        ("spi_txq_qip", c_uint8),
        ("spi_txq_qop", c_uint8),
        ("spi_txq_items", c_uint8),
        ("spi_txq_push_errors", c_uint8),
        ("spi_txq_pop_errors", c_uint8),
        ("ucb0_ifg", c_uint8),
        ("ucb0_ie", c_uint8),
        ("ucb0_cntrl0", c_uint8),
        ("ucb0_cntrl1", c_uint8),
        ("ucb0_stat", c_uint8),

       
    ]

MAX_RTDS = 7

class PktPt1000State(MrfStruct):
    _fields_ = [
        ("td",PktTimeDate),
        ("relay_cmd", c_uint8),
        ("relay_state", c_uint8),
        ("milliohms", c_uint32*MAX_RTDS),
        ("ref_r",c_uint32),
        ("ref_i",c_uint32),
        ]

class PktRelayState(MrfStruct):
    _fields_ = [
        ("chan", c_uint8),
        ("val", c_uint8),
        ]

    

mrf_app_cmd_test = 128
mrf_cmd_spi_read = 129
mrf_cmd_spi_write = 130
mrf_cmd_spi_debug = 131
mrf_cmd_spi_data  = 132
mrf_cmd_config_adc  = 133
mrf_cmd_read_state  = 134
mrf_cmd_get_relay   = 135
mrf_cmd_set_relay   = 136


Pt1000AppCmds = {
    mrf_cmd_app_test : {
        'name'  : "APP_TEST",
        'param' : None,
        'resp'  : PktTimeDate
    },
    mrf_cmd_spi_read : {
        
        'name'  : "SPI_READ",
        'param' : PktUint8,
        'resp'  : PktUint8
    },
    mrf_cmd_spi_write : {
        
        'name'  : "SPI_WRITE",
        'param' : PktUint8_2,
        'resp'  : None
    },
    mrf_cmd_spi_debug : {
        
        'name'  : "SPI_DEBUG",
        'param' : None,
        'resp'  : PktSpiDebug
    },
    mrf_cmd_spi_data : {
        'name'  : "SPI_DATA",
        'param' : None,
        'resp'  : PktUint16
    },
    mrf_cmd_config_adc : {
        'name'  : "CONFIG_ADC",
        'param' : None,
        'resp'  : None
    },
    mrf_cmd_read_state : {
        'name'  : "READ_STATE",
        'param' : None,
        'resp'  : PktPt1000State
    },
    mrf_cmd_get_relay : {
        'name'  : "GET_RELAY",
        'param' : PktRelayState,
        'resp'  : PktRelayState
    },
    mrf_cmd_set_relay : {
        'name'  : "SET_RELAY",
        'param' : PktRelayState,
        'resp'  : PktRelayState
    },

}





class MrfSensPt1000(MrfSens):
    _in_flds_ = [ ('date', PktTimeDate) ,
                  ('milliohms' , long) ]  # hmpff
    
    _out_flds_ = [ ('send_date' , datetime.now ),
                   ('recd_date' , datetime.now),
                   ('milliohms' , int ),
                   ('temp'      , float) ]

    def res_to_temp(self,milliohms):
        R = milliohms/1000.0

        if R > 2000.0:
            return 9999.9
        A=3.9083e-3
        B=-5.775e-7
        R0 = 1000.0
        R=R/R0
        T=0.0-A
        tmp = (A*A) - 4.0 * B * (1.0 - R)
        #try:
        T = T +  sqrt(tmp)
        T = T/ (2.0 * B)
        #except:
        #    self.log.error("res_to_temp error chan %d  with milliohms %d tmp %f"%(self.channel,milliohms,tmp))
        #    T = -273.16
        return T

    
    def genout(self,indata,outdata):
        self.log.info("%s input got type %s data %s"%(self.__class__.__name__, type(indata), indata))
        outdata['send_date'] = indata['date'].to_datetime()
        outdata['recd_date'] = datetime.now()
        outdata['milliohms']  = int(indata['milliohms'])
        outdata['temp']  = self.res_to_temp(outdata['milliohms'])
        self.log.info("%s gend output type %s data %s"%(self.__class__.__name__, type(outdata), outdata))
        return outdata
        


    
        
class Pt1000Dev(MrfDev):

    _capspec = { 'temp' : MrfSensPt1000 }                 
    _cmdset = Pt1000AppCmds

    def app_packet(self, hdr, param , resp):
        self.log.warn("%s app_packet type %s"%(self.__class__.__name__, type(resp)))
        
        self.log.warn("hdr %s param %s resp %s"%(repr(hdr), repr(param), repr(resp)))

        if param.type == mrf_cmd_read_state:

            for ch in range(len(resp.milliohms)):
                self.log.warn("chan %s milliohms %d type %s"%(ch, resp.milliohms[ch], type(resp.milliohms[ch])))
                inp = { 'date' : resp.td,
                        'milliohms' : resp.milliohms[ch]
                }
                self.caps['temp'][ch].input(inp)
