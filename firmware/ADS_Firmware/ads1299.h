/* 
 * File:   ads1299.h
 * Author: paulo
 *
 * Created on March 22, 2026, 2:51 PM
 */

#ifndef ADS1299_H
#define ADS1299_H
 
#include <stdint.h>
 
// Status codes
typedef enum {
    ADS1299_OK = 0,
    ADS1299_ERROR_SPI = 1,
    ADS1299_ERROR_TIMEOUT = 2
} ADS1299_Status;

#define ADS1299_CS_TRIS TRISAbits.TRISA4
#define ADS1299_CS_PIN LATAbits.LATA4
#define ADS1299_DRDY_TRIS TRISAbits.TRISA1
#define ADS1299_DRDY_PIN PORTAbits.RA1
#define ADS1299_RESET_TRIS TRISAbits.TRISA0
#define ADS1299_RESET_PIN LATAbits.LATA0

// RB15 = RP15 = SCK1OUT
// RB14 = RP14 = SDO1
// RB13 = RP13 = SDI1
#define ADS1299_SCK_TRIS TRISBbits.TRISB15
#define ADS1299_MOSI_TRIS TRISBbits.TRISB14
#define ADS1299_MISO_TRIS TRISBbits.TRISB13
 
// SPI command definitions Datasheet Table 10
#define ADS1299_CMD_WAKEUP 0x02
#define ADS1299_CMD_STANDBY 0x04
#define ADS1299_CMD_RESET 0x06
#define ADS1299_CMD_START 0x08
#define ADS1299_CMD_STOP 0x0A
#define ADS1299_CMD_RDATAC 0x10
#define ADS1299_CMD_SDATAC 0x11
#define ADS1299_CMD_RDATA 0x12
#define ADS1299_CMD_RREG 0x20 
#define ADS1299_CMD_WREG 0x40 
 
// Register addresses Datasheet Table 11
#define ADS1299_REG_ID 0x00
#define ADS1299_REG_CONFIG1 0x01
#define ADS1299_REG_CONFIG2 0x02
#define ADS1299_REG_CONFIG3 0x03
#define ADS1299_REG_LOFF 0x04
#define ADS1299_REG_CH1SET 0x05
#define ADS1299_REG_CH2SET 0x06
#define ADS1299_REG_CH3SET 0x07
#define ADS1299_REG_CH4SET 0x08
#define ADS1299_REG_CH5SET 0x09
#define ADS1299_REG_CH6SET 0x0A
#define ADS1299_REG_CH7SET 0x0B
#define ADS1299_REG_CH8SET 0x0C
#define ADS1299_REG_MISC1 0x15
 
// Register configuration values
#define ADS1299_CONFIG1_2KSPS 0x93
#define ADS1299_CONFIG2_DEFAULT 0xC0
#define ADS1299_CONFIG3_REFON 0xEC
#define ADS1299_CHNSET_ACTIVE 0x60
#define ADS1299_CHNSET_POWERDOWN 0x81
#define ADS1299_MISC1_SRB1ON 0x20
 
// DRDY active low
#define ADS1299_DRDY_READY 0 

// API declarations
ADS1299_Status ADS1299_Init(void);
ADS1299_Status ADS1299_WriteReg(uint8_t address, uint8_t value);
ADS1299_Status ADS1299_ReadReg(uint8_t address, uint8_t *value);
ADS1299_Status ADS1299_ReadSample(int32_t *ch1);
void TMR1_Init(void);
 
#endif /* ADS1299_H */