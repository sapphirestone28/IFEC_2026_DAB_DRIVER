/*
 * dab_telemetry.c
 *
 *  Created on: 08-Jul-2026
 *      Author: Kaus
 */

#include "dab_telemetry.h"

void DAB_Telemetry_Init(void)
{
    // Start with RS485 in Receive Mode (DE = LOW)
    GPIO_writePin(myBoardRS485_DE, 0);
}

// Helper to push 16-bit integers out as 2 sequential 8-bit bytes (Little Endian)
static void transmit_16bit(uint16_t data)
{
    SCI_writeCharBlockingFIFO(mySCI0_BASE, data & 0x00FF);
    SCI_writeCharBlockingFIFO(mySCI0_BASE, (data >> 8) & 0x00FF);
}

// Helper to push 32-bit IEEE-754 floats out as 4 sequential 8-bit bytes
static void transmit_float(float data)
{
    // Cast the float's memory address to a 32-bit integer pointer to read raw bits
    uint32_t *p = (uint32_t*)&data;
    uint32_t val = *p;

    SCI_writeCharBlockingFIFO(mySCI0_BASE, val & 0x00FF);         // Byte 0 (LSB)
    SCI_writeCharBlockingFIFO(mySCI0_BASE, (val >> 8) & 0x00FF);  // Byte 1
    SCI_writeCharBlockingFIFO(mySCI0_BASE, (val >> 16) & 0x00FF); // Byte 2
    SCI_writeCharBlockingFIFO(mySCI0_BASE, (val >> 24) & 0x00FF); // Byte 3 (MSB)
}

void DAB_Telemetry_Transmit_Binary(DAB_HAL_RawSensors_t *raw, DAB_Measurements_t *meas, float phase_pu, int16_t mode)
{
    // 1. Put RS485 chip into Transmit Mode (DE = HIGH)
    GPIO_writePin(myBoardRS485_DE, 1);

    // 2. Transmit Framing Header (0xAA, 0x55)
    SCI_writeCharBlockingFIFO(mySCI0_BASE, 0xAA);
    SCI_writeCharBlockingFIFO(mySCI0_BASE, 0x55);

    // 3. Transmit RAW Data (8 bytes)
    transmit_16bit(raw->V_bus_raw);
    transmit_16bit(raw->V_bat_raw);
    transmit_16bit((uint16_t)raw->I_bus_raw);
    transmit_16bit((uint16_t)raw->I_bat_raw);

    // 4. Transmit PROCESSED Float Data (20 bytes)
    transmit_float(meas->V_bus_V);
    transmit_float(meas->V_bat_V);
    transmit_float(meas->I_bus_A);
    transmit_float(meas->I_bat_A);
    transmit_float(phase_pu);

    // 5. Transmit Mode (2 bytes)
    transmit_16bit((uint16_t)mode);

    // 6. Wait for transmission to physically finish (32 bytes * 10 bits @ 230400 baud = 1.38 ms)
    while(SCI_getTxFIFOStatus(mySCI0_BASE) != SCI_FIFO_TX0) {}

    while(SCI_isTransmitterBusy(mySCI0_BASE) == true) {}

    // 7. Put RS485 chip back into Receive Mode (DE = LOW)
    GPIO_writePin(myBoardRS485_DE, 0);
}
