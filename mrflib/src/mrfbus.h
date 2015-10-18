#include "mrf_types.h"
#include "mrf_common_types.h"
#include "xb_cmd_def.h"
#include "xb_sys_structs.h"

void xbus_wakedevices();

uint8 xbus_can_sleep();

void xb_sleep_device(uint8 device);
void xb_wake_device(uint8 device);

void xbus_start_tick();
int xbus_is_idle();
void xbus_clear_stats();
