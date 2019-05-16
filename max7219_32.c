/*
*********************************************************************************************************
* Module     : MAX7219_32.C
* Description: MAX7219 LED Display Driver Routines for 32-bit AVR MCUs
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
#include "compiler.h"
#include "preprocessor.h"
#include "board.h"
#include "gpio.h"

#include "max7219.h"                                  // MAX7219 header file


/********************************************************************************************************
* Macros

  OUTPUT:
  PA05 -> pin 14 (DS, the MAX7219's data pin)--the bits that we want to
  shift into the MAX7219's shift register.

  PA06 -> pin 12 (ST_CP, the MAX7219's latch pin) for the ALL MAX7219s.
  When this is HIGH, we basically tell the MAX7219 to flush out the output.

  PA07 -> pin 11 (SH_CP, the MAX7219's shift register clock pin)
  to generate clock pulses for shifting bits into the shift register.
********************************************************************************************************/
#define GPIO_DATA_PIN  AVR32_PIN_PA05
#define GPIO_CLK_PIN   AVR32_PIN_PA06
#define GPIO_LOAD_PIN  AVR32_PIN_PA07

#define DATA_0()      gpio_clr_gpio_pin(GPIO_DATA_PIN)
#define DATA_1()      gpio_set_gpio_pin(GPIO_DATA_PIN)
#define CLK_0()       gpio_clr_gpio_pin(GPIO_CLK_PIN)
#define CLK_1()       gpio_set_gpio_pin(GPIO_CLK_PIN)
#define LOAD_0()      gpio_clr_gpio_pin(GPIO_LOAD_PIN)
#define LOAD_1()      gpio_set_gpio_pin(GPIO_LOAD_PIN)

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
static const struct {
	char   ascii;
	char   segs;
} SegmentData[] = {
  {' ', 0x00},
  {'0', 0x7e},
  {'1', 0x30},
  {'2', 0x6d},
  {'3', 0x79},
  {'4', 0x33},
  {'5', 0x5b},
  {'6', 0x5f},
  {'7', 0x70},
  {'8', 0x7f},
  {'9', 0x7b},
  {'A', 0x77},
  {'B', 0x1f},
  {'C', 0x4e},
  {'D', 0x3d},
  {'E', 0x4f},
  {'F', 0x47},
  {'.', 0x80},
  {'\0', 0x00}
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
static unsigned char MAX7219LookupCode (char character);

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

  gpio_enable_gpio_pin(GPIO_DATA_PIN);
  gpio_enable_gpio_pin(GPIO_CLK_PIN);
  gpio_enable_gpio_pin(GPIO_LOAD_PIN);

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
void MAX7219DisplayChar (char digit, char character, unsigned char setDot) {
  character = toupper(character);
  unsigned char byte = MAX7219LookupCode(character);
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
* MAX7219LookupCode()
*
* Description: Convert an alphanumeric character to the corresponding 7-segment code.
* Arguments  : character to display
* Returns    : segment code
*********************************************************************************************************
*/
static unsigned char MAX7219LookupCode (char character) {
  character = toupper(character);
  unsigned char i;
  for (i = 0; SegmentData[i].ascii; i++)      // scan font table for ascii code
    if (character == SegmentData[i].ascii)
      return SegmentData[i].segs;                     // return segments code
  return 0;                                           // code not found, return null (blank)
}

/*********************************************************************************************************
* MAX7219SendByte()
*
* Description: Send one byte to the MAX7219
* Arguments  : dataout = data to send
* Returns    : none
*********************************************************************************************************
*/
static void MAX7219SendByte (unsigned char dataout) {
  char i;
  for (i = 8; i > 0; i--) {
    unsigned char mask = 1 << (i - 1);                // calculate bitmask
    CLK_0();                                          // bring CLK low
    if (dataout & mask)                               // output one data bit
      DATA_1();                                       //  "1"
    else                                              //  or
      DATA_0();                                       //  "0"
    CLK_1();                                          // bring CLK high
  }
}
