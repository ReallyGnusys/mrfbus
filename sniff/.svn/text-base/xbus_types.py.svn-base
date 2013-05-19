from collections import OrderedDict

XB_PKT_SETTIME = 1
XB_PKT_TEMP = 2
XB_PKT_METEO1 = 5
XB_PKT_GETTIME = 0xB
XB_PKT_GETSTATS = 0x9
XB_PKT_PING = 0xa
XB_PKT_STAYAWAKE = 0x16
XB_PKT_SLEEP = 0x17
XB_PKT_METEO1_COEFFS = 0x18
XB_PKT_DBG_UINT8  = 0x19
XB_PKT_DBG_CHR32 = 0x1a  

XB_HUB_ADDR = 0x01
XB_PKT_NORMSTART  =  0x8
XB_PKT_SENDADC2 = 0xf   
XB_PKT_ADC2 = 0x10   
XB_PKT_INFO = 0x11

XB_MAXPKTLEN = 64

_xb_hdr_def = OrderedDict([\
    ('netid', 'uint8'),
    ('dest', 'uint8'),
    ('type', 'uint8'),
    ('source', 'uint8'),
    ('sender', 'uint8'),
    ('msgid', 'uint8'),
    ('rssi', 'uint8'),
    ('lqi', 'uint8')])


_xb_timedate_def = OrderedDict([\
    ('sec', 'uint8'),
    ('min', 'uint8'),
    ('hour', 'uint8'),
    ('day', 'uint8'),
    ('mon', 'uint8'),
    ('year', 'uint8')])

_xb_stats_def = OrderedDict([\
    ('rx_pkts', 'uint32'),
    ('tx_acks', 'uint32'),
    ('tx_pkts', 'uint32'),
    ('rx_crc_errs', 'uint16'),
    ('rx_net_errs', 'uint16'),
    ('tx_fails', 'uint16'),
    ('tx_retries', 'uint16'),
    ('tx_no_eop', 'uint16'),
    ('rx_overruns', 'uint16'),
    ('rx_unexp_ack', 'uint16'),
    ('tick_err', 'uint16'),
    ('fatal', 'uint16'),
    ('txn', 'uint16[4]')])

_xb_temp_def = OrderedDict([\
    ('wholec', 'uint8'),
    ('fract', 'uint8'),
    ('adc', 'uint16')])

_xb_adc_def = OrderedDict([\
    ('adc', 'uint16')])


_xb_info_def = OrderedDict([\
    ('devicename', 'uint8[10]'),
    ('version', 'uint8[10]')])

_xb_meteo1_coeffs_def = OrderedDict([\
    ('a0', 'uint16'),
    ('b1', 'uint16'),
    ('b2', 'uint16'),
    ('c12', 'uint16')])

_xb_meteo1_def = OrderedDict([\
    ('adc', 'uint16'),
    ('sens_temp_adc', 'uint16'),
    ('sens_humid_adc', 'uint16'),
    ('baro_temp_adc', 'uint16'),
    ('baro_press_adc', 'uint16')])

_xb_dbg_uint8_def = OrderedDict([\
    ('data', 'uint16[8]')])

_xb_dbg_chr32_def = OrderedDict([\
    ('data', 'uint8[32]')])
