
typedef struct  __attribute__ ((packed))   {
  int8 wholec;
  uint8 fract; // 0-99 = 0.0 - 0.99
  uint16 adc;
}XB_PKT_MODTC;

typedef struct  __attribute__ ((packed))   {
  uint16 adc;
}XB_PKT_ADC;



typedef struct  __attribute__ ((packed))   {
  TIMEDATE td;
  uint16 adc;
  uint16 sens_temp_adc;
  uint16 sens_humid_adc;
  uint16 baro_temp_adc;
  uint16 baro_press_adc;
} XB_PKT_METEO1;



typedef struct  __attribute__ ((packed))   {
  uint16 a0;
  uint16 b1;
  uint16 b2;
  uint16 c12;
} XB_PKT_METEO1_COEFFS;
