#ifndef __MRF_RELAYS_INCLUDED__
#define __MRF_RELAYS_INCLUDED__

uint8 set_relay_state(uint8 chan, uint8 val);
uint8 get_relay_state(uint8 chan);
uint8 relay_state();
void clear_relay_state();
void init_relays();

#endif
