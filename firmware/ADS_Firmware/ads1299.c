#include "xc.h"
#include <stdint.h>
#include "ads1299.h"
 
// Timer1 runs continuously at 1ms per tick, incremented by the T1 interrupt.
// delay_ms() and delay_us() read ms_ticks and TMR1 - no setup per call.
//
// Configuration: Fcy=16MHz, prescaler 1:8 -> tick rate 2MHz
// PR1 = 2000 - 1 = 1999 -> period = 1ms exactly
#define TIMER1_TICKS_PER_MS  2000u  // FCY_HZ / 8 / 1000
 
#define FCY 16000000UL

volatile uint32_t us_ticks = 0;

void TMR1_Init(void){
    T1CONbits.TON = 0;
    T1CONbits.TCS = 0; // Internal clock
    T1CONbits.TCKPS = 0; // 1:1 prescaler

    TMR1 = 0;
    PR1 = (uint16_t)(FCY / 1000000UL); // 1 µs period

    IFS0bits.T1IF = 0; // Clear flag
    IEC0bits.T1IE = 1; // Enable interrupt

    T1CONbits.TON = 1; // Start timer
}

void delay_us(uint32_t us){
    uint32_t start = us_ticks;
    while ((us_ticks - start) < us);
}

void delay_ms(uint32_t ms){
    delay_us(ms * 1000UL);
}

// ADS1299 needs 4 tCLK between command bytes and before CS high.
// At 2.048MHz CLK: 4 tCLK = ~1.95us. 2us gives clean margin.
static void delay_4tclk(void) {
    delay_us(2);
}
 
// SPI helpers
static uint8_t SPI1_Transfer(uint8_t data) {
    SPI1BUF = data;
    while (!SPI1STATbits.SPIRBF);
    delay_4tclk();
    return SPI1BUF;
}
 
static void ADS1299_SendCommand(uint8_t cmd) {
    ADS1299_CS_PIN = 0;
    SPI1_Transfer(cmd);
    ADS1299_CS_PIN = 1;
    delay_4tclk();
}
 
// CPOL=0, CPHA=1 (CKP=0, CKE=0) per datasheet Section 7.6.
// SCLK = 16MHz / (1 * 8) = 2MHz
static void SPI1_Init(void) {
    SPI1CON1 = 0;
    SPI1CON2 = 0;
    SPI1STAT = 0;
 
    ADS1299_SCK_TRIS = 0;
    ADS1299_MOSI_TRIS = 0;
    ADS1299_MISO_TRIS = 1;
 
    __builtin_write_OSCCONL(OSCCON & ~(1 << 6));
    RPOR7bits.RP15R = 8;      // RB15 -> SCK1OUT
    RPOR7bits.RP14R = 7;      // RB14 -> SDO1
    RPINR20bits.SDI1R = 13;   // SDI1 <- RP13 = RB13
    __builtin_write_OSCCONL(OSCCON | (1 << 6));
 
    SPI1CON1bits.MSTEN = 1;
    SPI1CON1bits.CKP = 0; // idle low (CPOL=0)
    SPI1CON1bits.CKE = 1; // shift on idle->active (CPHA=1)
    SPI1CON1bits.MODE16 = 0;
    SPI1CON1bits.PPRE = 3; // primary prescale 1:1
    SPI1CON1bits.SPRE = 3; // secondary prescale 8:1 -> 2MHz SCLK
 
    SPI1STATbits.SPIEN = 1;
}
 
void GPIO_Init(void) {
    // RST - RA0
    TRISAbits.TRISA0 = 0;
    LATAbits.LATA0 = 1;

    // DRDY - RA1 input
    TRISAbits.TRISA1 = 1;
    
    // CS - RA4
    TRISAbits.TRISA4 = 0;
    LATAbits.LATA4 = 1;
}
 
ADS1299_Status ADS1299_WriteReg(uint8_t address, uint8_t value) {
    ADS1299_CS_PIN = 0;
    SPI1_Transfer(ADS1299_CMD_WREG | address);
    delay_4tclk();
    SPI1_Transfer(0x00);
    delay_4tclk();
    SPI1_Transfer(value);
    ADS1299_CS_PIN = 1;
    delay_4tclk();
    return ADS1299_OK;
}
 
ADS1299_Status ADS1299_ReadReg(uint8_t address, uint8_t *value) {
    ADS1299_CS_PIN = 0;
    SPI1_Transfer(ADS1299_CMD_RREG | address);
    delay_4tclk();
    SPI1_Transfer(0x00);
    delay_4tclk();
    *value = SPI1_Transfer(0x00);
    ADS1299_CS_PIN = 1;
    delay_4tclk();
    return ADS1299_OK;
}
 
// Frame: 3 status bytes + 8 x 3 channel bytes = 27 bytes total (Section 9.4.4.2)
ADS1299_Status ADS1299_ReadSample(int32_t *ch1) {
    uint8_t b0, b1, b2;
    int32_t raw;
    uint8_t i;
    uint16_t timeout = 10000;
 
    while (ADS1299_DRDY_PIN != ADS1299_DRDY_READY) {
        __builtin_nop();
        if (--timeout == 0)
            return ADS1299_ERROR_TIMEOUT;
    }
 
    ADS1299_CS_PIN = 0;
 
    // Discard status bytes
    SPI1_Transfer(0x00);
    SPI1_Transfer(0x00);
    SPI1_Transfer(0x00);
 
    // CH1 - 24 bits MSB first
    b0 = SPI1_Transfer(0x00);
    b1 = SPI1_Transfer(0x00);
    b2 = SPI1_Transfer(0x00);
 
    raw = ((int32_t)b0 << 16) | ((int32_t)b1 << 8) | (int32_t)b2;
 
    // Sign-extend 24-bit two's complement to 32-bit
    if (raw & 0x00800000)
        raw |= 0xFF000000;
 
    *ch1 = raw;
 
    // Clock out CH2-CH8 to maintain frame sync
    for (i = 0; i < 21; i++)
        SPI1_Transfer(0x00);
 
    ADS1299_CS_PIN = 1;
    delay_4tclk();
 
    return ADS1299_OK;
}
 
// Full power-up sequence per Figure 67 of the datasheet.
// TMR1_Init() must be called before this function.
ADS1299_Status ADS1299_Init(void) {
    GPIO_Init();
    SPI1_Init();
 
    // tPOR: wait >= 128ms from power-up before reset (Table 30, 2^18 tCLK at 2.048MHz)
    // 150ms also covers VCAP1 >= 1.1V requirement (Figure 76)
    delay_ms(150);
 
    // Hardware reset - hold low >= 2 tCLK (tRST), then release
    ADS1299_RESET_PIN = 0;
    delay_ms(1);
    ADS1299_RESET_PIN = 1;
 
    // Wait 18 tCLK for register initialisation after reset
    delay_ms(1);
 
    // Explicit RESET command
    ADS1299_SendCommand(ADS1299_CMD_RESET);
    delay_ms(1);
    
    // Device wakes in RDATAC - send SDATAC before any register writes
    ADS1299_SendCommand(ADS1299_CMD_SDATAC);
    delay_4tclk();
 
    // Enable internal reference buffer, wait 150ms to settle
    ADS1299_WriteReg(ADS1299_REG_CONFIG3, ADS1299_CONFIG3_REFON);
    delay_ms(150);
 
    ADS1299_WriteReg(ADS1299_REG_CONFIG1, ADS1299_CONFIG1_2KSPS);
    ADS1299_WriteReg(ADS1299_REG_CONFIG2, ADS1299_CONFIG2_DEFAULT);
    ADS1299_WriteReg(ADS1299_REG_CH1SET, ADS1299_CHNSET_ACTIVE);
    ADS1299_WriteReg(ADS1299_REG_MISC1, ADS1299_MISC1_SRB1ON);
 
    // Power down unused channels, input shorted
    ADS1299_WriteReg(ADS1299_REG_CH2SET, ADS1299_CHNSET_POWERDOWN);
    ADS1299_WriteReg(ADS1299_REG_CH3SET, ADS1299_CHNSET_POWERDOWN);
    ADS1299_WriteReg(ADS1299_REG_CH4SET, ADS1299_CHNSET_POWERDOWN);
    ADS1299_WriteReg(ADS1299_REG_CH5SET, ADS1299_CHNSET_POWERDOWN);
    ADS1299_WriteReg(ADS1299_REG_CH6SET, ADS1299_CHNSET_POWERDOWN);
    ADS1299_WriteReg(ADS1299_REG_CH7SET, ADS1299_CHNSET_POWERDOWN);
    ADS1299_WriteReg(ADS1299_REG_CH8SET, ADS1299_CHNSET_POWERDOWN);
 
    ADS1299_SendCommand(ADS1299_CMD_START);
    ADS1299_SendCommand(ADS1299_CMD_RDATAC);
 
    return ADS1299_OK;
}