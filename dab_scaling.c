/*
 * dab_scaling.c
 *
 *  Created on: 18-Jun-2026
 *      Author: Kaus
 *
 *
 * 2026/07/01 - optimization by changing all the division into multiplications
 */

#include "dab_scaling.h"

// Initialize the measurements structure
DAB_Measurements_t dab_meas = {0};

// FORCE THIS FUNCTION TO RUN IN FAST RAM!
#pragma CODE_SECTION(DAB_Scale_Sensors, ".TI.ramfunc");
void DAB_Scale_Sensors(float smooth_V_bus_raw, float smooth_V_bat_raw, DAB_HAL_RawSensors_t* raw_sensors, DAB_Measurements_t* measurements)
{
    // -------------------------------------------------------------
    // 1. VOLTAGE SCALING
    // Formula: V_actual = (V_pin * V_GAIN) + V_OFFSET
    // -------------------------------------------------------------
    float V_bus_pin = smooth_V_bus_raw * ADC_TO_PIN_VOLTS;
    measurements->V_bus_V = (V_bus_pin * V_GAIN_BUS) + V_OFFSET_BUS;

    //measurements->V_bus_V = (smooth_V_bus_raw * V_GAIN_BUS_DIRECT) + V_OFFSET_BUS_DIRECT;


    float V_bat_pin = smooth_V_bat_raw * ADC_TO_PIN_VOLTS;
   measurements->V_bat_V = (V_bat_pin * V_GAIN_BAT) + V_OFFSET_BAT;

    //measurements->V_bat_V = (smooth_V_bat_raw * V_GAIN_BAT_DIRECT) + V_OFFSET_BAT_DIRECT;

    // -------------------------------------------------------------
    // 2. CURRENT SCALING (Replaced Division with Multiplication!)
    // Formula: I_actual = (V_pin * SENSITIVITY_INVERSE) + I_OFFSET
    // -------------------------------------------------------------
    float I_bus_pin = (float)raw_sensors->I_bus_raw * ADC_TO_PIN_VOLTS;
    measurements->I_bus_A = (I_bus_pin * I_BUS_SENSITIVITY_INV) + I_BUS_OFFSET;

    float I_bat_pin = (float)raw_sensors->I_bat_raw * ADC_TO_PIN_VOLTS;
    measurements->I_bat_A = (I_bat_pin * I_BAT_SENSITIVITY_INV) + I_BAT_OFFSET;

    // Calculate Power
    measurements->P_bat_W = measurements->V_bat_V * measurements->I_bat_A;

    // -------------------------------------------------------------
    // 3. PER UNIT CALCULATIONS (Replaced Division with Multiplication!)
    // -------------------------------------------------------------
    measurements->V_bus_pu = measurements->V_bus_V * V_BUS_BASE_INV;
    measurements->V_bat_pu = measurements->V_bat_V * V_BAT_BASE_INV;
    
    measurements->I_bus_pu = measurements->I_bus_A * I_BASE_INV;
    measurements->I_bat_pu = measurements->I_bat_A * I_BASE_INV;

    measurements->P_bat_pu = measurements->P_bat_W * P_BASE_INV;
}
