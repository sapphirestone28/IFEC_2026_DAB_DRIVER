/*
 * dab_control.h
 *
 *  Created on: 04-Jul-2026
 *      Author: Kaus
 */

#ifndef DAB_CONTROL_H_
#define DAB_CONTROL_H_


#include "DCLF32.h"
#include "dab_scaling.h"


//=============================================
//change this to the 50, 100 , 150, 200 , or whatever you want
#define SYSTEM_TEST_VOLTAGE 200



//------------------------------------------
//universal control parameters
//------------------------------------------
#define MAX_PHASE_PU         0.25f   // DAB power transfer peaks at 90 deg
#define MIN_PHASE_PU         0.0f

#define MAX_I_SLEW_PER_ISR   0.0005f // Smooth ramp-up

#if SYSTEM_TEST_VOLTAGE == 100

    #define V_BAT_CV_MAX_TARGET     100.0f
    #define V_BAT_CV_MIN_TARGET     77.8f
    #define THRESH_G2V_ENTER        95.0f
    #define THRESH_V2G_ENTER        85.0f
    #define P_CHG_LIMIT_W           80.0f
    #define P_DCHG_LIMIT_W          80.0f
    #define I_CC_LIMIT_A            0.90f
    #define I_DCHG_LIMIT_A          0.90f
    #define V_BAT_TAPER_START       82.8f    // 5V soft-taper padding
    #define I_CUTOFF_LIMIT_A        0.20f    // Safe dead-time cutoff

#elif SYSTEM_TEST_VOLTAGE == 150

    #define V_BAT_CV_MAX_TARGET     150.0f
    #define V_BAT_CV_MIN_TARGET     116.7f
    #define THRESH_G2V_ENTER        142.5f
    #define THRESH_V2G_ENTER        125.0f
    #define P_CHG_LIMIT_W           180.0f   // Perfect intersection (180W / 1.35A = 133.33V Crossover)
    #define P_DCHG_LIMIT_W          180.0f
    #define I_CC_LIMIT_A            1.35f
    #define I_DCHG_LIMIT_A          1.35f
    #define V_BAT_TAPER_START       121.7f
    #define I_CUTOFF_LIMIT_A        0.20f

#elif SYSTEM_TEST_VOLTAGE == 200

    #define V_BAT_CV_MAX_TARGET     200.0f
    #define V_BAT_CV_MIN_TARGET     155.6f
    #define THRESH_G2V_ENTER        190.0f
    #define THRESH_V2G_ENTER        165.0f
    #define P_CHG_LIMIT_W           320.0f   // Perfect intersection (320W / 1.80A = 177.77V Crossover)
    #define P_DCHG_LIMIT_W          320.0f
    #define I_CC_LIMIT_A            1.80f
    #define I_DCHG_LIMIT_A          1.80f
    #define V_BAT_TAPER_START       160.6f
    #define I_CUTOFF_LIMIT_A        0.20f

#elif SYSTEM_TEST_VOLTAGE == 225

    //
    #define V_BAT_CV_MAX_TARGET     225.0f
    #define V_BAT_CV_MIN_TARGET     175.0f
    #define THRESH_G2V_ENTER        215.0f   // Catches the 450V Bus
    #define THRESH_V2G_ENTER        195.0f   // Catches the 350V Bus
    #define P_CHG_LIMIT_W           250.0f  // IFEC 1kW Target
    #define P_DCHG_LIMIT_W          250.0f
    #define I_CC_LIMIT_A            1.25f    // 1000W / 400V Crossover
    #define I_DCHG_LIMIT_A          1.25f
    #define V_BAT_TAPER_START       180.0f   // Starts taper 5V before hitting bottom
    #define I_CUTOFF_LIMIT_A        0.30f    // Slightly higher cutoff to prevent 450V low-power ringing

#elif SYSTEM_TEST_VOLTAGE == 250

    #define V_BAT_CV_MAX_TARGET     250.0f
    #define V_BAT_CV_MIN_TARGET     194.4f
    #define THRESH_G2V_ENTER        237.5f
    #define THRESH_V2G_ENTER        205.0f
    #define P_CHG_LIMIT_W           500.0f   // Perfect intersection (500W / 2.25A = 222.22V Crossover)
    #define P_DCHG_LIMIT_W          500.0f
    #define I_CC_LIMIT_A            2.25f
    #define I_DCHG_LIMIT_A          2.25f
    #define V_BAT_TAPER_START       199.4f
    #define I_CUTOFF_LIMIT_A        0.20f

#elif SYSTEM_TEST_VOLTAGE == 300

    #define V_BAT_CV_MAX_TARGET     300.0f
    #define V_BAT_CV_MIN_TARGET     233.3f
    #define THRESH_G2V_ENTER        285.0f
    #define THRESH_V2G_ENTER        245.0f
    #define P_CHG_LIMIT_W           660.0f   // Perfect intersection (660W / 2.47A = 267.2V Crossover)
    #define P_DCHG_LIMIT_W          660.0f
    #define I_CC_LIMIT_A            2.47f    // Safely held below hardware 2.5A limits!
    #define I_DCHG_LIMIT_A          2.47f
    #define V_BAT_TAPER_START       238.3f
    #define I_CUTOFF_LIMIT_A        0.20f

#elif SYSTEM_TEST_VOLTAGE == 450

    // Final Target Profile (Absolute Physical Max is ~1714W)
    #define V_BAT_CV_MAX_TARGET     450.0f
    #define V_BAT_CV_MIN_TARGET     350.0f
    #define THRESH_G2V_ENTER        420.0f   // Catches the 450V Bus
    #define THRESH_V2G_ENTER        380.0f   // Catches the 350V Bus
    #define P_CHG_LIMIT_W           1000.0f  // IFEC 1kW Target
    #define P_DCHG_LIMIT_W          1000.0f
    #define I_CC_LIMIT_A            2.50f    // 1000W / 400V Crossover
    #define I_DCHG_LIMIT_A          2.50f
    #define V_BAT_TAPER_START       355.0f   // Starts taper 5V before hitting bottom
    #define I_CUTOFF_LIMIT_A        0.30f    // Slightly higher cutoff to prevent 450V low-power ringing

#else
    #error "SYSTEM_TEST_VOLTAGE is not defined or is set to an unsupported value. Please define it to 100, 150, 200, 250, 300, or 450."
#endif


#define TAPER_SLOPE (I_DCHG_LIMIT_A / (V_BAT_TAPER_START - V_BAT_CV_MIN_TARGET))  // Slope of the taper region



// ---------------------------------------------
// PI controllers
// ---------------------------------------------
extern DCL_PI pi_g2v_voltage;
extern DCL_PI pi_v2g_voltage;
extern DCL_PI pi_g2v_current;
extern DCL_PI pi_v2g_current;


void DAB_Control_Init(void);
void DAB_Run_Control(DAB_Measurements_t *meas, float *out_phase, int *out_dir);







#endif /* DAB_CONTROL_H_ */
