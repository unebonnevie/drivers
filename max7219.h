/*
*********************************************************************************************************
* Module     : MAX7219.H
* Description: Header file for MAX7219.C (LED Display Driver Routines)
*
* DISCLAIMER:
* -No warranty or whatsoever.  This is a sample/demo.
* -The author holds no responsibility of any sort of damage not limited to physically, emotionally, mentally,
*  medically, finanically, or property-wise.  In other words, use at your own risks and your own
*  liability.
* -Do not use this sample commercially.
*********************************************************************************************************
*/

#ifndef _MAX7219H
#define _MAX7219H

/*
*********************************************************************************************************
* Constants
*********************************************************************************************************
*/
#define REG_DECODE        0x09                        // "decode mode" register
#define REG_INTENSITY     0x0a                        // "intensity" register
#define REG_SCAN_LIMIT    0x0b                        // "scan limit" register
#define REG_SHUTDOWN      0x0c                        // "shutdown" register
#define REG_DISPLAY_TEST  0x0f                        // "display test" register

#define INTENSITY_MIN     0x00                        // minimum display intensity
#define INTENSITY_MAX     0x0f                        // maximum display intensity

// For the colon and degree dots on the 7-segment display.
#define L1 0x01
#define L2 0x02
#define L3 0x04

/*
*********************************************************************************************************
* Public Function Prototypes
*********************************************************************************************************
*/
void MAX7219Init (void);
void MAX7219ShutdownStart (void);
void MAX7219ShutdownStop (void);
void MAX7219DisplayTestStart (void);
void MAX7219DisplayTestStop (void);
void MAX7219SetBrightness (char brightness);
void MAX7219Clear (void);
void MAX7219DisplayChar (char digit, char character, uint8_t setDot);
void MAX7219DisplayL123 (char bits);
void MAX7219Write (unsigned char reg_number, unsigned char data);
#endif // _MAX7219H
