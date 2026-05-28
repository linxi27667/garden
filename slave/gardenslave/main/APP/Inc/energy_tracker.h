#ifndef ENERGY_TRACKER_H
#define ENERGY_TRACKER_H

#include <stdbool.h>
#include <stdint.h>
#include "mqtt_iot_protocol.h"

#define ENERGY_NVS_NAMESPACE     "energy"
#define ENERGY_NVS_KEY           "accum"
#define ENERGY_SAVE_INTERVAL_MS  30000    // 30 seconds
#define ENERGY_REPORT_INTERVAL_S 10
#define ENERGY_DEMO_SPEEDUP      60     // 2 hours

typedef struct {
    float kwh[ENERGY_DEV_COUNT];
} energy_accum_t;

void Energy_Tracker_Init(void);
void Energy_Tracker_Update(bool device_on[ENERGY_DEV_COUNT]);
float Energy_Tracker_Get_Total(void);
const energy_accum_t* Energy_Tracker_Get_Data(void);
void Energy_Tracker_Report_Now(void);

#endif
