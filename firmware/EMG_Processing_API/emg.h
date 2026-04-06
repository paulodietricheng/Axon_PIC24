/* 
 * File:   emg.h
 * Author: paulo
 *
 * Created on March 22, 2026, 2:26 PM
 */

#ifndef EMG_H
#define EMG_H
 
#include <stdint.h>
 
// Status codes
typedef enum {
    EMG_OK = 0,
    EMG_ERROR_HARDWARE = 1
} EMG_Status;

#define EMG_ENVELOPE_WINDOW 500 // 250ms at 2kSPS
 
// EMG library state
typedef struct {
    // DC offset removal
    int32_t dcAccumulator; // Running average of raw samples (IIR)
 
    // Envelope extraction - circular buffer moving average
    int32_t envBuffer[EMG_ENVELOPE_WINDOW]; // Circular buffer of rectified samples
    int32_t envSum; // Running sum of buffer contents
    uint16_t envIndex;  
    
    // Calibration
    int32_t calibratedMax; // Peak envelope seen during calibration
    uint8_t isCalibrated; // 0 = not calibrated, 1 = calibrated
    
    // Thresholding
    int32_t threshold; // Noise floor
 
    // Smoothing
    int32_t smoothingState; // IIR low-pass filter state on envelope
 
    // Output
    uint8_t strength; // Normalized output: 0-100 (%)
 
} EMG_State;
 
extern EMG_State emg;

// API declarations
EMG_Status EMG_Init(void);
void EMG_Update(void);
void EMG_Calibrate(void);
 
#endif /* EMG_H */