/*
 * dab_hal.h
 *
 *  Created on: 16-Jun-2026
 *      Author: Kaus
 */

#ifndef DAB_HAL_H_
#define DAB_HAL_H_

#include "driverlib.h"
#include "device.h"
#include "board.h"

// definations 

#define DAB_DIR_CHARGE       1
#define DAB_DIR_DISCHARGE   -1


// Data structure to hold the ADC measurements
typedef struct
{
    uint16_t V_bus_raw;
    uint16_t V_bat_raw;
    int16_t I_bus_raw;
    int16_t I_bat_raw;
} DAB_HAL_RawSensors_t;

// function Prototypes 

void DAB_HAL_Init(void);
void DAB_HAL_enablePWM(void);
void DAB_HAL_disablePWM(void);
void DAB_HAL_clearHardwareTrips(void);

// ADC read 
void DAB_HAL_readSensors(DAB_HAL_RawSensors_t* raw_sensors);


void DAB_HAL_updatePhaseShift(float phase_pu, int direction);



#endif /* DAB_HAL_H_ */
