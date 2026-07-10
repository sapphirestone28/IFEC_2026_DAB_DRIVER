#ifndef DAB_TELEMETRY_H_
#define DAB_TELEMETRY_H_

#include "driverlib.h"
#include "board.h"
#include "dab_hal.h"
#include "dab_scaling.h"

// Initialize the SCI and RS485 pins
void DAB_Telemetry_Init(void);

// Transmits RAW counts and PROCESSED floats in pure binary
void DAB_Telemetry_Transmit_Binary(DAB_HAL_RawSensors_t *raw, DAB_Measurements_t *meas, float phase_pu, int16_t mode);

#endif /* DAB_TELEMETRY_H_ */
