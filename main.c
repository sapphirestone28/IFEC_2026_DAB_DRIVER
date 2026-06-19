// #############################################################################
//
//  FILE:   empty_driverlib_main.c
//
//! \addtogroup driver_example_list
//! <h1>Empty Project Example</h1>
//!
//! This example is an empty project setup for Driverlib development.
//!
//
// #############################################################################
//
//
// $Copyright:
// Copyright (C) 2025 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the
//   distribution.
//
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
// #############################################################################

//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "c2000ware_libraries.h"
#include "dab_hal.h"
#include "dab_scaling.h"
#include "dab_filter.h"

// Tell main.c that dab_meas exists somewhere else (in dab_scaling.c)
extern DAB_Measurements_t dab_meas;


DAB_HAL_RawSensors_t dab_sensors;


// variable to measeure ISR timings
volatile uint32_t isr_start_time = 0;
volatile uint32_t isr_end_time = 0;
volatile uint32_t isr_total_cycles = 0;


// Variables for the live debugger phase test
volatile float target_phase_shift_pu = 0.0f; // -0.25f to +0.25f

__interrupt void DAB_Fast_ISR(void)
{
    isr_start_time = CPUTimer_getTimerCount(myCPUTIMER0_BASE);
    
    // 1. Read the raw ADC sensor values into the DAB_HAL_RawSensors_t structure
    DAB_HAL_readSensors(&dab_sensors);

    // Filter the values of RAW voltage 
    float smooth_V_bus_raw = DAB_Run_Voltage_Filter(&filt_V_bus, dab_sensors.V_bus_raw);
    float smooth_V_bat_raw = DAB_Run_Voltage_Filter(&filt_V_bat, dab_sensors.V_bat_raw);

    // scaling all the values and calculating the per unit values for the control loop
    DAB_Scale_Sensors(smooth_V_bus_raw, smooth_V_bat_raw, &dab_sensors, &dab_meas);

    // Run the fast IIR filters for currents to reduce noise with minimal delay
    dab_meas.I_bus_pu = DAB_Run_Fast_IIR(&filt_I_bus, dab_meas.I_bus_pu);
    dab_meas.I_bat_pu = DAB_Run_Fast_IIR(&filt_I_bat, dab_meas.I_bat_pu);


    if (ADC_getInterruptOverflowStatus(myADC1_BASE, ADC_INT_NUMBER1))
    {
        ADC_clearInterruptOverflowStatus(myADC1_BASE, ADC_INT_NUMBER1);
        ADC_clearInterruptStatus(myADC1_BASE, ADC_INT_NUMBER1);
    }

    // clearing interuupt flags
    ADC_clearInterruptStatus(myADC1_BASE, ADC_INT_NUMBER1);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);

    isr_end_time = CPUTimer_getTimerCount(myCPUTIMER0_BASE);
    isr_total_cycles = isr_start_time - isr_end_time;
}

void delay_loop(void)
{
    DEVICE_DELAY_US(100000);
}
//
// Main
//
void main(void)
{

    //
    // Initialize device clock and peripherals
    //
    Device_init();

    //
    // Disable pin locks and enable internal pull-ups.
    //
    Device_initGPIO();

    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    //
    // PinMux and Peripheral Initialization
    //
    Board_init();

    //
    // C2000Ware Library initialization
    //
    C2000Ware_libraries_init();

    DAB_HAL_Init();
    DAB_Filter_Init();


    //
    // Enable Global Interrupt (INTM) and real time interrupt (DBGM)
    //
    EINT;
    ERTM;

    while (1)
    {



        // calculate the number of timer ticks corresponding to the target phase shift
        float abs_phase = target_phase_shift_pu;
        int direction = DAB_DIR_CHARGE;

        if (target_phase_shift_pu < 0.0f)
        {
            direction = DAB_DIR_DISCHARGE;
            abs_phase = -target_phase_shift_pu; // make it positive for tick calculation
        }

        // set the count direction based on the sign of the target phase shift
        DAB_HAL_updatePhaseShift(abs_phase, direction);

        GPIO_togglePin(LED_DEBUG);

        // F. Wait 100ms
        delay_loop();
    }
}

//
// End of File
//
