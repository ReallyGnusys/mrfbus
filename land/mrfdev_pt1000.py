#!/usr/bin/env python
'''  Copyright (c) 2012-17 Gnusys Ltd

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

from mrf_sens import MrfSens
from mrf_dev  import MrfDev
from mrf_sens_relay import MrfSensRelay
import datetime
import ctypes
from mrf_structs import *
from core_tests import mrf_cmd_app_test
from math import sqrt
from mrflog import mrflog
from collections import deque


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
        ("spi_rxq_push_errors", c_uint8),
        ("spi_rxq_pop_errors", c_uint8),
        ("spi_txq_qip", c_uint8),
        ("spi_txq_qop", c_uint8),
        ("spi_txq_push_errors", c_uint8),
        ("spi_txq_pop_errors", c_uint8),
        ("cyc_cnt",  c_uint32),
        ("cyc_err1", c_uint32),
        ("cyc_err2", c_uint32),
        ("cyc_err3", c_uint32),
        ("chan_err_last", c_uint8)

    ]

MAX_RTDS = 7

class PktPt1000State(MrfStruct):
    _fields_ = [
        ("td",PktTimeDate),
        ("relay_cmd", c_uint8),
        ("relay_state", c_uint8),
        ("milliohms", c_uint32*MAX_RTDS),
        ("ref_r",c_uint32),
        ("ref_i",c_uint32)
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
mrf_cmd_sample_start   = 137
mrf_cmd_sample_stop   = 138


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
    mrf_cmd_sample_start : {
        'name'  : "SAMPLE_START",
        'param' : None,
        'resp'  : None
    },
    mrf_cmd_sample_stop : {
        'name'  : "SAMPLE_STOP",
        'param' : None,
        'resp'  : None
    },

}





class MrfSensPt1000(MrfSens):
    _in_flds_ = [ ('date', PktTimeDate) ,
                  ('milliohms' , int) ]  # hmpff

    _out_flds_ = [ ('send_date' , datetime.datetime.now ),
                   ('recd_date' , datetime.datetime.now),
                   ('milliohms' , int ),
                   ('temp'      , float) ]

    _history_ =  { 'fields' : ['temp']
                 }

    _stype_    =  'temp'
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
        #    mrflog.error("res_to_temp error chan %d  with milliohms %d tmp %f"%(self.channel,milliohms,tmp))
        #    T = -273.16
        T = (int)(T*10)   # only 1 DP makes sense
        T = T/10.0
        return T


    def filter(self, ntaps):
        self.ftaps = deque()

        for i in range(ntaps):
            self.ftaps.append(0)

    def filter_out(self,newval): # cheap and nasty filter option
        self.ftaps.popleft()
        self.ftaps.append(newval)
        tot = 0
        for v in self.ftaps:
            tot += v
        mo = 1.0 * tot / len(self.ftaps)
        return int(mo)

    def genout(self,indata):

        outdata = dict()
        #mrflog.info("%s input got type %s data %s"%(self.__class__.__name__, type(indata), indata))
        outdata['send_date'] = indata['date'].to_datetime()
        outdata['recd_date'] = datetime.datetime.now()
        if not hasattr(self,'ftaps'):
            outdata['milliohms']  = int(indata['milliohms'])
        else:
            outdata['milliohms']  = self.filter_out(int(indata['milliohms']))

        outdata['temp']  = self.res_to_temp(outdata['milliohms'])

        if (outdata['temp'] < -40.0)  or (outdata['temp'] > 140.0) :
            mrflog.error("%s  %s invalid temp %.2f, discarding"%( self.__class__.__name__, self.label,  outdata['temp']))
            return None

        if hasattr(self,'last_temp')  and (abs(self.last_temp - outdata['temp'])) > 20.0:  # no way should shift 20C in one reading
            mrflog.error("%s  %s  temp swing beyond limit got  %.2f  last was %.2f , discarding"%( self.__class__.__name__, self.label,  outdata['temp'],self.last_temp))
            return None
        #mrflog.info("%s gend output type %s data %s"%(self.__class__.__name__, type(outdata), outdata))
        self.last_temp = outdata['temp']
        return outdata



class Pt1000Dev(MrfDev):

    _capspec = {
        'temp'  : MrfSensPt1000,
        'relay' : MrfSensRelay}
    _cmdset = Pt1000AppCmds

    def init(self):
        self.filt_ind = 0
    def app_packet(self, hdr, param , resp):
        mrflog.info("%s app_packet type %s"%(self.__class__.__name__, type(resp)))

        mrflog.info("Pt1000Dev app_packet, hdr %s param %s resp %s"%(repr(hdr), repr(param), repr(resp)))

        if param.type == mrf_cmd_read_state:

            for ch in range(len(resp.milliohms)):
                #mrflog.debug("chan %s milliohms %d type %s"%(ch, resp.milliohms[ch], type(resp.milliohms[ch])))
                inp = { 'date' : resp.td,
                        'milliohms' : resp.milliohms[ch]
                }
                self.caps['temp'][ch].input(inp)


            for ch in range(len(self.caps['relay'])):
                #(resp.relay_state >> ch) & 0x01
                inp = { 'date' : resp.td,
                        'relay' :  (resp.relay_state >> ch) & 0x01
                }
                self.caps['relay'][ch].input(inp)


        elif  param.type == mrf_cmd_set_relay or param.type == mrf_cmd_get_relay:
            now = datetime.datetime.now()
            td =  PktTimeDate()
            td.set(now)
            inp = { 'date' : td,
                    'relay' :  resp.val
            }
            self.caps['relay'][resp.chan].input(inp)
