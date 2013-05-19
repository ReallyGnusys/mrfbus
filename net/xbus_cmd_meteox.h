
typedef struct  __attribute__ ((packed))   {
  TIMEDATE td;
  uint16 adc;
  uint16 sens_temp_adc;
  uint16 sens_humid_adc;
  uint16 baro_temp_adc;
  uint16 baro_press_adc;
} XB_MSG_METEOX;

int meteox_rxfunc(void *ptr){
  __delay_cycles(1);
}
const XBCMD meteox = { .name = "METEOX",
		       .size = sizeof(XB_MSG_METEOX),
		       .rxfunc = meteox_rxfunc
};

typedef struct  __attribute__ ((packed))   {
  uint16 a0;
  uint16 b1;
  uint16 b2;
  uint16 c12;
} XB_MSG_METEOX_COEFFS;

const XBCMD meteox_coeffs = { .name = "METEOX_COEFFS",
		       .size = sizeof(XB_MSG_METEOX_COEFFS),
		       .rxfunc = NULL
};
