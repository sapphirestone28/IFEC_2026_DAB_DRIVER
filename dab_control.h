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
#define SYSTEM_TEST_VOLTAGE 100


//------------------------------------------
//universal control parameters
//------------------------------------------
#define MAX_PHASE_PU         0.24f   // DAB power transfer peaks at 90 deg
#define MIN_PHASE_PU         0.0f

#define MAX_I_SLEW_PER_ISR   0.0005f

#if SYSTEM_TEST_VOLTAGE == 100

    #define V_BAT_CV_MAX_TARGET     100.0f
    #define V_BAT_CV_MIN_TARGET     77.0f
    #define THRESH_G2V_ENTER        95.0f
    #define THRESH_V2G_ENTER        85.0f
    #define P_CHG_LIMIT_W           13.0f
    #define P_DCHG_LIMIT_W          13.0f
    #define I_CC_LIMIT_A            0.20f
    #define I_DCHG_LIMIT_A          0.20f
    #define V_BAT_TAPER_START       82.0f
    #define I_CUTOFF_LIMIT_A        0.02f

#elif SYSTEM_TEST_VOLTAGE == 150

    #define V_BAT_CV_MAX_TARGET     150.0f
    #define V_BAT_CV_MIN_TARGET     116.0f
    #define THRESH_G2V_ENTER        140.0f
    #define THRESH_V2G_ENTER        125.0f
    #define P_CHG_LIMIT_W             45.0f
    #define P_DCHG_LIMIT_W            45.0f
    #define I_CC_LIMIT_A             0.30f
    #define I_DCHG_LIMIT_A            0.30f
    #define V_BAT_TAPER_START       122.0f
    #define I_CUTOFF_LIMIT_A        0.03f

#elif SYSTEM_TEST_VOLTAGE == 200

    #define V_BAT_CV_MAX_TARGET     200.0f
    #define V_BAT_CV_MIN_TARGET     155.0f
    #define THRESH_G2V_ENTER        185.0f
    #define THRESH_V2G_ENTER        170.0f
    #define P_CHG_LIMIT_W             80.0f
    #define P_DCHG_LIMIT_W            80.0f
    #define I_CC_LIMIT_A             0.40f
    #define I_DCHG_LIMIT_A            0.40f
    #define V_BAT_TAPER_START       162.0f
    #define I_CUTOFF_LIMIT_A        0.04f

#elif SYSTEM_TEST_VOLTAGE == 250

    #define V_BAT_CV_MAX_TARGET     250.0f
    #define V_BAT_CV_MIN_TARGET     194.0f
    #define THRESH_G2V_ENTER        235.0f
    #define THRESH_V2G_ENTER        210.0f
    #define P_CHG_LIMIT_W             120.0f
    #define P_DCHG_LIMIT_W            120.0f
    #define I_CC_LIMIT_A             0.48f
    #define I_DCHG_LIMIT_A            0.48f
    #define V_BAT_TAPER_START       200.0f
    #define I_CUTOFF_LIMIT_A        0.05f

#elif SYSTEM_TEST_VOLTAGE == 300
    #define V_BAT_CV_MAX_TARGET  300.0f
    #define V_BAT_CV_MIN_TARGET  233.0f   // (300 * 0.777)
    #define THRESH_G2V_ENTER     280.0f
    #define THRESH_V2G_ENTER     250.0f
    #define P_CHG_LIMIT_W        180.0f   // From Table
    #define P_DCHG_LIMIT_W       180.0f
    #define I_CC_LIMIT_A         0.60f    // From Table
    #define I_DCHG_LIMIT_A       0.60f
    #define V_BAT_TAPER_START    240.0f
    #define I_CUTOFF_LIMIT_A     0.06f

#elif SYSTEM_TEST_VOLTAGE == 450
    // Final Target Profile (Absolute Physical Max is 357W)
    #define V_BAT_CV_MAX_TARGET  450.0f
    #define V_BAT_CV_MIN_TARGET  350.0f
    #define THRESH_G2V_ENTER     420.0f   // Catches the 450V Bus
    #define THRESH_V2G_ENTER     380.0f   // Catches the 350V Bus
    #define P_CHG_LIMIT_W        300.0f   // Safe limit for 55uH HB
    #define P_DCHG_LIMIT_W       300.0f
    #define I_CC_LIMIT_A         0.85f
    #define I_DCHG_LIMIT_A       0.85f
    #define V_BAT_TAPER_START    365.0f   // Starts taper 15V before hitting bottom
    #define I_CUTOFF_LIMIT_A     0.08f

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
