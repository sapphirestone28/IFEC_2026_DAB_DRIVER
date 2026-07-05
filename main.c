// #############################################################################
//  FILE:   main.c
//  TITLE:  1kW DAB Closed-Loop Controller (CC-CP-CV & V2G)
// #############################################################################

#include "driverlib.h"
#include "device.h"
#include "board.h"
#include "c2000ware_libraries.h"

#include "dab_hal.h"
#include "dab_scaling.h"
#include "dab_filter.h"
#include "dab_control.h"

// ---------------------------------------------------------
// GLOBALS
// ---------------------------------------------------------
DAB_HAL_RawSensors_t dab_sensors;
extern DAB_Measurements_t dab_meas;

// Profiling Variables
volatile uint32_t isr_start_time = 0;
volatile uint32_t isr_end_time = 0;
volatile uint32_t isr_total_cycles = 0;

// Hardware Controls (Edit in CCS Expressions!)
volatile int clear_hardware_faults = 1; // Set to 1 to clear trips
volatile int enable_gate_drivers = 0;   // Start at 0 for safety!

// Open-Loop Manual Test Variables (Currently Commented Out)
volatile float test_phase = 0.0f;       // Degrees (0 to 90)
volatile int   test_direction = 1;      // 1 = Charge (G2V), -1 = Discharge (V2G)

// Master Variables sent to Hardware
volatile float active_phase_pu = 0.0f;
volatile int   active_direction = 1;

// ---------------------------------------------------------
// THE 50 kHz FAST ISR
// ---------------------------------------------------------
// Force the ISR to execute from high-speed RAM instead of Flash
#pragma CODE_SECTION(DAB_Fast_ISR, ".TI.ramfunc");
__interrupt void DAB_Fast_ISR(void)
{
    GPIO_writePin(myGPIOdebug, 1); // Start ISR Profiling
    isr_start_time = CPUTimer_getTimerCount(myCPUTIMER0_BASE);

    // 1. Read Raw Sensors
    DAB_HAL_readSensors(&dab_sensors);

    // 2. Filter Voltages
    float smooth_V_bus_raw = DAB_Run_Voltage_Filter(&filt_V_bus, dab_sensors.V_bus_raw);
    float smooth_V_bat_raw = DAB_Run_Voltage_Filter(&filt_V_bat, dab_sensors.V_bat_raw);

    // 3. Scale and populate dab_meas
    DAB_Scale_Sensors(smooth_V_bus_raw, smooth_V_bat_raw, &dab_sensors, &dab_meas);

    // 4. Filter Currents
    dab_meas.I_bus_A = DAB_Run_Fast_IIR(&filt_I_bus, dab_meas.I_bus_A);
    dab_meas.I_bat_A = DAB_Run_Fast_IIR(&filt_I_bat, dab_meas.I_bat_A);

    // Refresh PUs based on filtered current (Multiplication for FPU speed!)
    dab_meas.I_bus_pu = dab_meas.I_bus_A * I_BASE_INV;
    dab_meas.I_bat_pu = dab_meas.I_bat_A * I_BASE_INV;
    dab_meas.P_bat_W  = dab_meas.V_bat_V * dab_meas.I_bat_A;
    dab_meas.P_bat_pu = dab_meas.P_bat_W * P_BASE_INV;

    // =========================================================
    // 5. CLOSED-LOOP CONTROL MATH
    // =========================================================

    // --> CLOSED LOOP ACTIVE: PI controllers calculate phase and direction
    DAB_Run_Control(&dab_meas, (float*)&active_phase_pu, (int*)&active_direction);

    // ---> OPEN LOOP (Manual Override): Uncomment below block to run Open-Loop
    /*
    // Safety clamp user input BEFORE math
    if (test_phase > 90.0f) test_phase = 90.0f;
    if (test_phase < 0.0f)  test_phase = 0.0f;

    // Convert Degrees to PU (1 / 360 = 0.0027777778f)
    active_phase_pu = test_phase * 0.0027777778f;
    active_direction = test_direction;
    */
    // =========================================================

    // 6. Send commands to Hardware
    DAB_HAL_updatePhaseShift(active_phase_pu, active_direction);

    // 7. Clear Interrupts
    ADC_clearInterruptStatus(myADC1_BASE, ADC_INT_NUMBER1);
    ADC_clearInterruptStatus(myADC2_BASE, ADC_INT_NUMBER1);

    isr_end_time = CPUTimer_getTimerCount(myCPUTIMER0_BASE);
    isr_total_cycles = isr_start_time - isr_end_time;

    GPIO_writePin(myGPIOdebug, 0); // End ISR Profiling
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

// ---------------------------------------------------------
// UTILITY
// ---------------------------------------------------------
void delay_loop(void)
{
    DEVICE_DELAY_US(100000); // 100 ms
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
    DAB_Control_Init(); // CRITICAL: Initializes PI controllers!

    // Using ADCA (myADC1) trigger as requested
    Interrupt_register(INT_myADC1_1, &DAB_Fast_ISR);

    EINT;
    ERTM;


    int boot_timer;
       for(boot_timer = 0; boot_timer < 50; boot_timer++)
       {
           // Toggle LEDs rapidly so user knows board is alive and waiting
           GPIO_togglePin(LED_DEBUG_2);
           delay_loop(); // 50 * 100ms = 5000ms (5 seconds)
       }

       // Clear any EMI noise trips that happened when high voltage was turned on
       DAB_HAL_clearHardwareTrips();
       clear_hardware_faults = 0;

       // Enable drivers to let PI loops take control!
       enable_gate_drivers = 1;
       DAB_HAL_enablePWM();
       // =========================================================

    static int tick_counter = 0;

    while (1)
    {
        // 1. Fault Management
        if (clear_hardware_faults == 1)
        {
            DAB_HAL_clearHardwareTrips();
            clear_hardware_faults = 0;
        }

        // 2. Gate Driver Management
        if (enable_gate_drivers == 1)
        {
            DAB_HAL_enablePWM();
        }
        else
        {
            DAB_HAL_disablePWM();
        }

        // =========================================================
        // 3. OPTIONAL AUTOMATED SEQUENCE
        // =========================================================
        /*
        tick_counter++;
        if (tick_counter >= 40) // 40 ticks * 100ms = 4 seconds
        {
            tick_counter = 0;
            test_phase += 5.0f; // Jump 5 degrees

            if (test_phase > 70.0f) {
                test_phase = 70.0f;
            }
        }
        */

        // =========================================================
        // 4. THE HARDWARE DAC (ePWM7B Duty Cycle)
        // Passes active_phase_pu out to a multimeter!
        // Maps 0.0 to 0.25 PU directly to 0-240 ticks (0% to 100% duty)
        // Math: 240 - (PU * 4 * 240) = 240 - (PU * 960)
        // =========================================================
        uint16_t dac_cmpb = 240 - (uint16_t)(active_phase_pu * 960.0f);
        EPWM_setCounterCompareValue(myEPWM7_BASE, EPWM_COUNTER_COMPARE_B, dac_cmpb);

        // 5. Heartbeats
        GPIO_togglePin(LED_DEBUG);

        delay_loop();
    }
}
