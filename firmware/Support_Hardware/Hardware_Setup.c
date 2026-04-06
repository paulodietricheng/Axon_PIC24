/*
 * File:   Hardware_Setup.c
 * Author: paulo
 *
 * Created on April 5, 2026, 9:30 PM
 */


#include "xc.h"
#include "ads1299.h"
#include "emg.h"
#include "Hardware_Setup.h"

#define PRESSBUFF 2

volatile unsigned int pressBuffer[PRESSBUFF];
volatile unsigned int currPress;
volatile unsigned int prevPress = 0;
volatile int rollOver = 0;
volatile int callCalibrate = 0;

void __attribute__((interrupt, auto_psv)) _IC1Interrupt(){
    _IC1IF = 0;    
    static unsigned int pressIdx = 0;

    while(_ICBNE){ // Empty the IC buffer to select the most recent timer possible
        currPress = rollOver*(PR2+1) + IC1BUF;
    }
   
    if(currPress - prevPress > (PR2+1)/50){ // If the time difference is greater than 2 ms, assign as a real click.
        prevPress = currPress;
        pressBuffer[pressIdx] = prevPress;
        pressIdx = (pressIdx + 1) % PRESSBUFF;
    }
    
    // Call calibrate function
    callCalibrate = 1;
}

void PIC24_Init(void) {
    AD1PCFG = 0xFFFF;       // All pins digital
    CLKDIVbits.RCDIV = 0;   // 16 MHz Fcy
}

void Calibrate_Setup(void){
    TRISBbits.TRISB12 = 1;
    CNPU1bits.CN14PUE = 1;    // CN14 maps to RB12

    __builtin_write_OSCCONL(OSCCON & 0xbf);
    RPINR7bits.IC1R = 12;     // IC1 <- RP12 = RB12
    __builtin_write_OSCCONL(OSCCON | 0x40);
    
    // Timer 2 setup and interrupt enable
    T2CON = 0;
    T2CONbits.TCKPS = 0b11;// 1:256 pre-scaler
    T2CONbits.TON = 1;
    TMR2 = 0;    
    PR2 = 62499;
    _T2IF = 0;
    _T2IE = 1; // Enable interrupt
    
    // Input Capture 1 setup
    IC1CON = 0;
    IC1CONbits.ICTMR = 1; // Timer 2
    IC1CONbits.ICM = 2; // Detect every falling edge
    _IC1IF = 0;
    _IC1IE = 1;
}

void __attribute__((interrupt, auto_psv)) _T2Interrupt(){
    _T2IF = 0;
    rollOver++;
}
