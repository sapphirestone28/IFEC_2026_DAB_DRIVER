/*
 * dab_scaling.h
 *
 *  Created on: 18-Jun-2026
 *      Author: Kaus
 *
 * 2026/07/01 - optimization by changing all the division into multiplications
 */

#ifndef DAB_SCALING_H_
#define DAB_SCALING_H_

#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "dab_hal.h"

// ---------------------------------------------------------
// INVERTED PER-UNIT BASES (Multiplication is 40x faster than Division)
// ---------------------------------------------------------
#define V_BUS_BASE_INV   0.0025f    // 1.0f / 400.0f
#define V_BAT_BASE_INV   0.0025f    // 1.0f / 400.0f
#define I_BASE_INV       0.1f       // 1.0f / 10.0f
#define P_BASE_INV       0.001f     // 1.0f / 1000.0f

// Calibration constants for ADC conversion
#define ADC_TO_PIN_VOLTS (3.3f / 4096.0f) // 12-bit ADC with 3.3V reference

// Voltage Calibration
#define V_GAIN_BAT           174.587f
#define V_OFFSET_BAT         1.0f
#define V_GAIN_BUS           174.587f
#define V_OFFSET_BUS         -1.0f


// Voltage Calibration
#define V_GAIN_BAT_DIRECT     0.200009f
#define V_OFFSET_BAT_DIRECT   59.f

#define V_GAIN_BUS_DIRECT        0.5844f
#define V_OFFSET_BUS_DIRECT     -508.0f

// ---------------------------------------------------------
// INVERTED CURRENT SENSITIVITIES (1.0f / Sensitivity)
// ---------------------------------------------------------
// Original I_BUS: 0.0331 V/A -> Inverse is 30.21148 A/V
#define I_BUS_SENSITIVITY_INV   20.41f

#define I_BAT_SENSITIVITY_INV   19.54f

// Fine-tuning software offsets
#define I_BUS_OFFSET         -0.040f
#define I_BAT_OFFSET         -0.090f

typedef struct
{
    // Physical values
    float V_bus_V;
    float V_bat_V;
    float I_bus_A;
    float I_bat_A;
    float P_bat_W;

    // Per unit values for PI Control and math 
    float V_bus_pu;
    float V_bat_pu;
    float I_bus_pu;
    float I_bat_pu;
    float P_bat_pu;
} DAB_Measurements_t;

// Function Prototype
void DAB_Scale_Sensors(float smooth_V_bus_raw, float smooth_V_bat_raw, DAB_HAL_RawSensors_t* raw_sensors, DAB_Measurements_t* measurements);

#endif /* DAB_SCALING_H_ */
