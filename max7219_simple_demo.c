/*
*********************************************************************************************************
* Module     : MAX7219_DEMO.C
* Description: MAX7219 LED demo.
*              This demo display all segments and dots of the LED.  It will display
*              first at dim level for 2 seconds then at brighter level for 2 seconds
*              and loops back.
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
#include <avr/io.h>                                   // microcontroller header file
#include <util/delay.h>
#include <avr/interrupt.h>
#include "max7219.h"                                  // MAX7219 header file

/*
*********************************************************************************************************
* intrInit() 
*
* Description: Interrupt initialization to read the buttons with proper debouncing.
* Arguments  : None
* Returns    : none
*********************************************************************************************************
*/
void intrInit() {
}

/*
*********************************************************************************************************
* ioInit() 
*
* Description: Initializes I/O pins.
* Arguments  : None
* Returns    : none
*********************************************************************************************************
*/
void ioInit (void) {
}


/*
*********************************************************************************************************
* main
*
* Description: Main
* Arguments  : None
* Returns    : none
*********************************************************************************************************
*/
int main(void) {
  
  MAX7219Init();
  
  ioInit();
  intrInit();

  // 4 digits + the ':' on the display.
  MAX7219Write(REG_SCAN_LIMIT, 5);       // 5 digit scan

  // Loop forever and display all segments with brightness of
  // 3 and 15 alternatively.
  uint8_t brightness_levels[2] = {3,15};
  uint8_t index = 0;
  while (1) {
    // No button pressed.  Light up everything on the display.
    index = index ^ 1;
    MAX7219SetBrightness(brightness_levels[index]);
    MAX7219DisplayChar(1, '8', 0x80);
    MAX7219DisplayChar(2, '8', 0x80);
    MAX7219DisplayL123(L1 | L2 | L3);
    MAX7219DisplayChar(4, '8', 0x80);
    MAX7219DisplayChar(5, '8', 0x80);
    _delay_ms(2000);
  }

  return 0;
}
