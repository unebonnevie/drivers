/*
*********************************************************************************************************
* Module     : MAX7219.C
* Description: MAX7219 LED Display Driver Routines
*
*  The MAX7219 is configured for "no decode" mode, rather than "code B" decoding.  This
*  allows the program to display more than the 0-9,H,E,L,P that code B provides.  However,
*  the "no decode" method requires that each character to be displayed have a corresponding
*  entry in a lookup table, to convert the ascii character to the proper 7-segment code.
*  If you need to display more than the characters, simply add them to the table.
*
*  Please see the datasheet for more details.
*
* 20110723 by jucing:
*  * Modified for the ATMEGA8/ATMEGA328 and enhanced to work with the 7-segment display for the
*    EG-400 Katana.
*
* DISCLAIMER:
* -No warranty or whatsoever.  This is a sample/demo.
* -The author holds no responsibility of any sort of damage not limited to physically, emotionally, mentally,
*  medically, finanically, or property-wise.  In other words, use at your own risks and your own
*  liability.
* -Do not use this sample commercially.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
* Include Header Files
*********************************************************************************************************
*/
#include <ctype.h>
#include <avr/io.h>                                   // microcontroller header file
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "max7219.h"                                  // MAX7219 header file


/********************************************************************************************************
* Macros

  OUTPUT:
  PC0 -> pin 14 (DS, the MAX7219's data pin)--the bits that we want to
  shift into the MAX7219's shift register.

  PC1 -> pin 12 (ST_CP, the MAX7219's latch pin) for the ALL MAX7219s.
  When this is HIGH, we basically tell the MAX7219 to flush out the output.

  PC2 -> pin 11 (SH_CP, the MAX7219's shift register clock pin)
  to generate clock pulses for shifting bits into the shift register.
********************************************************************************************************/
#define DATA_PORT     PORTC                           // assume "DATA" is on PC.0
#define DATA_DDR      DDRC
#define DATA_BIT      0x01
#define DATA_0()      (DATA_PORT &= ~DATA_BIT)
#define DATA_1()      (DATA_PORT |=  DATA_BIT)
#define CLK_PORT      PORTC                           // assume "CLK" is on PC.2
#define CLK_DDR       DDRC
#define CLK_BIT       0x04
#define CLK_0()       (CLK_PORT &= ~CLK_BIT)
#define CLK_1()       (CLK_PORT |=  CLK_BIT)
#define LOAD_PORT     PORTC                           // assume "LOAD" is on PC.1
#define LOAD_DDR      DDRC
#define LOAD_BIT      0x02
#define LOAD_0()      (LOAD_PORT &= ~LOAD_BIT)
#define LOAD_1()      (LOAD_PORT |=  LOAD_BIT)

/*
*********************************************************************************************************
* LED Segments:         a
*                     ----
*                   f|    |b
*                    |  g |
*                     ----
*                   e|    |c
*                    |    |
*                     ----  o dp
*                       d
*   Register bits:
*      bit:  7  6  5  4  3  2  1  0
*           dp  a  b  c  d  e  f  g
*
* NOTES:
* -You can define more patterns here and use PROGMEM to store the patterns into the flash to save
*  SRAM!
*********************************************************************************************************
*/
const uint8_t SegmentData[] PROGMEM = {
  0b00000000,  // space
  0b00000110,  // !
  0b10001000,  // "
  0b11101000,  // #
  0b00011101,  // $
  0b01011100,  // %
  0b10111101,  // &
  0b00001000,  // ' (single quote)
  0b00111100,  // (, left-parenthesis
  0b11010100,  // ), right-parenthesis
  0b10001101,  // *
  0b00101001,  // +
  0b01010000,  // ,
  0b00000001,  // -
  0x80,  // . or dp
  0b10000001,  // /
  0x7e,  // 0
  0x30,  // 1
  0x6d,  // 2
  0x79,  // 3
  0x33,  // 4
  0x5B,  // 5
  0x5F,  // 6
  0x70,  // 7
  0x7F,  // 8
  0x7B,  // 9
  0b00010001,  // :
  0b01010001,  // ;
  0b00001101,  // <
  0b00000101,  // =
  0b10000101,  // >
  0b10100101,  // ?
  0b11110101,  // @
  0x77,  // A
  0x1F,  // B
  0x4E,  // C
  0x3D,  // D
  0x4F,  // E
  0x47,  // F
  0b01111100,  // G
  0b01101001,  // H
  0b00101000,  // I
  0b11110000,  // J
  0b01101101,  // K
  0b00111000,  // L
  0b01100100,  // M
  0b11101100,  // N
  0b11111100,  // O
  0b10101101,  // P
  0b11001101,  // Q
  0b10101100,  // R
  0b01011101,  // S
  0b00111001,  // T
  0b11111000,  // U
  0b11011000,  // V
  0b10011000,  // W
  0b11101001,  // X
  0b11011001,  // Y
  0b10010101,  // Z
  0b00111100,  // [
  0b00001001,  // \ back slash
  0b11010100,  // ]
  0b10001100,  // ^
  0b00010000,  // _
};

/*
*********************************************************************************************************
* Private Data
*********************************************************************************************************
*/

/*
*********************************************************************************************************
* Private Function Prototypes
*********************************************************************************************************
*/
static void MAX7219SendByte (unsigned char data);


// ...................................... Public Functions ..............................................


/*
*********************************************************************************************************
* MAX7219Init()
*
* Description: Initialize MAX7219 module; must be called before any other MAX7219 functions.
* Arguments  : none
* Returns    : none
*********************************************************************************************************
*/
void MAX7219Init (void) {
  DATA_DDR |= DATA_BIT;                               // configure "DATA" as output
  CLK_DDR  |= CLK_BIT;                                // configure "CLK"  as output
  LOAD_DDR |= LOAD_BIT;                               // configure "LOAD" as output

  MAX7219Write(REG_SCAN_LIMIT, 7);                   // set up to scan all eight digits
  MAX7219Write(REG_DECODE, 0x00);                    // set to "no decode" for all digits
  MAX7219ShutdownStop();                             // select normal operation (i.e. not shutdown)
  MAX7219DisplayTestStop();                          // select normal operation (i.e. not test mode)
  MAX7219Clear();                                    // clear all digits
  MAX7219SetBrightness(INTENSITY_MAX);               // set to maximum intensity
}


/*
*********************************************************************************************************
* MAX7219Write()
*
* Description: Write to MAX7219
* Arguments  : reg_number = register to write to, basically the digit id, 0-7.
*              dataout = data to write to MAX7219
* Returns    : none
*********************************************************************************************************
*/
void MAX7219Write (unsigned char reg_number, unsigned char dataout) {
  LOAD_1();                                           // take LOAD high to begin
  MAX7219SendByte(reg_number);                        // write register number to MAX7219
  MAX7219SendByte(dataout);                           // write data to MAX7219
  LOAD_0();                                           // take LOAD low to latch in data
  LOAD_1();                                           // take LOAD high to end
}


/*
*********************************************************************************************************
* MAX7219ShutdownStart()
*
* Description: Shut down the display.
* Arguments  : none
* Returns    : none
*********************************************************************************************************
*/
void MAX7219ShutdownStart (void) {
  MAX7219Write(REG_SHUTDOWN, 0);                     // put MAX7219 into "shutdown" mode
}


/*
*********************************************************************************************************
* MAX7219ShutdownStop()
*
* Description: Take the display out of shutdown mode.
* Arguments  : none
* Returns    : none
*********************************************************************************************************
*/
void MAX7219ShutdownStop (void) {
  MAX7219Write(REG_SHUTDOWN, 1);                     // put MAX7219 into "normal" mode
}


/*
*********************************************************************************************************
* MAX7219DisplayTestStart()
*
* Description: Start a display test.
* Arguments  : none
* Returns    : none
*********************************************************************************************************
*/
void MAX7219DisplayTestStart (void) {
  MAX7219Write(REG_DISPLAY_TEST, 1);                 // put MAX7219 into "display test" mode
}


/*
*********************************************************************************************************
* MAX7219DisplayTestStop()
*
* Description: Stop a display test.
* Arguments  : none
* Returns    : none
*********************************************************************************************************
*/
void MAX7219DisplayTestStop (void) {
  MAX7219Write(REG_DISPLAY_TEST, 0);                 // put MAX7219 into "normal" mode
}


/*
*********************************************************************************************************
* MAX7219SetBrightness()
*
* Description: Set the LED display brightness
* Arguments  : brightness (0-15)
* Returns    : none
*********************************************************************************************************
*/
void MAX7219SetBrightness (char brightness) {
  brightness &= 0x0f;                                // mask off extra bits
  MAX7219Write(REG_INTENSITY, brightness);           // set brightness
}


/*
*********************************************************************************************************
* MAX7219Clear()
*
* Description: Clear the display (all digits blank)
* Arguments  : none
* Returns    : none
*********************************************************************************************************
*/
void MAX7219Clear (void) {
  char i;
  for (i=0; i < 8; i++)
    MAX7219Write(i, 0x00);                           // turn all segments off
}


/*
*********************************************************************************************************
* MAX7219DisplayChar()
*
* Description: Display a character on the specified digit.
* Arguments  : digit = digit number (1-8)
*              character = character to display (0-9, A-Z, etc.)
*              setDot = 0x80 to enable the digit's decimal dot. 0x00 not to enable.
* Returns    : none
*********************************************************************************************************
*/
void MAX7219DisplayChar (char digit, char character, uint8_t setDot) {
  character = toupper(character);
  uint8_t byte = pgm_read_byte(&SegmentData[character - 32]);
  MAX7219Write(digit, byte | setDot);
}

/*
*********************************************************************************************************
* MAX7219DisplayL123()
*
* Description: Display the colon and/or degree dots
* Arguments  : bit = L1/L2/L3 constants can be OR'ed together to display any of the dots of the colon
*              or the degree dot.
* Returns    : none
*********************************************************************************************************
*/
void MAX7219DisplayL123(char bits) {
  MAX7219Write(3, bits << 4);
}	

// ..................................... Private Functions ..............................................
/*
*********************************************************************************************************
* MAX7219SendByte()
*
* Description: Send one byte to the MAX7219
* Arguments  : dataout = data to send
* Returns    : none
*********************************************************************************************************
*/
static void MAX7219SendByte (unsigned char dataout) {
  char i;
  for (i=8; i>0; i--) {
    unsigned char mask = 1 << (i - 1);                // calculate bitmask
    CLK_0();                                          // bring CLK low
    if (dataout & mask)                               // output one data bit
      DATA_1();                                       //  "1"
    else                                              //  or
      DATA_0();                                       //  "0"
    CLK_1();                                          // bring CLK high
  }
}
