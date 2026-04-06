/* 
 * File:   Hardware_Setup.h
 * Author: paulo
 *
 * Created on April 5, 2026, 10:04 PM
 */

#ifndef HARDWARE_SETUP_H
#define	HARDWARE_SETUP_H

#include <stdint.h>

extern volatile int callCalibrate;
void PIC24_Init(void);
void Calibrate_Setup(void);

#endif	/* HARDWARE_SETUP_H */

