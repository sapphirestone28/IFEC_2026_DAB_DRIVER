/*
 * dab_control.c
 *
 *  Created on: 04-Jul-2026
 *      Author: Kaus
 *  Revised:
 *    - CV/CC/CP selection via dynamic Umax => correct anti-windup + handover
 *    - Integrators fully reset on a direction swap (DCL_resetPI)
 *    - Added Light-Load Dead-Time Cutoff logic
 *    - Added RAM execution (.TI.ramfunc) and Division-to-Multiplication optimizations
 */

#include "dab_control.h"
#include "DCLF32.h"
#include <math.h>

// ---------------------------------------------------------
// CONTROLLER INSTANCES
// ---------------------------------------------------------
DCL_PI pi_g2v_voltage = {0}; // CV outer loop (Charge)
DCL_PI pi_v2g_voltage = {0}; // CV outer loop (Discharge)
DCL_PI pi_g2v_current = {0}; // CC inner loop (Charge)
DCL_PI pi_v2g_current = {0}; // CC inner loop (Discharge)

// ---------------------------------------------------------
// STATE & SLEW VARIABLES
// ---------------------------------------------------------
// 0 = standby, +1 = charge, -1 = discharge
static int active_dab_mode = 0;
static float I_ref_slewed = 0.0f; // Slewed reference current for the inner loop

void DAB_Control_Init(void)
{

#if SYSTEM_TEST_VOLTAGE == 100
    float Kp_cur = 0.045f;
    float Ki_cur = 0.0045f;
    float Kp_vol = 0.225f;
    float Ki_vol = 0.0225f;
#elif SYSTEM_TEST_VOLTAGE == 150
    float Kp_cur = 0.030f;
    float Ki_cur = 0.0030f;
    float Kp_vol = 0.150f;
    float Ki_vol = 0.0150f;
#elif SYSTEM_TEST_VOLTAGE == 200
    float Kp_cur = 0.022f;
    float Ki_cur = 0.0022f;
    float Kp_vol = 0.110f;
    float Ki_vol = 0.0110f;
#elif SYSTEM_TEST_VOLTAGE == 250
    float Kp_cur = 0.018f;
    float Ki_cur = 0.0018f;
    float Kp_vol = 0.090f;
    float Ki_vol = 0.0090f;
#elif SYSTEM_TEST_VOLTAGE == 300
    float Kp_cur = 0.015f;
    float Ki_cur = 0.0015f;
    float Kp_vol = 0.075f;
    float Ki_vol = 0.0075f;
#elif SYSTEM_TEST_VOLTAGE == 400
    float Kp_cur = 0.011f;
    float Ki_cur = 0.0011f;
    float Kp_vol = 0.055f;
    float Ki_vol = 0.0055f;
#elif SYSTEM_TEST_VOLTAGE == 450
    float Kp_cur = 0.010f;
    float Ki_cur = 0.0010f;
    float Kp_vol = 0.050f;
    float Ki_vol = 0.0050f;
#endif

    // Inner current loops
    pi_g2v_current.Kp = Kp_cur;
    pi_g2v_current.Ki = Ki_cur;
    pi_g2v_current.Umax = MAX_PHASE_PU;
    pi_g2v_current.Umin = MIN_PHASE_PU;
    DCL_resetPI(&pi_g2v_current);

    pi_v2g_current.Kp = Kp_cur;
    pi_v2g_current.Ki = Ki_cur;
    pi_v2g_current.Umax = MAX_PHASE_PU;
    pi_v2g_current.Umin = MIN_PHASE_PU;
    DCL_resetPI(&pi_v2g_current);

    // Outer voltage loops
    pi_g2v_voltage.Kp = Kp_vol;
    pi_g2v_voltage.Ki = Ki_vol;
    pi_g2v_voltage.Umax = I_CC_LIMIT_A;
    pi_g2v_voltage.Umin = 0.0f;
    DCL_resetPI(&pi_g2v_voltage);

    pi_v2g_voltage.Kp = Kp_vol;
    pi_v2g_voltage.Ki = Ki_vol;
    pi_v2g_voltage.Umax = I_DCHG_LIMIT_A;
    pi_v2g_voltage.Umin = 0.0f;
    DCL_resetPI(&pi_v2g_voltage);

    active_dab_mode = 0;
    I_ref_slewed = 0.0f;
}

// ---------------------------------------------------------
// MAIN CONTROL STEP (Optimized for FPU)
// ---------------------------------------------------------
// Force the ISR to execute from high-speed RAM instead of Flash
#pragma CODE_SECTION(DAB_Run_Control, ".TI.ramfunc");
void DAB_Run_Control(DAB_Measurements_t *meas, float *out_phase, int *out_dir)
{
    float I_target = 0.0f; // Target current for the inner loop
    int requested_mode = active_dab_mode;

    // ======================================================
    // 1. AFE INTENTION DETECTION (What does the Grid want?)
    // Uses a hysteresis band to prevent bouncing
    // CRITICAL FIX: Checking V_bus_V (Grid side), not V_bat_V!
    // ======================================================
    if (meas->V_bus_V > THRESH_G2V_ENTER)
    {
        requested_mode = DAB_DIR_CHARGE;
    }
    else if (meas->V_bus_V < THRESH_V2G_ENTER)
    {
        requested_mode = DAB_DIR_DISCHARGE;
    }

    // ======================================================
    // 2. ZERO-DWELL DIRECTION SWAP MANAGER
    // Ramps to zero current slowly before flipping the physical mode
    // ======================================================
    if (requested_mode != active_dab_mode)
    {
        I_target = 0.0f;

        if (I_ref_slewed <= 0.01f)
        {
            active_dab_mode = requested_mode;

            // Full reset of ALL controllers to wipe integrator memory
            DCL_resetPI(&pi_g2v_voltage);
            DCL_resetPI(&pi_v2g_voltage);
            DCL_resetPI(&pi_g2v_current);
            DCL_resetPI(&pi_v2g_current);
        }
    }
    else
    {
        // ==================================================
        // OPTIMIZATION: INVERSE VOLTAGE CALCULATION
        // ==================================================
        float safe_v_bat = fmaxf(meas->V_bat_V, 5.0f);
        float inv_v_bat = 1.0f / safe_v_bat;

        // ==================================================
        // 3. TARGET CURRENT (we are settled in a mode)
        // ==================================================
        if (active_dab_mode == DAB_DIR_CHARGE)
        {
            // ======== G2V MODE (Charge) ==============
            float cc_cp_limit = fminf(I_CC_LIMIT_A, P_CHG_LIMIT_W * inv_v_bat);

            pi_g2v_voltage.Umax = cc_cp_limit;
            pi_g2v_voltage.Umin = 0.0f;

            I_target = DCL_runPI_C6(&pi_g2v_voltage, V_BAT_CV_MAX_TARGET, meas->V_bat_V);
        }
        else if (active_dab_mode == DAB_DIR_DISCHARGE)
        {
            // ======== V2G MODE (Discharge) ==============
            float cc_cp_limit = fminf(I_DCHG_LIMIT_A, P_DCHG_LIMIT_W * inv_v_bat);

            pi_v2g_voltage.Umax = cc_cp_limit;
            pi_v2g_voltage.Umin = 0.0f;

            // INVERTING THE ERROR MATH!
            // Meas is 'rk' and Target is 'yk'.
            I_target = DCL_runPI_C6(&pi_v2g_voltage, meas->V_bat_V, V_BAT_CV_MIN_TARGET);

            // Hard Backup Taper (Soft landing for bottom of battery limits)
            if (meas->V_bat_V < V_BAT_TAPER_START)
            {
                float taper = (meas->V_bat_V - V_BAT_CV_MIN_TARGET) * TAPER_SLOPE;
                I_target = fminf(I_target, taper);
            }
            if (meas->V_bat_V <= V_BAT_CV_MIN_TARGET)
            {
                I_target = 0.0f;
            }
        }
        else
        {
            // Standby
            I_target = 0.0f;
        }
    }

    // ======================================================
    // 4. LIGHT-LOAD DEAD-TIME CUTOFF
    // Prevents Bang-Bang oscillations from hardware swallowing pulses
    // ======================================================
    if (I_target < I_CUTOFF_LIMIT_A)
    {
        I_target = 0.0f;
    }

    // ======================================================
    // 5. SLEW-RATE LIMITER
    // ======================================================
    if (I_ref_slewed < I_target)
    {
        I_ref_slewed += MAX_I_SLEW_PER_ISR;
        if (I_ref_slewed > I_target)
            I_ref_slewed = I_target;
    }
    else if (I_ref_slewed > I_target)
    {
        I_ref_slewed -= MAX_I_SLEW_PER_ISR;
        if (I_ref_slewed < I_target)
            I_ref_slewed = I_target;
    }
    if (I_ref_slewed < 0.0f)
        I_ref_slewed = 0.0f;

    // ======================================================
    // 6. INNER CURRENT LOOP
    // ======================================================
    *out_dir = active_dab_mode;

    if (I_ref_slewed == 0.0f)
    {
        *out_phase = 0.0f;
        DCL_resetPI(&pi_g2v_current);
        DCL_resetPI(&pi_v2g_current);
    }
    else if (active_dab_mode == DAB_DIR_CHARGE)
    {
        *out_phase = DCL_runPI_C6(&pi_g2v_current, I_ref_slewed, meas->I_bat_A);
    }
    else if (active_dab_mode == DAB_DIR_DISCHARGE)
    {
        // Must use fabsf() because discharging current physically flows backwards
        *out_phase = DCL_runPI_C6(&pi_v2g_current, I_ref_slewed, fabsf(meas->I_bat_A));
    }
    else
    {
        *out_phase = 0.0f;
    }
}
