/*
 * dab_filter.h
 *
 *  Created on: 19-Jun-2026
 *      Author: Kaus
 */

#ifndef DAB_FILTER_H_
#define DAB_FILTER_H_

#include <stdint.h>

#define VOLTAGE_BUF_SIZE 50
#define VOLTAGE_BUF_INV  0.02f   // 1.0 / 50.0

typedef struct {
    uint16_t buffer[VOLTAGE_BUF_SIZE];
    uint16_t index;
    uint32_t sum;
    float    alpha;
    float    prev_out;
} Voltage_Filter_t;

// Fast IIR Filter for Currents (Low Delay)
typedef struct {
    float alpha;    
    float prev_out; 
} Fast_IIR_Filter_t;

// Filter Instances
extern Voltage_Filter_t   filt_V_bus;
extern Voltage_Filter_t   filt_V_bat;
extern Fast_IIR_Filter_t filt_I_bus;
extern Fast_IIR_Filter_t filt_I_bat;

// Function Prototypes
void DAB_Filter_Init(void);
float DAB_Run_Voltage_Filter(Voltage_Filter_t *filt, uint16_t new_adc_sample);
float DAB_Run_Fast_IIR(Fast_IIR_Filter_t *filt, float input);

#endif /* DAB_FILTER_H_ */
