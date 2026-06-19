/*
 * dab_hal.c
 *
 *  Created on: 16-Jun-2026
 *      Author: Kaus
 */

#include "dab_hal.h"

#define PERIOD_TICKS 240.0f
#define FULL_CYCLE_TICKS (2.0f * PERIOD_TICKS)   // = 480

void DAB_HAL_Init(void)
{
    DAB_HAL_disablePWM();
}

void DAB_HAL_readSensors(DAB_HAL_RawSensors_t *raw_sensors)
{

    // 1. Read the standard 0-4095 voltage results
    raw_sensors->V_bus_raw = ADC_readResult(myADC1_RESULT_BASE, ADC_SOC_NUMBER0);
    raw_sensors->V_bat_raw = ADC_readResult(myADC1_RESULT_BASE, ADC_SOC_NUMBER1);

    // 2. Read the PPB Bipolar Current results (-2048 to +2047)
    raw_sensors->I_bus_raw = ADC_readPPBResult(myADC2_RESULT_BASE, myADC2_PPB1);
    raw_sensors->I_bat_raw = ADC_readPPBResult(myADC2_RESULT_BASE, myADC2_PPB2);
}

void DAB_HAL_updatePhaseShift(float phase_pu, int direction)
{
    // 1. Clamp to safe limits
    // DAB power transfer peaks at 90 degrees (0.25 PU).
    if (phase_pu > 0.25f)
        phase_pu = 0.25f;
    if (phase_pu < 0.f)
        phase_pu = 0.0f;

    // 2. Calculate absolute phase shift in ticks
    // In Up-Down mode, full phase cycle (360 deg) is 2 * PRD (480 ticks)
    // 0.25 PU (90 deg) * 480 = 120 ticks.
    uint16_t phase_ticks = (uint16_t)(phase_pu * FULL_CYCLE_TICKS);

    // 3. Determine Count Direction based on Lead/Lag Physics
    EPWM_SyncCountMode count_dir;

    if (direction == DAB_DIR_CHARGE)
    {
        // G2V (Charge): Power flows Primary -> Secondary.
        // Therefore, Secondary must LAG.
        // Counting DOWN delays the slave timer, creating a LAG.
        count_dir = EPWM_COUNT_MODE_DOWN_AFTER_SYNC;
    }
    else // DAB_DIR_DISCHARGE
    {
        // V2G (Discharge): Power flows Secondary -> Primary.
        // Therefore, Secondary must LEAD.
        // Counting UP gives the slave timer a head-start, creating a LEAD.
        count_dir = EPWM_COUNT_MODE_UP_AFTER_SYNC;
    }

    // 4. WRITE TO SECONDARY BRIDGE MODULES ONLY!
    // EPWM1 and EPWM2 shift.
    // EPWM4 (Master), EPWM6, and EPWM7 (Primary) remain locked at Phase 0.

    EPWM_setPhaseShift(myEPWM1_BASE, phase_ticks);
    EPWM_setCountModeAfterSync(myEPWM1_BASE, count_dir);

    EPWM_setPhaseShift(myEPWM2_BASE, phase_ticks);
    EPWM_setCountModeAfterSync(myEPWM2_BASE, count_dir);
}


void DAB_HAL_enablePWM(void)
{
    // Write HIGH to the Gate Driver Enable GPIO
    GPIO_writePin(GD_EN_1, 1);
    GPIO_writePin(GD_EN_2, 1);
    GPIO_writePin(GD_EN_3, 1);
    GPIO_writePin(GD_EN_4, 1);
}


void DAB_HAL_disablePWM(void)
{
    // Write LOW to the Gate Driver Enable GPIO
    GPIO_writePin(GD_EN_1, 0);
    GPIO_writePin(GD_EN_2, 0);
    GPIO_writePin(GD_EN_3, 0);
    GPIO_writePin(GD_EN_4, 0);
}


void DAB_HAL_clearHardwareTrips(void)
{
    // Clears the OST latch so PWMs can resume after a fault is removed
    uint16_t clear_flags = EPWM_TZ_INTERRUPT_OST | EPWM_TZ_INTERRUPT_DCAEVT1 | EPWM_TZ_INTERRUPT_DCBEVT1;

    EPWM_clearTripZoneFlag(myEPWM1_BASE, clear_flags);
    EPWM_clearTripZoneFlag(myEPWM2_BASE, clear_flags);
    EPWM_clearTripZoneFlag(myEPWM4_BASE, clear_flags);
    EPWM_clearTripZoneFlag(myEPWM6_BASE, clear_flags);
    EPWM_clearTripZoneFlag(myEPWM7_BASE, clear_flags);
}

