// #############################################################################
//  FILE:   main.c
//  TITLE:  DAB Open-Loop Hardware Test Bench
// #############################################################################

#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "c2000ware_libraries.h"

#include "dab_hal.h"
#include "dab_scaling.h"
#include "dab_filter.h"

// ---------------------------------------------------------
// GLOBALS
// ---------------------------------------------------------
DAB_HAL_RawSensors_t dab_sensors;
extern DAB_Measurements_t dab_meas;

volatile uint32_t isr_start_time = 0;
volatile uint32_t isr_end_time   = 0;
volatile uint32_t isr_total_cycles = 0;

// ---------------------------------------------------------
// HARDWARE TEST VARIABLES (Edit these in CCS Expressions!)
// ---------------------------------------------------------
volatile float test_phase_pu = 0.0f;        // Change this between 0.0 and 0.25!
volatile int   test_direction = 1;          // 1 = Charge (G2V), -1 = Discharge (V2G)
volatile int   clear_hardware_faults = 0;   // Set to 1 to clear trips
volatile int   enable_gate_drivers = 1;     // Set to 1 to physically turn on the GD_EN pins

// ---------------------------------------------------------
// THE 50 kHz FAST ISR
// ---------------------------------------------------------
__interrupt void DAB_Fast_ISR(void)
{
    isr_start_time = CPUTimer_getTimerCount(myCPUTIMER0_BASE);

    // SOFTWARE "AND" GUARD: Wait for ADCA to finish
    uint16_t guard = 0;
    while (ADC_getInterruptStatus(myADC1_BASE, ADC_INT_NUMBER1) == false) {
        if (++guard > 1000U) break;
    }

    // 1. Read Raw Sensors
    DAB_HAL_readSensors(&dab_sensors);

    // 2. Filter Voltages
    float smooth_V_bus_raw = DAB_Run_Voltage_Filter(&filt_V_bus, dab_sensors.V_bus_raw);
    float smooth_V_bat_raw = DAB_Run_Voltage_Filter(&filt_V_bat, dab_sensors.V_bat_raw);

    // 3. Scale and populate dab_meas
    DAB_Scale_Sensors(smooth_V_bus_raw, smooth_V_bat_raw, &dab_sensors, &dab_meas);

    // 4. Filter Currents
    dab_meas.I_bus_A  = DAB_Run_Fast_IIR(&filt_I_bus, dab_meas.I_bus_A);
    dab_meas.I_bat_A  = DAB_Run_Fast_IIR(&filt_I_bat, dab_meas.I_bat_A);

    // Refresh PUs based on filtered current
    dab_meas.I_bus_pu = dab_meas.I_bus_A / I_BASE;
    dab_meas.I_bat_pu = dab_meas.I_bat_A / I_BASE;
    dab_meas.P_bat_W  = dab_meas.V_bat_V * dab_meas.I_bat_A;
    dab_meas.P_bat_pu = dab_meas.P_bat_W / P_BASE;

    // 5. OPEN-LOOP TEST: Bypass control math, apply test variables directly!
    // Safety clamp just in case you type a crazy number in the debugger
    if(test_phase_pu > 0.25f) test_phase_pu = 0.25f;
    if(test_phase_pu < 0.0f)  test_phase_pu = 0.0f;

    DAB_HAL_updatePhaseShift(test_phase_pu, test_direction);

    // 6. Clear Interrupts (NOTE: Fix applied - myADC2_BASE used!)
    ADC_clearInterruptStatus(myADC1_BASE, ADC_INT_NUMBER1);
    ADC_clearInterruptStatus(myADC2_BASE, ADC_INT_NUMBER1);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);

    isr_end_time = CPUTimer_getTimerCount(myCPUTIMER0_BASE);
    isr_total_cycles = isr_start_time - isr_end_time;
}

// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
void main(void)
{



    Device_init();
    Device_initGPIO();
    Interrupt_initModule();
    Interrupt_initVectorTable();
    Board_init();
    C2000Ware_libraries_init();

    DAB_HAL_Init();
    DAB_Filter_Init();

    // FIX: Registering to ADCC (myADC2_1) because it triggers the CPU!
    Interrupt_register(INT_myADC1_1, &DAB_Fast_ISR);

    EINT;
    ERTM;

    while (1)
    {
        // 1. Fault Management
        if (clear_hardware_faults == 1)
        {
            DAB_HAL_clearHardwareTrips();
            clear_hardware_faults = 0;
        }

        // 2. Gate Driver Management
        if (enable_gate_drivers == 1) {
            DAB_HAL_enablePWM();
        } else {
            DAB_HAL_disablePWM();
        }

        GPIO_togglePin(LED_DEBUG);
        DEVICE_DELAY_US(100000);
    }
}
