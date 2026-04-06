/*
 * File:   emg.c
 * Author: paulo
 *
 * Created on March 22, 2026, 2:23 PM
 */
 
#include "xc.h"
#include <string.h>
#include "emg.h"
#include "ads1299.h"
 
EMG_Status EMG_Init(void) {
    // Set the state to 0
    memset(&emg, 0, sizeof(emg));
 
    // Placeholder max. Will be overwritten
    emg.calibratedMax = 1000;
 
    // Noise floor
    emg.threshold = 50;
 
    // Check for error in the ADC
    if (ADS1299_Init() != ADS1299_OK)
        return EMG_ERROR_HARDWARE;
 
    return EMG_OK;
}
 
void EMG_Update(void) {
    int32_t sample;
    int32_t envelope;
    int32_t strength;
 
    // Initialize ADS1299
    if (ADS1299_ReadSample(&sample) != ADS1299_OK)
        return;
 
    // Leaky IIR filter to remove the DC offset
    emg.dcAccumulator += (sample - emg.dcAccumulator) / 64;
    sample -= emg.dcAccumulator;
 
    // Full wave retification
    if (sample < 0)
        sample = -sample;
 
    // Moving average for envelop extraction
    emg.envSum -= emg.envBuffer[emg.envIndex];
    emg.envBuffer[emg.envIndex] = sample;
    emg.envSum += sample;
    emg.envIndex++;
    emg.envIndex %= EMG_ENVELOPE_WINDOW;
 
    envelope = emg.envSum / EMG_ENVELOPE_WINDOW;
 
    // IIR filter for smoothing the envelope
    emg.smoothingState += (envelope - emg.smoothingState) / 8;
 
    // If signal is smaller than the treshold, set to 0
    if (emg.smoothingState < emg.threshold) {
        emg.strength = 0;
        return;
    }
 
    // Normalize 0 - 100
    strength = (emg.smoothingState * 100) / emg.calibratedMax;
 
    // Clamp signals exeeding max to 100
    if (strength > 100)
        strength = 100;
 
    emg.strength = (uint8_t)strength;
}

// Calibration
#define EMG_CALIBRATE_SAMPLES 6000U

void EMG_Calibrate (void){
    uint16_t i = EMG_CALIBRATE_SAMPLES;
    emg.envIndex = 0;
    emg.envSum = 0;
    emg.smoothingState = 0;
    for (int j = 0; j < EMG_ENVELOPE_WINDOW; j++)
        emg.envBuffer[j] = 0;
    
    int32_t peak = 0;
    
    while(i > 0){
        i--;
        EMG_Update();
        if(emg.smoothingState > peak){
            peak = emg.smoothingState;
        }
    }
    
    if (peak < emg.threshold){
        emg.calibratedMax = 1000;
        emg.isCalibrated = 0;
    } else {
        emg.calibratedMax = peak;
        emg.isCalibrated = 1;
    }
}
